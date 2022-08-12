/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Nov 29, 2020
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#include <math.h>

/**
 * Compute mean and variance of a running measurement without keeping track of
 * all data points using this aggregation algorithm.
 *
 * https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
 * https://www.tandfonline.com/doi/abs/10.1080/00401706.1962.10490022
 *
 * Notes:
 * - no optimization for catastrophic cancellation implemented
 * - no overflow protection implemented
 *
 */
class VarianceAggregator {
private:
	// running values
	uint32_t num_recorded_values                           = 0;
	float M2                                               = 0.0f;
	float mean                                             = 0.0f;

	// these values are just estimated to be reasonable.
	// reduceCount is called if M2 or num_recorded_values surpass
	// these thresholds.
	static const constexpr float float_precision_threshold = 10e9f;
	static const constexpr int count_precision_threshold   = 10e6;

public:
	/**
	 * update the aggregated data with a new measurement.
	 */
	void addValue(float new_measurement) {
		if (isNumericPrecisionLow()) {
			reduceCount();
		}

		float diff_with_old_mean = new_measurement - mean;

		num_recorded_values++;
		mean += diff_with_old_mean / num_recorded_values;

		float diff_with_new_mean = new_measurement - mean;

		M2 += diff_with_old_mean * diff_with_new_mean;
	}

	int getCount() const { return num_recorded_values; }

	float getMean() const { return mean; }

	float getVariance() const { return M2 / (num_recorded_values - 1); }

	float getStandardDeviation() const { return sqrt(getVariance()); }

	bool isNumericPrecisionLow() const {
		return M2 > float_precision_threshold || num_recorded_values > count_precision_threshold;
	}

	/**
	 * Reduce |M2| and num_recorded_values to prevent overflow.
	 * The factors have been chosen to ensure that if we have:
	 *   float v0 = getVariance();
	 *   reduceCount();
	 *   float v1 = getVariance();
	 * Then the absolute difference |v0-v1| is minimal (0 if no rounding error occurs).
	 *
	 * Since the mean converges to the actual mean of the measurements,
	 * no correction is necessary for that member value.
	 */
	void reduceCount() {
		M2 /= (2 + 2 / (num_recorded_values - 2));
		num_recorded_values /= 2;
	}

	void reset() {
		num_recorded_values = 0;
		M2                  = 0.0f;
		mean                = 0.0f;
	}
};
