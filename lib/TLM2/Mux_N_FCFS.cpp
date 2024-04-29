/* Mux_N_FCFS.cpp (07/19/22, RD) */

#ifndef MUX_N_FCFS_CPP
#define MUX_N_FCFS_CPP

template <unsigned N>
SC_MODULE(Mux_N)
{
public:
    tlm_utils::simple_target_socket<Mux_N> s[N];
    tlm_utils::simple_initiator_socket<Mux_N> out;

    void forward_request(tlm::tlm_generic_payload& trans, sc_time& delay)
    {
	sc_time d1, d2, memory_delay;

	// update the delay
	if (busy_until < sc_time_stamp())
	{
            busy_until = sc_time_stamp();
	}
	delay += MuxDelay + (busy_until - sc_time_stamp());

	// perform the memory transaction
	d1 = delay;
	out->b_transport(trans, delay);
	d2 = delay;
	memory_delay = d2 - d1;

	// update the scheduler state
	busy_until += memory_delay;
    }

    Mux_N(sc_module_name n,
	sc_time mux_delay = sc_time(0,SC_NS)):
	sc_module(n),
	MuxDelay(mux_delay),
	busy_until(sc_time(0,SC_NS))
    {
	for(unsigned i=0;i<N;i++)
	{
	    s[i].register_b_transport(this, &Mux_N::forward_request);
	}
    }

private:
    sc_time MuxDelay;
    // state of FCFS scheduling
    sc_time busy_until;
};

#endif

// EOF
