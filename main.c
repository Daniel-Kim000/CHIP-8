#include "chip-8.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

int main(void) {
	setup_state();
	if (open_rom("test") == -1) { // the "test" parameter is temporary
		return -1;
	}
	bool running = true;
	while (running) {
		uint16_t instr = fetch();
		printf("Fetched Instruction: %04X \n", instr);
		increment_pc();		
		decode(instr);
		running = false; // only here for now
	}
	return 0;
}
