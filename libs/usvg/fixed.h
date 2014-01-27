#ifndef _ARMFIX_H_
#define _ARMFIX_H_

// from http://wiki.leaflabs.com/Fixed_Point_Math
fff
/*
 * the following typedefs are valid only for cpu=cortex-m3 and arch=armv7-m
 */

// unsigned fractionals
typedef sat unsigned short fract	fix_0_8_t;	// size: 1
typedef sat unsigned fract		fix_0_16_t;	// size: 2
typedef sat unsigned long fract		fix_0_32_t;	// size: 4
typedef sat unsigned long long fract	fix_0_64_t;	// size: 8

// signed fractionals
typedef sat short fract			fix_s_7_t;	// size: 1
typedef sat fract			fix_s_15_t;	// size: 2
typedef sat long fract			fix_s_31_t;	// size: 4
typedef sat long long fract		fix_s_63_t;	// size: 8

// unsigned accumulators
typedef sat unsigned short accum	fix_8_8_t;	// size: 2
typedef sat unsigned accum		fix_16_16_t;	// size: 4
typedef sat unsigned long accum		fix_32_32_t;	// size: 8
//typedef sat unsigned long long accum	fix_32_32_t;	// size: 8

// signed accumulators
typedef sat short accum			fix_s7_8_t;	// size: 2
typedef sat accum			fix_s15_16_t;	// size: 4
typedef sat long accum			fix_s31_32_t;	// size: 8
//typedef sat long long accum		fix_s31_32_t;   // size: 8

#endif /* _ARMFIX_H */
