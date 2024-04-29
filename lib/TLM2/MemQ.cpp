/* MemQ.cpp */
// 10/13/22, RD	first version
// 11/24/22, RD	API revision

#ifndef MEMQ_CPP
#define MEMQ_CPP

#include <assert.h>

#ifndef MEMQ_INITIALIZATION_DELAY
#define MEMQ_INITIALIZATION_DELAY sc_time(1, SC_MS)
#endif

//#define DEBUG_MEMQ	// enable/disable

#ifdef DEBUG_MEMQ
#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF(s,...) printf("DEBUG: " s "\n", __VA_ARGS__)
#endif
#else
#ifndef DEBUG_PRINTF
#define DEBUG_PRINTF(s,...)
#endif
#endif

template <class T, unsigned int S>
struct Queue
{
	unsigned int Sent;
	unsigned int Rcvd;
	T Buffer[S];
};

class SharedMem	// shared memory
{
public:
	tlm::tlm_initiator_socket<> &m_sock;
	sc_event &m_sig;
	sc_dt::uint64 m_addr;
	unsigned int m_offset;
	unsigned int m_size;
	const unsigned int ChannelStorageOffset = 0x080;

	SharedMem(tlm::tlm_initiator_socket<> &sock,	// socket to memory
		sc_event &sig,				// event for memory updates
		sc_dt::uint64 addr,			// memory start address
		unsigned int size)			// memory size
		: m_sock(sock)
		, m_sig(sig)
		, m_addr(addr)
		, m_size(size)
	{
	    /* m_offset remains unitialized until host sets it */
	    assert(ChannelStorageOffset < size);
	}

	void MemAccess(
	    tlm::tlm_command cmd,
	    sc_dt::uint64 a,
	    void *data,
	    unsigned int len)
	{
	  assert((a >= m_addr) && (a < m_addr+m_size));
	  sc_time delay = SC_ZERO_TIME;
	  tlm::tlm_generic_payload trans;
	  trans.set_command(cmd);
	  trans.set_address(a);
	  trans.set_data_ptr(reinterpret_cast<unsigned char*>(data));
	  trans.set_data_length(len);
	  trans.set_streaming_width(len);
	  trans.set_byte_enable_ptr(0);
	  trans.set_dmi_allowed(false);
	  trans.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
	  m_sock->b_transport(trans, delay);
	  if (trans.get_response_status() != tlm::TLM_OK_RESPONSE)
	  {
	    SC_REPORT_FATAL("SharedMem::MemAccess", trans.get_response_string().c_str());
	  }
	  wait(delay);
	}

	void Load(
	    sc_dt::uint64 a,
	    unsigned int &d)
	{
	    MemAccess(tlm::TLM_READ_COMMAND, a, &d, sizeof(d));
	}

	void Store(
	    sc_dt::uint64 a,
	    unsigned int d)
	{
	    MemAccess(tlm::TLM_WRITE_COMMAND, a, &d, sizeof(d));
	}
};

class HostedSM : public SharedMem	// hosted shared memory segment
{
public:
	HostedSM(tlm::tlm_initiator_socket<> &sock,	// socket to memory
		sc_event &sig,				// event for memory updates
		sc_dt::uint64 addr,			// memory start address
		unsigned int offset,			// offset to shared segment
		unsigned int size)			// size of shared segment
		: SharedMem(sock, sig, addr, size)
	{
	    m_offset = offset;
	    Store(addr+ChannelStorageOffset, offset);	// set the offset to the shared data
	    DEBUG_PRINTF("HostedSM@%06lx.CTOR(addr=%08llx, size=%u, offset=%u)",
			(long)this&0xffffff, addr, size, offset);
	}
};

class RemoteSM : public SharedMem	// remote shared memory segment
{
public:
	RemoteSM(tlm::tlm_initiator_socket<> &sock,	// socket to memory
		sc_event &sig,				// event for memory updates
		sc_dt::uint64 addr,			// memory start address
		unsigned int size)			// size of shared segment
		: SharedMem(sock, sig, addr, size)
	{
	    Load(addr+ChannelStorageOffset, m_offset);	// get the offset to the shared data
	    DEBUG_PRINTF("RemoteSM@%06lx.CTOR(addr=%08llx, size=%u) m_offset=%u",
			(long)this&0xffffff, addr, size, m_offset);
	    assert((m_offset > 0) && ((m_offset & 0x3) == 0));	// host set plausible value?
	}
};

template <class T, unsigned int S>
class MemQ
{
private:
	SharedMem    &m_sm;
	sc_dt::uint64 m_addr;
	unsigned int  m_sent;	// owned by sender
	unsigned int  m_rcvd;	// owned by receiver

public:
	MemQ(HostedSM &hsm, unsigned int channel_offset)
		: m_sm(hsm)
		, m_addr(hsm.m_addr + hsm.m_offset + channel_offset)
		, m_sent(0)
		, m_rcvd(0)
	{
	    typedef Queue<T,S> qtype;

	    DEBUG_PRINTF("MemQ@%06lx.CTOR(HostedSM@%06lx, channel_offset=%u",
			(long)this&0xffffff, (long)&hsm&0xffffff, channel_offset);

	    // host must initialize the queue counters in the shared memory
	    hsm.Store(m_addr + offsetof(qtype,Sent), m_sent);
	    hsm.Store(m_addr + offsetof(qtype,Rcvd), m_rcvd);
	}

	MemQ(RemoteSM &rsm, unsigned int channel_offset)
		: m_sm(rsm)
		, m_addr(rsm.m_addr + rsm.m_offset + channel_offset)
		, m_sent(0)
		, m_rcvd(0)
	{
	    DEBUG_PRINTF("MemQ@%06lx.CTOR(RemoteSM@%06lx, channel_offset=%u",
			(long)this&0xffffff, (long)&rsm&0xffffff, channel_offset);

	    // remote client trusts host to initialize the queue in the shared memory
	}

	void Push(
	    T &data)
	{
	    unsigned int Rcvd;
	    typedef Queue<T,S> qtype;

	    m_sm.Load(m_addr+offsetof(qtype,Rcvd), Rcvd);
	    DEBUG_PRINTF("MemQ@%06lx.Push m_sent-Rcvd = %u-%u = %u (%s)",
			(long)this&0xffffff, m_sent, Rcvd, m_sent-Rcvd,
			m_sent-Rcvd>=S ? "full":"avail");
	    assert((m_sent-Rcvd >= 0) && (m_sent-Rcvd <= S));
	    while(m_sent - Rcvd >= S)
	    {
		wait(m_sm.m_sig);
		m_sm.Load(m_addr+offsetof(qtype,Rcvd), Rcvd);
		DEBUG_PRINTF("MemQ@%06lx.Push m_sent-Rcvd = %u-%u = %u (%s)",
			(long)this&0xffffff, m_sent, Rcvd, m_sent-Rcvd,
			m_sent-Rcvd>=S ? "full":"avail");
	    }
	    StoreBuffer(m_addr+offsetof(qtype,Buffer[0])+(m_sent%S)*sizeof(T), data);
	    m_sm.Store(m_addr+offsetof(qtype,Sent), ++m_sent);
	    m_sm.m_sig.notify(SC_ZERO_TIME);
	}

	void Pop(
	    T &data)
	{
	    unsigned int Sent;
	    typedef Queue<T,S> qtype;

	    m_sm.Load(m_addr+offsetof(qtype,Sent), Sent);
	    DEBUG_PRINTF("MemQ@%06lx.Pop  Sent-m_rcvd = %u-%u = %u (%s)",
			(long)this&0xffffff, Sent, m_rcvd, Sent-m_rcvd,
			Sent-m_rcvd==0 ? "empty":"avail");
	    assert((Sent-m_rcvd >= 0) && (Sent-m_rcvd <= S));
	    while(Sent - m_rcvd == 0)
	    {
		wait(m_sm.m_sig);
		m_sm.Load(m_addr+offsetof(qtype,Sent), Sent);
		DEBUG_PRINTF("MemQ@%06lx.Pop  Sent-m_rcvd = %u-%u = %u (%s)",
			(long)this&0xffffff, Sent, m_rcvd, Sent-m_rcvd,
			Sent-m_rcvd==0 ? "empty":"avail");
	    }
	    LoadBuffer(m_addr+offsetof(qtype,Buffer[0])+(m_rcvd%S)*sizeof(T), data);
	    m_sm.Store(m_addr+offsetof(qtype,Rcvd), ++m_rcvd);
	    m_sm.m_sig.notify(SC_ZERO_TIME);
	}

private:
	void LoadBuffer(
	    sc_dt::uint64 a,
	    T &d)
	{
	    m_sm.MemAccess(tlm::TLM_READ_COMMAND, a, &d, sizeof(d));
	}

	void StoreBuffer(
	    sc_dt::uint64 a,
	    T &d)
	{
	    m_sm.MemAccess(tlm::TLM_WRITE_COMMAND, a, &d, sizeof(d));
	}
};

#endif

// EOF
