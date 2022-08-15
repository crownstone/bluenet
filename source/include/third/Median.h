#ifndef MEDIAN_H
#define MEDIAN_H

#include "third/Element.h"
//#include <cassert>
//#include <stdexcept>

struct MedianFilter {
	unsigned k;
	unsigned blocks;
	unsigned n;
	unsigned half;
	unsigned result;

	MedianFilter(unsigned half_, unsigned blocks_)
			: k{2 * half_ + 1}, blocks{blocks_}, n{k * blocks_}, half{half_}, result{k * (blocks_ - 1) + 1} {
		//        if (half_ == 0) {
		//            throw std::invalid_argument("half-window size must be at least 1");
		//        }
		//        if (blocks_ == 0) {
		//            throw std::invalid_argument("number of blocks must be at least 1");
		//        }
		//        if (n / blocks != k) {
		//            throw std::overflow_error("input too large");
		//        }
	}
};

#endif
