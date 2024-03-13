/* Memory.cpp (08/30/22, RD) */

#ifndef MEMORY_CPP
#define MEMORY_CPP

//#define DEBUG
#ifndef DEBUG_MODULE
#define DEBUG_MODULE(s,...)
//#define DEBUG_MODULE(s,...) printf("DEBUG:%8llu.%4llu:%s: " s "\n", sc_time_stamp().value()/1000000, sc_delta_count(), name(), __VA_ARGS__)
#endif

// RISC-V VP doesn't support these, so we will ignore them as well (12/05/22, RD)
#define IGNORE_STREAMING_WIDTH
#define IGNORE_BYTE_ENABLE_PTR

SC_MODULE(Memory)
{
    tlm_utils::simple_target_socket<Memory> s0;

    unsigned char *M;
    size_t Size;
    sc_time ReadAccessDelay;
    sc_time WriteAccessDelay;

    Memory(sc_module_name n, size_t size,
	sc_time delay_read = sc_time(0,SC_NS),
	sc_time delay_write = sc_time(0,SC_NS)):
	sc_module(n),
	Size(size),
	ReadAccessDelay(delay_read),
	WriteAccessDelay(delay_write)
    {
	M = new unsigned char[size];
#ifdef DEBUG
	memset(M, 0x01, size);
	DEBUG_MODULE("size=0x%08lx, init=%s...", size, HexDump(M, 8));
#endif
	s0.register_b_transport(this, &Memory::access);
    }

    void access(tlm::tlm_generic_payload& trans, sc_time& delay)
    {
	tlm::tlm_command  cmd = trans.get_command();
	sc_dt::uint64     adr = trans.get_address();
	unsigned char     *ptr = trans.get_data_ptr();
	unsigned int      len = trans.get_data_length();
#ifndef IGNORE_STREAMING_WIDTH
	unsigned int      wid = trans.get_streaming_width();
#endif
#ifndef IGNORE_BYTE_ENABLE_PTR
	unsigned char     *byt = trans.get_byte_enable_ptr();
#endif
	if (  (adr + len <= Size)
	    &&(ptr != 0)
#ifndef IGNORE_STREAMING_WIDTH
	    &&(wid == len)
#endif
#ifndef IGNORE_BYTE_ENABLE_PTR
	    &&(byt == 0)
#endif
	   )
	{
	    if (cmd == tlm::TLM_READ_COMMAND)
	    {
		memcpy(ptr, &M[adr], len);
		DEBUG_MODULE("READ(adr=%llu,len=%u)=%s",adr,len,HexDump(ptr,len));
		trans.set_response_status(tlm::TLM_OK_RESPONSE);
		delay += (len/4)*ReadAccessDelay; //per word delay
	    }
	    else if (cmd == tlm::TLM_WRITE_COMMAND)
	    {
		memcpy(&M[adr], ptr, len);
		DEBUG_MODULE("WRITE(adr=%llu,len=%u)=%s",adr,len,HexDump(ptr,len));
		trans.set_response_status(tlm::TLM_OK_RESPONSE);
		delay += (len/4)*WriteAccessDelay; //per word delay
	    }
	    else
	    {
		DEBUG_MODULE("ERROR(cmd=%d neither READ nor WRITE)",(int)cmd);
		trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
	    }
	}
	else
	{
#ifndef IGNORE_STREAMING_WIDTH
	    DEBUG_MODULE("ERROR(adr=%llu,len=%u,size=%lu,wid=%u)",adr,len,Size,wid);
#else
	    DEBUG_MODULE("ERROR(adr=%llu,len=%u,size=%lu)",adr,len,Size);
#endif
	    assert(ptr);
	    trans.set_response_status(tlm::TLM_GENERIC_ERROR_RESPONSE);
	}
    }

#ifdef DEBUG
  char *HexDump(const unsigned char *p, unsigned int l=4)
  {
    static char Buffer[256];
    char *b = Buffer;

    for(unsigned int i=0; i<l && i<16; i++)
    {
      if (i && i%4==0) *b++=' ';
      sprintf(b, "%02x", p[i]);
      b += 2;
    }
    if (l>16)
    {
      *b++='.';
      *b++='.';
      *b++='.';
    }
    if (l==4)
    {
      sprintf(b, " (int=%d)", *((int*)p));
    }
    else
    {
      *b = 0;
    }
    return(Buffer);
  }
#endif
};

#endif

// EOF
