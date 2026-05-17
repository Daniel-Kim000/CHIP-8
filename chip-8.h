#ifndef CHIP_8_H
#define CHIP_8_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>

/* A struct that has values for the entire hardware state of the CHIP-8 */
typedef struct {
	uint8_t memory[4096]; 		// 4KB of memory
	uint8_t V_regs[16]; 		// V0 - VF are general purpose registers; VF is also a flag register
	uint16_t I_reg; 		// Register used for memory addressing
	uint16_t PC;			// Program Counter
	uint16_t SP;			// Stack Pointer
	uint16_t stack[16]; 		// Stack has space for 16 entries of 16-bit addresses
	uint8_t delay_timer; 		// General purpose countdown timer
	uint8_t sound_timer; 		// Countdown timer that also triggers a beep as long as its > 0
	//uint8_t font[80];		// Preloaded Font
	uint8_t display_buffer[64][32]; // 64 by 32 display buffer; when instructions draw sprites, the display buffer is modified. Then this buffer is rendered.
	
	uint8_t input[16];		// Input Detection - Represented as 1 or 0 based on if its pressed or not. 
					// When SDL (or any equivalent graphics framework) detects a keypress, the associated index in this array is set to 1.
					// When CHIP-8 instructions check for input, they read from this input array.
} chip_8_state_t;

// Sets up some initial values in the state
int setup_state();
#endif
