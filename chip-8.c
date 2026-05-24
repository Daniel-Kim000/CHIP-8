#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>

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
	printf("Read %zu chars\n", chars_read);
	// Read the first 32 bytes of the ROM (if needed)
	for (int i = 0; i < 32 && i < chars_read; i++) {
		printf("%02x ", buffer[i]);
		if ((i + 1) % 16 == 0) printf("\n");  // New line every 16 bytes
	}
	printf("\n");
	memcpy(&c8_state.memory[0x200], buffer, rom_size);
	//printf("%02x\n", c8_state.memory[0x200]); // print the first byte

	free(buffer);
	fclose(rom);
	return 0;
}
