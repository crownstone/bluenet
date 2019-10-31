/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Oct 31, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

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
    if(Fletcher(test2,sizeof(test2)) != 0xEBE19591) { 
        LOGe("Fletcher test 2 broken"); 
        return -1; 
    }
    
    return 0;    
}