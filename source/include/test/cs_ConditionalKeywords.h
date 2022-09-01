/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 1, 2022
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#pragma once

/**
 * implementation of some inline functions is deferred to mock .cpp file on host.
 * In that case, define the prototype as:
 *
 * INLINE retVal foo(Arg0 a0, ...);
 */
#if !defined HOST_TARGET
#define INLINE inline
#else
#define INLINE
#endif
