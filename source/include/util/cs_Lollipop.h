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
 *
 * Warning: max == 0, 1 and 255 don't work as expected.
 */
class Lollipop {
private:
	uint8_t val;
	uint8_t max;
public:
	Lollipop(uint8_t m) : val(0), max(m) {}
	Lollipop(uint8_t v, uint8_t m) : val(v), max(m) {}
	Lollipop(const Lollipop& l) = default;

	// strict comparison: this < other
	bool operator<(const Lollipop& other) {
		if (max != other.max) {
			return false;
		}
		if (val == 0) {
			return true;
		}
		if (other.val == 0) {
			return false;
		}

		bool reverse = val > max / 2;

		// TODO: what is m and M ?
		int m = val - (reverse? max / 2 : 0);
		int M = val + (reverse? 0 : max / 2);

		return (m < other.val && other.val <= M) ^ reverse;
	}

	/**
	 * Return the next value given the current value.
	 *
	 * Rolls over at maxValue and skips 0.
	 */
	static uint8_t next(uint8_t currentValue, uint8_t maxValue) {
		return ++currentValue >= maxValue ? 1 : currentValue;
	}

	/**
	 * Returns true when current value is newer than previous value.
	 *
	 * This is always true when previousValue is 0.
	 * This is false when currentValue is 0.
	 */
	static bool isNewer(uint8_t previousValue, uint8_t currentValue, uint8_t maxValue){
		return Lollipop(previousValue, maxValue) < Lollipop(currentValue, maxValue);
	}

	// pre-fix increment operator
	Lollipop& operator++() {
		val = next(val, max);
		return *this;
	}

	// post-fix increment operator
	Lollipop operator++(int) {
		Lollipop copy(*this);
		val = next(val, max);
		return copy;
	}
};
