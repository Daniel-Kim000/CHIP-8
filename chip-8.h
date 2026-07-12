#ifndef CHIP_8_H
#define CHIP_8_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

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


// Loads a rom based on a file_name in the ./tests/ directory
int open_rom(char file_name[]);


// Increments the PC by 2 since on each fetch, 2 values are read
void increment_pc();	

void decrement_pc();

// Thread function for decrementing the sound and delay timer
/* Simulating a 60 Hz Timer:
	sleep the thread for 16.67 milliseconds (roughly the same as 60 Hz)
	lock mutex
	decrement both timers if > 0 and beep if needed
	unlock mutex
	repeat!
*/
void* timer_thread_function(void* arg);


// Fetches memory[PC] and memory[PC + 1] to create a full, 16-bit instruction
uint16_t fetch();

// Decodes a given instruction
/*
  Generally, CHIP-8 instructions are divided into broad categories based on the first "nibble" of the instruction, or the first hexadecimal number. 

  Meanings for the other hex numbers of the instruction:
    X: The second hex number - used to look up one of the 16 registers (VX) from V0 through VF. (remember to use this number JUST to look up the register number, and not as a value itself!)
    Y: The third hex number - used to look up one of the 16 registers (VX) from V0 through VF.  (see above note)
    N: The fourth hex number - a 4-bit number.
    NN: The third and fourth hex number - a 8-bit number used in immediates.
    NNN: The second, third, and fourth hex number - a 12-bit number used in immediate memory addressing.

*/
uint16_t decode_and_exec(uint16_t instr); 

// Cleans up anything when the emulator is to be closed
int cleanup();
#endif
