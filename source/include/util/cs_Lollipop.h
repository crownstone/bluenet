/**
 * Increment administration object with 'none' value default.
 * Once operator ++ is called, this->val will never be equal to 0 again.
 *
 * Lollipop will roll over after max value.
 *
 *
 * uint8_t max = 10;
 * Lollipop l(max);
 * Lollipop r(max);
 *
 * if r.max != l.max, false is returned.
 * if r.val and l.val  are in range [1,..max], the expression l < r is true iff.
 * r.val is in the range [(l.val+1)%max, ..., (l.val+max/2) %max]
 *
 * else, if l.val == 0, l<r is true
 * else, if r.val == 0, l<r is false.
 *
 * E.g.
 *
 * Lollipop(8,10) < Lollipop(
 */
class Lollipop{
private:
	uint8_t val;
	uint8_t max;
public:
	Lollipop(uint8_t m) : val(0), max(m) {}
	Lollipop(uint8_t v, uint8_t m) : val(v), max(m) {}
	Lollipop(const Lollipop& l) = default;

	// implicit cast operator
	operator uint8_t(){
		return val;
	}

	// strict comparison
	bool operator<(const Lollipop& other){
		if(max != other.max){
			return false;
		}
		if (val == 0) {
			return false;
		}
		if(other.val == 0){
			return false;
		}

		bool reverse = val > max/2;

		auto m = val - (reverse? max/2 : 0);
		auto M = val + (reverse? 0 : max/2);

		return (m < other.val && other.val <= M) ^ reverse;
	}

	/**
	 * uint8_t operator++, but with roll over at m and skipping 0.
	 */
	static uint8_t next(uint8_t v, uint8_t m){
		return ++v >= m ? 1 : v;
	}

	static bool compare(uint8_t v_l, uint8_t v_r, uint8_t m){
		return Lollipop(v_l, m) < Lollipop(v_r, m);
	}

	// pre-fix increment operator
	Lollipop& operator++(){
		val = next(val,max);
		return *this;
	}

	// post-fix increment operator
	Lollipop operator++(int){
		Lollipop copy(*this);
		val = next(val,max);
		return copy;
	}
};
