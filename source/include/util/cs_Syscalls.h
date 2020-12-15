#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void _exit(int status);
const char* getHeapEnd();
const char* getHeapEndMax();
unsigned long getSbrkNumFails();

#ifdef __cplusplus
}
#endif
