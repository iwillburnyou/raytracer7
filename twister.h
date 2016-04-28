#ifndef I_TWISTER_H
#define I_TWISTER_H

#include "common.h"

namespace Raytracer {

#define mtRand_N 624

class Twister
{
public:
	void Seed( unsigned long seed );	
	Twister( unsigned long seed )
	{
	    if (seed) { Seed(seed); } 
		else { Seed( (unsigned long)0xf2710812 ); }
	}
	Twister() { Seed( (unsigned long)0xf2710812 ); }
	real Rand();
	unsigned long RandL();
protected:
	 unsigned long mt[mtRand_N];
	 int mti;
};

}; // namespace Raytracer

#endif
