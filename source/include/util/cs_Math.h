/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Okt 18, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

namespace CsMath{

    /**
     * Returns the canonical representation of [v] considered as element
     * of Z/mZ, regardless of any silly features C++ board thought that 
     * would be better than actual math when using non-positive input.
     * 
     * Return value is always in the half open interval [0,m).
     * For any values v,n and m, mod(v,m) == mod(v+ n*m, m) holds.
     * 
     * Template implemented for integral types.
     */
    template <class T, class S>
    auto mod(T v, S m) -> decltype(v%m) {
        if(m == 0){
            return v;
        }
        if(m < 0){
            // flip [m] doesn't change the value.
            return mod(v,-m);
        }
        if(v < 0){
            // flip [v] twice won't change it either, but be careful not to return 
            // [m] instead of 0 when val is a multiple of m.
            if(auto neg_v = mod(-v,m)){ 
                // hidden recursion in if clause will only go one level deep ;)
                return m - neg_v; 
            } else {
                return 0;
            }
        }
        // m positive, v non-negative, yay C++ actually works in that case.
        return v%m;
    }

    template<class T, class S>
    auto min(T l, S r){
        return l < r ? l : r;
    }

    template<class T, class S>
    auto max(T l, S r){
        return l > r ? l : r;
    }
}