/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 31, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <util/cs_Hash.h>

int main(){
    // just a few strings with known hashes
    uint8_t test0[] = {'a','b','c','d','e'};
    uint8_t test1[] = {'a','b','c','d','e','f'};
    uint8_t test2[] = {'a','b','c','d','e','f','g','h'};

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
    fletch_aggr1 = Fletcher(test3+0, 1, fletch_aggr1);
    fletch_aggr1 = Fletcher(test3+2, 1, fletch_aggr1);
    fletch_aggr1 = Fletcher(test3+4, 1, fletch_aggr1);
    fletch_aggr1 = Fletcher(test3+6, 1, fletch_aggr1);

    if(fletch_aggr1_1 != Fletcher(test3,sizeof(test3))){
        LOGe("Fletcher test aggr1 broken");
        return -1;
    }
    
    return 0;    
}