#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>

// CHIP-8 Emulator ^^
// Notes:
// Use SDL when doing graphical stuff and detecting keypresses.

/* Initializations */
int memory[4096] = {0}; // 4KB of memory
struct registers = { // Looooots of registers. V0-VF are general purpose.
	uint8_t V0;
	uint8_t V1;
	uint8_t V2;
	uint8_t V3;
	uint8_t V4;
	uint8_t V5;
	uint8_t V6;
	uint8_t V7;
	uint8_t V8;
	uint8_t V9;
	uint8_t VA;
	uint8_t VB;
	uint8_t VC;
	uint8_t VD;
	uint8_t VE;
	uint8_t VF; // general register and a flag register
	uint16_t I; // register used for memory addressing
	};

uint16_t PC = 0x200; // PC starts at 0x200
uint16_t SP = 0x0; // SP starts at 0
uint16_t stack[16] = {0}; // Stack has space for 16 entries for 16-bit addresses

// Timers should be decremented by one 60 times per second (i.e. at 60Hz).
uint8_t delay_timer = 0; // General purpose countdown timer
uint8_t sound_timer = 0; // Countdown timer that also triggers a beep as long as its > 0

// (Code for loading font data to memory goes here)


// Display Buffer - When instructions draw sprites, this buffer is modified. Then this buffer is rendered.
int display_buffer[64][32] = {0};

// Input Detection - Represented as 1 or 0 based on if its pressed or not
// When SDL (or any equivalent graphics framework) detects a keypress, the associated index in this array is set to 1.
// When CHIP-8 instructions check for input, they read from this array.
int input[16] = {0};


