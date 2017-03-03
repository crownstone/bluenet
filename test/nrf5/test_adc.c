#include "cfg/cs_Boards.h"

int main() {
	// enabled hard float, without it, we get a hardfaults
	SCB->CPACR |= (3UL << 20) | (3UL << 22); __DSB(); __ISB();

	// ADC stuff
	
}
