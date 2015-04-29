
// https://stackoverflow.com/questions/14028076/memory-utilization-for-unwind-support-on-arm-architecture
extern "C" void __wrap___aeabi_unwind_cpp_pr0(void) {

}

// the following leads to a duplicate...
char __aeabi_unwind_cpp_pr0[0];
