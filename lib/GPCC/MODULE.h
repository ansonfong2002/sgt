// MODULE.h: (10/17/23, RD)

// references:
// - SpecC
// - SystemC

#ifndef MODULE_H
#define MODULE_H

#include <list>

class MODULE
{
    public:
	MODULE(void)
	{
	    modules_.push_back(this);
	}

	virtual ~MODULE(void)
	{
	}

	virtual void main(void)
	{
	}

    private:
	static std::list<MODULE*> modules_;
};

std::list<MODULE*> MODULE::modules_;

#endif // MODULE_H

/* ex: set softtabstop=4 shiftwidth=4: */
