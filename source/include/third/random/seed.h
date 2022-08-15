/**************************************************************************\
*                                                                          *
*   init_rand_digits(n)                                                    *
*                                                                          *
*   This routine creates a random 64-bit pattern which can be used to      *
*   initialize the constant s in the state vector.  The pattern is chosen  *
*   so that the 8 upper hexadecimal digits are different and also that the *
*   8 lower digits are different.  Only non-zero digits are used and the   *
*   last digit is chosen to be odd.  Roughly half the bits will be set to  *
*   one and roughly half the bits will be set to zero.  This assures       *
*   sufficient change in the Weyl sequence on each iteration of the RNG.   *
*                                                                          *
*   The input parameter n is a 64-bit number in the range of 0 to 2^64-1.  *
*   A set of random digits will be created for this input value.  This     *
*   routine has been verified to produce unique constants for input values *
*   from 0 to 3 billion.  For input values greater than 3 billion, some    *
*   previously generated constants may occur at random.                    *
*                                                                          *
*   About 33 quadrillion constants may generated.  Since the stream length *
*   is 2^64, this provides about 2^118 random numbers in total.            *
*                                                                          *
*   One may use this routine as follows to initialize the state:           *
*                                                                          *
*   #include "init.h"                                                      *
*                                                                          *
*   x = w = s = init_rand_digits(n);                                       *
*                                                                          *
*   Alternatively, one may use init_rand_digits offline to create an       *
*   include file of constants which could be compiled into the the program.*
*   This would provide the fastest initialization of the state.  This may  *
*   useful for certain time critical applications which require a fast     *
*   initialization. The seed directory contains a program which creates    *
*   a seed.h file which may be used as shown below.                        *
*                                                                          *
*   uint64_t seed[] = {                                                    *
*   #include "seed.h"                                                      *
*   };                                                                     *
*                                                                          *
*   x = w = s = seed[n];                                                   *
*                                                                          *
*   See streams_example for example usage.                                 *
*                                                                          *
\**************************************************************************/

#pragma once

#include <stdint.h>
#include <third/random/state.h>

namespace Msws {

constexpr uint64_t sconst[] = {
		0x37e1c9b5e1a2b843, 0x56e9d7a3d6234c87, 0xc361be549a24e8c7, 0xd25b9768a1582d7b, 0x18b2547d3de29b67,
		0xc1752836875c29ad, 0x4e85ba61e814cd25, 0x17489dc6729386c1, 0x7c1563ad89c2a65d, 0xcdb798e4ed82c675,
		0xd98b72e4b4e682c1, 0xdacb7524e4b3927d, 0x53a8e9d7d1b5c827, 0xe28459db142e98a7, 0x72c1b3461e4569db,
		0x1864e2d745e3b169, 0x6a2c143bdec97213, 0xb5e1d923d741a985, 0xb4875e967bc63d19, 0x92b64d5a82db4697,
		0x7cae812d896eb1a5, 0xb53827d41769542d, 0x6d89b42c68a31db5, 0x75e26d434e2986d5, 0x7c82643d293cb865,
		0x64c3bd82e8637a95, 0x2895c34d9dc83e61, 0xa7d58c34dea35721, 0x3dbc5e687c8e61d5, 0xb468a235e6d2b193,
};

/* local copy of msws rng */

uint64_t xi, wi, si;

inline static uint32_t mswsi() {
	xi *= xi;
	xi += (wi += si);
	return xi = (xi >> 32) | (xi << 32);
}

inline static uint64_t init_rand_digits(uint64_t n) {
	uint64_t c, i, j, k, m, r, t, u, v;

	/* initialize state for local msws rng */

	r  = n / 100000000;
	t  = n % 100000000;
	si = sconst[r % 30];
	r /= 30;
	xi = wi = t * si + r * si * 100000000;

	/* get odd random number for low order digit */

	u       = (mswsi() % 8) * 2 + 1;
	v       = (1 << u);

	/* get rest of digits */

	for (m = 60, c = 0; m > 0;) {
		j = mswsi(); /* get 8 digit 32-bit random word */
		for (i = 0; i < 32; i += 4) {
			k = (j >> i) & 0xf;                  /* get a digit */
			if (k != 0 && (c & (1 << k)) == 0) { /* not 0 and not previous */
				c |= (1 << k);
				u |= (k << m); /* add digit to output */
				m -= 4;
				if (m == 24 || m == 28) c = (1 << k) | v;
				if (m == 0) break;
			}
		}
	}

	return u;
}

inline static State seed(uint64_t n) {
	auto seed = init_rand_digits(n);
	return State{.x = seed, .w = seed, .s = seed};
}

}  // namespace Msws
