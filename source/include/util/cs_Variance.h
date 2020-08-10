
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

#include <math.h>

class OnlineVarianceRecorder {
private:
	// running values
	uint32_t num_recorded_values = 0;
	float M2 = 0.0f;
	float mean = 0.0f;

public:
	/**
	 * update the aggregated data with a new measurement.
	 */
	void addValue(float new_measurement){
		float diff_with_old_mean = new_measurement - mean;

		num_recorded_values++;
		mean += diff_with_old_mean / num_recorded_values;

		float diff_with_new_mean = new_measurement - mean;

		M2 += diff_with_old_mean * diff_with_new_mean;
	}

	int getCount(){
		return num_recorded_values;
	}

	float getMean(){
		return mean;
	}

	float getVariance(){
		return M2 / (num_recorded_values -1);
	}

	float getStandardDeviation(){
		return sqrt(getVariance());
	}

	void reduceCount(){
		// suggestion:
		M2 /= 2;
		num_recorded_values /= 2;
	}
};
