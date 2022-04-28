/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Dec 17, 2021
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */


#include <util/cs_Hash.h>
//#include <logging/cs_Logger.h>
#include <iostream>



//#undef LOGe
#define LOGe(X) do{std::cout << (X) << std::endl;}while(0)
//
//#undef LOGd
#define LOGd(X) do{std::cout << (X) << std::endl;}while(0)


#define XSTR(x) STR(x)
#define STR(x) #x

int main(){
    // just a few strings with known hashes
    uint8_t test0[] = {'a','b','c','d','e'};
    uint8_t test1[] = {'a','b','c','d','e','f'};
    uint8_t test2[] = {'a','b','c','d','e','f','g','h'};
    uint8_t test3[] = {'a','b','c','d','e','f','g','h', 'i','j','k','l','m','n','o','p'};

    if(Fletcher(test0,sizeof(test0)) != 0xF04FC729) {
        LOGe("Fletcher test 0 broken");
        return -1;
    }
    if(Fletcher(test1,sizeof(test1)) != 0x56502D2A) {
        LOGe("Fletcher test 1 broken");
        return -1;
    }

    uint32_t fletch2 = Fletcher(test2,sizeof(test2));
    if(fletch2 != 0xEBE19591) {
        LOGe("Fletcher test 2 broken");
        return -1;
    }

    // aggregated computation:
    uint32_t fletch_aggr0 = Fletcher(test2,4);
    fletch_aggr0 = Fletcher(test2 + 4, 4, fletch_aggr0);

    if(fletch_aggr0 != fletch2){
        LOGe("Fletcher test aggr broken");
	    return -1;
    }

    uint32_t fletch_aggr1 = 0;
    int stepsize = 2;

    if(sizeof(test3)%stepsize) {
    	LOGe("Fletcher test broken, sizeof(test3) not a multiple of stepsize");
		return -1;
    }

    for (auto i{0} ; i<sizeof(test3); i+=stepsize) {
		fletch_aggr1 = Fletcher(test3+i, stepsize, fletch_aggr1);
    }

    if(fletch_aggr1 != Fletcher(test3,sizeof(test3))){
        LOGe("Fletcher test aggr1 broken");
        return -1;
    }

    LOGe("success");
    fflush( stdout );
#pragma message("serial verbosity: " XSTR(SERIAL_VERBOSITY))

    return 0;
}
