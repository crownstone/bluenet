class RandomGenerator {
	private:
	uint32_t seed;
	public:
	RandomGenerator(uint32_t seed) : seed(seed) {}
	uint32_t operator()() { return seed++;} // worst random generator ever :)
};
