/*
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Okt 18, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */
#include <algorithm>

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

    /**
     * Represents an interval by two unsigned integers [base, base + diff].
     * (base + diff doesn't have to be representable in the current type,
     * the interval represented will wrap around to 0)
     */
    template<class T, class S = T>
    class Interval{
        private: 
        T low,high;
        public:
        Interval(T base, S diff) : low(base), high(base+diff) {
            // notes:
            // - addition base+diff is allowed to overflow/underflow.
            // - having a different type S for diff allows negative 
            // values without conflicting type resolutions.
            if(diff < 0){
                std::swap(low,high);
            }
        } 

        // E.g.
        // Interval<uint8_t> i(200,00)
        // i.contains(0) == true.
        bool contains(T val){
            return
                low < high // reversed interval?
                ? (low <= val && val < high)  // nope, both must hold
                : (low <= val || val < high); // yup, only one can hold

        }
    };
}