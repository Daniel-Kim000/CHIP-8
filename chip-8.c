#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>

#include "chip-8.h"

// CHIP-8 Interpreter ^^
// Notes:
// Use SDL when doing graphical stuff and detecting keypresses.
// Timers should be decremented by one 60 times per second (i.e. at 60Hz).
//
// Input Detection - Represented as 1 or 0 based on if its pressed or not
// 	When SDL (or any equivalent graphics framework) detects a keypress, the associated index in this array is set to 1.
// 	When CHIP-8 instructions check for input, they read from the input array.
//
// Display Buffer - When instructions draw sprites, the display buffer is modified. Then this buffer is rendered.


static chip_8_state_t c8_state = {0};

/* Sets up some values in the state */
int setup_state() {
	c8_state.PC = 0x200; // PC starts at 0x200
	c8_state.SP = 0x0; // SP starts at 0

	// Font Data:
	// Each hexadecimal digit is a 8 x 5 sprite.
	// Consider a "0": 
	// ****      (binary: 11110000 = 0xF0)
	// *  *      (binary: 10010000 = 0x90)
	// *  *      (binary: 10010000 = 0x90)
	// *  *      (binary: 10010000 = 0x90)
	// ****      (binary: 11110000 = 0xF0)
	// In CHIP-8, the font data is provided by the interpreter. This will be loaded into memory at addresses 0x050 to 0x09F.
	uint8_t font_data[80] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};
	memcpy(&c8_state.memory[0x050], font_data, 80); // Copy the entire font data into memory, starting at 0x050.
	return 0;
}

/* Opens a .ch8 file based on a given file_name */
// might wanna make this a bit more secure down the line since strings in C are a bit interesting...
int open_rom(char file_name[]) {
	// For the time being, only open up /tests/1-chip8-logo.ch8
	
	FILE* rom = fopen("./tests/1-chip8-logo.ch8", "rb");
	if (rom == NULL) {
		printf("File read error");
	}
	fseek(rom, 0L, SEEK_END);
	long rom_size = ftell(rom); // size in bytes
	fseek(rom, 0L, SEEK_SET);
	printf("Rom size in bytes: %ld\n", rom_size);
	uint8_t* buffer = malloc(rom_size);
	if (buffer == NULL) {
		printf("Error on malloc");
		return -1;
	}
	size_t chars_read = fread(buffer, 1, rom_size, rom);
	printf("Read %zu chars from the file\n", chars_read);
	
	// Display the instructions of the ROM (if needed)
	printf("Read from left to right, going downwards: \n");
	int cols = 0;
	for (int i = 0; i < rom_size && i < chars_read; i += 2) {
		uint16_t instr = (buffer[i] << 8) | (buffer[i + 1]);
		printf("%04X ", instr);
		cols += 1;
		if (cols == 8) {
			printf("\n");
			cols = 0;
		}
	}
	printf("\n");
	memcpy(&c8_state.memory[0x200], buffer, rom_size);
	//printf("%02x\n", c8_state.memory[0x200]); // print the first byte

	free(buffer);
	fclose(rom);
	return 0;
}

// Increments the PC by 2 since on each fetch, 2 values are read
void increment_pc() {	
	c8_state.PC = c8_state.PC + 2; 
}

// Fetches memory[PC] and memory[PC + 1] to create a full, 16-bit instruction
uint16_t fetch() {
	// Each instruction is 2 bytes (16 bits), so we will need to fetch two entries from memory
	uint16_t instr = (c8_state.memory[c8_state.PC] << 8) | (c8_state.memory[c8_state.PC + 1]); // shift left by 8 bits to make room for both fetches from memory
	return instr;
	//Display the fetched instruction (if needed)
	//printf("PC = 0x%03X\n", c8_state.PC);
	//printf("memory[PC] = 0x%02X\n", c8_state.memory[c8_state.PC]);
	//printf("memory[PC+1] = 0x%02X\n", c8_state.memory[c8_state.PC + 1]);
	//uint16_t instr = (c8_state.memory[c8_state.PC] << 8) | 
                 //(c8_state.memory[c8_state.PC + 1]);
		 //printf("Fetched instruction = 0x%04X\n", instr);

}

// Decodes a given instruction
/*
  Generally, CHIP-8 instructions are divided into broad categories based on the first hexadecimal value of the instruction.

  Meanings for the other hex numbers of the instruction:
    X: The second hex number - used to look up one of the 16 registers (VX) from V0 through VF. (remember to use this number JUST to look up the register number, and not as a value itself!)
    Y: The third hex number - used to look up one of the 16 registers (VX) from V0 through VF.  (see above note)
    N: The fourth hex number - a 4-bit number.
    NN: The third and fourth hex number - a 8-bit number used in immediates.
    NNN: The second, third, and fourth hex number - a 12-bit number used in immediate memory addressing.

*/
uint16_t decode(uint16_t instr) {
	uint8_t X = (instr & 0x0F00) >> 8;
	uint8_t Y = (instr & 0x00F0) >> 4;
	uint8_t N = (instr & 0x000F);
	uint8_t NN = (instr & 0x00FF);
	uint16_t NNN = (instr & 0x0FFF);

	switch (instr >> 12) {
		case 0x0: // Clear or return from subroutine
		  if (instr == 0x00E0) {
		  	printf("Clear screen instruction \n");
		  }
		  else if (instr == 0x00EE) {
		  	printf("Return from subroutine instruction \n");
		  }
		  else {
		  	printf("Unknown instruction \n");
		  }
		  break;
		case 0x1: // Jump 
		  printf("Jump instruction \n");
		  break;
		case 0x2: // Call subroutine
		  printf("Call subroutine instruction \n");
		  break;

		case 0x3: // Skip conditionally
		  printf("Skip conditionally instruction (0x3XNN) \n");
		  break;
		case 0x4: // Skip conditionally
		  printf("Skip condtionally instruction (0x4XNN) \n");
		  break;
		case 0x5: // Skip conditionally
		  printf("Skip conditionally instruction (0x5XY0) \n");
		  break;
		case 0x9: // Skip conditionally
		  printf("Skip conditionally instruction (0x9XY0) \n");
		  break;

		case 0x6: // Set
		  printf("Set instruction with NN \n");
		  break;
		case 0x7: // Add
		  printf("Add instruction with NN \n");
		  break;
		case 0x8: // Logical/Arithmetic
		  switch (N) {
		  	case 0:
			  printf("Set instruction with X and Y \n");
			  break;
			case 1:
			  printf("Binary OR instruction \n");
			  break;
			case 2:
			  printf("Binary AND instruction \n");
			  break;
			case 3:		
			  printf("Logical XOR instruction \n");
			  break;
			case 4:
			  printf("Add instruction with X and Y \n");
			  break;

			case 5:
			  printf("Subtract instruction: VX = VX - VY \n");
			  break;
			case 7:
			  printf("Subtract instruction: VX = VY - VX \n");
			  break;
			/* Ambiguous instructions: */
			case 6: // look more into this later
			  printf("Shift instruction \n");
			  break;
			case 0xE: // look more into this later
			  printf("Shift instruction \n");
			  break;
		  }
		  break;
		case 0xA:
		  printf("Set index instruction \n");
		  break;
		/* Ambiguous instruction: */
		case 0xB:
		  printf("Jump with offset instruction \n");
		  break;
		case 0xC:
		  printf("Random number generator instruction \n");
		  break;
		case 0xD:
		  printf("Display control instruction \n");
		  break;
		case 0xE:
		  switch (NN) {
			case 0x9E:
			  printf("Skip instruction if key corresponding to VX's value is pressed");
			  break;
			case 0xA1:
			  printf("Skip instruction if key corresponding to VX's value is not pressed");
			  break;
		  }
		  break;
		case 0xF:
		  switch (NN) {
		  	/* Timer manipulation instructions */
		  	case 0x07:
			  printf("Set VX to the current value of the delay timer");
			  break;
			case 0x15:
			  printf("Set the delay timer to the value in VX");
			  break;
			case 0x18:
			  printf("Set the sound timer to the value in VX");
			  break;

			case 0x1E:
			  printf("Add to index instruction");
			  break;
			case 0x0A:
			  printf("Get key instruction");
			  break;
			case 0x29:
			  printf("Font character instruction");
			  break;
			case 0x33:
			  printf("Binary-coded decimal conversion instruction");
			  break;
			
			/* Ambiguous instructions */
			case 0x55:
			  printf("Store instruction");
			  break;
			case 0x65:
			  printf("Load instruction");
			  break;
		  }
		  break;
		default:
		  printf("Unknown instruction: %04X \n, instr");
	}
	return 0;	
}
