#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
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
pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
bool timer_thread_running;
pthread_t timer_thread;

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
	timer_thread_running = true;
	pthread_create(&timer_thread, NULL, timer_thread_function, NULL);
	return 0;
}
// Sets up SDL
/*void SDL_setup() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD) == false) {
        printf("SDL Error: %s\n", SDL_GetError());
    }

    // Take a closer look at making the SDL_Window. somehow, this needs to be destroyed in cleanup(); find a way to pass the memory address to the window.
    SDL_Window = SDL_CreateWindow(
            "CHIP-8 Interpreter",
            640, // The display window is normally 64 x 32, but for the sake of this project, it will be upscaled by 10x
            320,
            SDL_WINDOW_RESIZABLE
    );
    if (SDL_Window == NULL) {
        printf("SDL Error: %s\n", SDL_GetError());
    }
}*/

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
	printf("\nFetched Rom Instructions: \nRead from left to right, going downwards: \n");
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
	printf("\n\n");
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

void decrement_pc() {
	c8_state.PC = c8_state.PC - 2;
}

// Thread function for decrementing the sound and delay timer
/* Simulating a 60 Hz Timer:
	sleep the thread for 16.67 milliseconds (roughly the same as 60 Hz)
	lock mutex
	decrement both timers if > 0 and beep if needed
	unlock mutex
	repeat!
*/
void* timer_thread_function(void* arg) {
	while (timer_thread_running) {
		usleep(16670); // 16670 microseconds = 16.67 milliseconds
		pthread_mutex_lock(&timer_mutex);
		if (c8_state.sound_timer > 0) {
			c8_state.sound_timer--;	
		}
		if (c8_state.delay_timer > 0) {
			c8_state.delay_timer--;
			// TODO: implement beep
		}
		pthread_mutex_unlock(&timer_mutex);
	}
	return NULL;
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
uint16_t decode_and_exec(uint16_t instr) {
	uint8_t X = (instr & 0x0F00) >> 8;
	uint8_t Y = (instr & 0x00F0) >> 4;
	uint8_t N = (instr & 0x000F);
	uint8_t NN = (instr & 0x00FF);
	uint16_t NNN = (instr & 0x0FFF);

	switch (instr >> 12) {
		case 0x0: // Clear or return from subroutine
		  if (instr == 0x00E0) {
		  	// Zero out all values in the display buffer
		  	// printf("Clear screen instruction \n");
			for (int i = 0; i < 64; i++) {
			  for (int j = 0; j < 32; j++) {
			    c8_state.display_buffer[i][j] = 0;
			  }
			}
		  }
		  else if (instr == 0x00EE) {
		  	// Reduce the SP by 1 and then pop the last address from the stack and set that to PC
		  	//printf("Return from subroutine instruction \n");
			c8_state.SP = c8_state.SP - 1;
			c8_state.PC = c8_state.stack[c8_state.SP];
		  }
		  else {
		  	printf("Unknown instruction \n");
		  }
		  break;
		case 0x1: // Jump: 1NNN
		  // Set PC = NNN
		  // printf("Jump instruction \n");
		  c8_state.PC = NNN;
		  break;
		case 0x2: // Call subroutine: 2NNN
		  // Push PC to the stack and then set PC = NNN
		  //printf("Call subroutine instruction \n");
		  c8_state.stack[c8_state.SP] = c8_state.PC;
		  c8_state.SP = c8_state.SP + 1;
		  c8_state.PC = NNN;
		  break;

		case 0x3: // Skip conditionally: 3XNN
		  // Skip if VX == NN
		  //printf("Skip conditionally instruction (0x3XNN) \n");
		  if (c8_state.V_regs[X] == NN) {
		  	increment_pc();
		  }
		  break;
		case 0x4: // Skip conditionally: 4XNN
		  // Skip if VX != NN
		  //printf("Skip condtionally instruction (0x4XNN) \n");
		  if (c8_state.V_regs[X] != NN) {
		  	increment_pc();
		  }
		  break;
		case 0x5: // Skip conditionally: 5XY0
		  // Skip if VX == VY
		  //printf("Skip conditionally instruction (0x5XY0) \n");
		  if (c8_state.V_regs[X] == c8_state.V_regs[Y]) {
		  	increment_pc();
		  }
		  break;
		case 0x9: // Skip conditionally: 9XY0
		  // Skip if VX != VY
		  //printf("Skip conditionally instruction (0x9XY0) \n");
		  if (c8_state.V_regs[X] != c8_state.V_regs[Y]) {
	  	  	increment_pc();
		  }
		  break;

		case 0x6: // Set
		  // Sets the value at VX to the value NN
		  // printf("Set instruction with NN \n");
		  c8_state.V_regs[X] = NN;
		  break;
		case 0x7: // Add
		  // Adds the value at VX with NN
		  // printf("Add instruction with NN \n");
		  c8_state.V_regs[X] += NN;
		  break;
		case 0x8: // Logical/Arithmetic
		  switch (N) {
		  	case 0: // 8XY0
			  //printf("Set instruction with X and Y \n");
			  c8_state.V_regs[X] = c8_state.V_regs[Y];
			  break;
			case 1: // 8XY1
			  //printf("Binary OR instruction \n");
			  c8_state.V_regs[X] = c8_state.V_regs[X] | c8_state.V_regs[Y];
			  break;
			case 2: // 8XY2
			  //printf("Binary AND instruction \n");
			  c8_state.V_regs[X] = c8_state.V_regs[X] & c8_state.V_regs[Y];
			  break;
			case 3:	// 8XY3
			  //printf("Binary XOR instruction \n");
			  c8_state.V_regs[X] = c8_state.V_regs[X] ^ c8_state.V_regs[Y];
			  break;
			case 4: // 8XY4
			  // If the result of adding these two registers exceeds the 8-bit integer limit of 255, set the flag register to 1
			  //printf("Add instruction with X and Y \n");
			  int result = c8_state.V_regs[X] + c8_state.V_regs[Y];
			  if (result > 255) { 
			  	c8_state.V_regs[15] = 1;
			  }
			  else {
			  	c8_state.V_regs[15] = 0;
			  }
			  c8_state.V_regs[X] = c8_state.V_regs[X] + c8_state.V_regs[Y];
			  break;

			case 5: // 8XY5
			  // VX = VX - VY; if VX >= VY, VF = 1; VF = 0 otherwise
			  //printf("Subtract instruction: VX = VX - VY \n");
			  if (c8_state.V_regs[X] >= c8_state.V_regs[Y]) {
			  	c8_state.V_regs[15] = 1;
			  }
			  else {
			  	c8_state.V_regs[15] = 0;
			  }
			  c8_state.V_regs[X] = c8_state.V_regs[X] - c8_state.V_regs[Y];
			  break;
			case 7: // 8XY7
			  // VX = VY - VX; if VY >= VX, VF = 1; VF = 0 otherwise
			  //printf("Subtract instruction: VX = VY - VX \n");
			  if (c8_state.V_regs[Y] >= c8_state.V_regs[X]) {
			  	c8_state.V_regs[15] = 1;
			  }
			  else {
			  	c8_state.V_regs[15] = 0;
			  }
			  c8_state.V_regs[X] = c8_state.V_regs[Y] - c8_state.V_regs[X];
			  break;
			/* Ambiguous instructions: */
			// Maybe let the user choose what kind of setting to use?
			// On the original COSMAC VIP, these instructions did: VX = VY and then shifted VX by 1 bit to the right/left; VF would be set to the bit that was shifted out
			// On CHIP-48 and SUPER-CHIP, these instructions shifted VX in place and ignored VY completely
			// This program will default to the modern behavior, since most ROMs made after the 90s use the modern behavior
			case 6: // 8XY6
			  //printf("Shift right instruction \n");
			  c8_state.V_regs[15] = c8_state.V_regs[X] & 1; // put the least significant bit into VF
			  c8_state.V_regs[X] = c8_state.V_regs[X] >> 1;
			  break; 
			case 0xE: // 8XYE
			  //printf("Shift left instruction \n");
			  c8_state.V_regs[15] = (c8_state.V_regs[X] >> 7) & 1; // put the most significcant bit into VF
			  c8_state.V_regs[X] = c8_state.V_regs[X] << 1;
			  break;
		  }
		  break;
		case 0xA:
		  // Sets the index register I to the value NNN
		  // printf("Set index instruction \n");
		  c8_state.I_reg = NNN;
		  break;

		/* Ambiguous instruction: */
		case 0xB: // BNNN (or BXNN)
		  //printf("Jump with offset instruction \n");
		  // On the original COSMAC VIP, this instruction jumped to address NNN + value in V0
		  // In CHIP-48 and SUPER-CHIP, this address becomes BXNN; this instruction jumps to address XNN + value in VX
		  // For the sake of simplicity, this insntruction will default to the original COSMAC VIP implementation
		  c8_state.PC = NNN + c8_state.V_regs[0];
		  break;

		case 0xC: // CXNN
		  //printf("Random number generator instruction \n");
		  // Generate a random number and then binary AND it with the value NN and put the result in VX
		  srand(time(NULL)); // Seed the RNG based on the current time
		  int random = rand() % 256; // Generate random number from 0 - 255 
		  c8_state.V_regs[X] = random & NN; // this is done to limit the range of random numbers if desired
		  break;
		case 0xD: // DXYN
		  /*
		     Draws a sprite N pixels tall.
		     The sprite is read from starting at index register I.
		     Draws the sprite at screen coordinates (V[X], V[Y]).
		     Each sprite pixel is XOR'd with the display buffer
		     Set VF = 1 if any pixels became turned off - collision detection
		     Be sure to allow coordinates to wrap: an X coordinate of 4 is equal to an X coordinate of 68 (because 68 % 64 = 4)
		     The sprite should not wrap around if it goes beyond the border, it should just be clipped
		  */
		  // printf("Display control instruction \n");
		  uint8_t X_coord = c8_state.V_regs[X] % 64;
		  uint8_t Y_coord = c8_state.V_regs[Y] % 32;
		  c8_state.V_regs[15] = 0; // Set the VF flag to 0; doing so will make it obvious if it was raised in this instruction
		  for (int i = 0; i < N; i++) {  // Fetch the sprite data from memory[I + 0] to memory[I + N-1]
			uint8_t sprite_row = c8_state.memory[c8_state.I_reg + i]; // Each sprite is 8 bits wide - every bit of this variable counts!
			for (int j = 0; j < 8; j++) { // Iterate over all 8 bits of a row of the sprite
				uint8_t sprite_pixel = (sprite_row >> (7 - j)) & 1; // extract a bit of this sprite, from left to right
				if (X_coord + j >= 64) { // If the desired row is going to exceed the length of the display, stop drawing this row
					break;
				}
				else if ((sprite_pixel == 1) && c8_state.display_buffer[X_coord + j][Y_coord + i] == 1) { // If the the current pixel in the sprite row is on and the pixel at (X,Y) in the display buffer is on
					c8_state.display_buffer[X_coord + j][Y_coord + i] = 0;
					c8_state.V_regs[15] = 1;
				}
				else if (sprite_pixel == 1) { // If the current pixel in the sprite row is on and the pixel at (X,Y) in the display buffer is off
					c8_state.display_buffer[X_coord + j][Y_coord + i] = 1;
				}
		  	}
			if (Y_coord + i >= 32) { // If the next sprite row is going to exceed the height of the display, stop drawing the sprite entirely
				break;
			}
		  }
		  break;
		case 0xE:
		  uint8_t key = c8_state.V_regs[X] & 0x0F; // Done so we can make sure that whatever the keypress is, its some number from 0 to 15
		  switch (NN) {
			case 0x9E: // EX9E
			  // Skip the next instruction if the key corresponding to VX's value is pressed
			  //printf("Skip instruction if key corresponding to VX's value is pressed");
			  if (c8_state.input[key]) {
			  	increment_pc();
			  }
			  break;
			case 0xA1: // EXA1
			  //printf("Skip instruction if key corresponding to VX's value is not pressed");
			  // Skip the next instruction if the key corresponding to VX's value is not pressed
			  if (c8_state.input[key] == 0) {
			  	increment_pc();
			  }
			  break;
		  }
		  break;
		case 0xF:
		  switch (NN) {
		  	/* Timer manipulation instructions */
			// For each manipulation or read of the delay timer, the timer_mutex is locked before it and unlocked after
		  	case 0x07: // FX07
			  //printf("Set VX to the current value of the delay timer");
			  // Sets VX to the current value of the delay timer
			  pthread_mutex_lock(&timer_mutex);
			  c8_state.V_regs[X] = c8_state.delay_timer;
			  pthread_mutex_unlock(&timer_mutex);
			  break;
			case 0x15: // FX15
			  //printf("Set the delay timer to the value in VX");
			  // Sets the delay timer to the value in VX
			  pthread_mutex_lock(&timer_mutex);
			  c8_state.delay_timer = c8_state.V_regs[X];
			  pthread_mutex_unlock(&timer_mutex);
			  break;
			case 0x18: // FX18
			  //printf("Set the sound timer to the value in VX");
			  // Sets the sound timer to the value in VX
			  pthread_mutex_lock(&timer_mutex);
			  c8_state.sound_timer = c8_state.V_regs[X];
			  pthread_mutex_unlock(&timer_mutex);
			  break;

			case 0x1E: // FX1E
			  //printf("Add to index instruction");
			  // Index register I has VX added to it
			  // Optional: Set VF to 1 if index register I overflows from 0xFFF to above 1000; though this is not the behavior on the COSMAC VIP, many interpreters do this.
			  c8_state.I_reg = c8_state.I_reg + c8_state.V_regs[X];
			  break;
			case 0x0A: // FX0A
			  //printf("Get key instruction");
			  // Blocking instruction, waits for key input. Decrement PC unless a key is pressed. Store which key was pressed in V[X]
			  // Can simulate a block by not letting the PC go past this instruction.
			  // Note: on the COSMAC VIP, the key was only registered when it was pressed and then released.
			  decrement_pc(); // Assume that no key is pressed
			  for (int i = 0; i < 16; i++) { // Check if any key is pressed
				if (c8_state.input[i]) {
					c8_state.V_regs[X] = i;
					increment_pc(); // Since a key is pressed, we can allow the instruction to pass
					break;
				}
			  }
			  break;
			case 0x29: // FX29
			  //printf("Font character instruction");
			  // Index register I is set to the memory address of the font sprite for the hex digit that is stored in V[X]
			  // Each font sprite is 5 bytes and the font starts from 0x050
			  uint8_t digit = c8_state.V_regs[X] & 0x0F; // Ensure that the extracted digit is from 0 - 15
			  c8_state.I_reg = (5 * digit) + 0x050;
			  break;
			case 0x33: // FX33
			  // Take the number in V[X] (which would be a number from 0 - 255 because its a uint8_t) and converts it to three decimal digits that are placed in memory contiguously, from I.
			  /* Example: V[X] = 123
			    123 / 100 = 1       -> memory[I]
			    (123 / 10) % 10 = 2 -> memory[I + 1]
			    123 % 10 = 3        -> memory[I + 2]
			  */
			  //printf("Binary-coded decimal conversion instruction");
			  c8_state.memory[c8_state.I_reg] = c8_state.V_regs[X] / 100;
			  c8_state.memory[c8_state.I_reg + 1] = (c8_state.V_regs[X] / 10) % 10;
			  c8_state.memory[c8_state.I_reg + 2] = c8_state.V_regs[X] % 10;
			  break;
			
			/* Ambiguous instructions */
			case 0x55: // FX55
			  // Store the value of each variable register from V0 to VX inclusive in contiguous memory addresses starting from I.
			  //printf("Store instruction");
			  for (int i = 0; i <= X; i++) {
			  	c8_state.memory[c8_state.I_reg + i] = c8_state.V_regs[i];
			  }
			  break;
			case 0x65: // FX65 - Different behavior on the COSMAC VIP
			  // Read values from memory and load them into variable registers from V0 to VX inclusive
			  //printf("Load instruction");
			  for (int i = 0; i <= X; i++) {
			  	c8_state.V_regs[i] = c8_state.memory[c8_state.I_reg + i];
			  }
			  break;
		  }
		  break;
		default:
		  printf("Unknown instruction: %04X\n, instr");
	}
	return 0;	
}


// Cleans up anything when the emulator is to be closed
int cleanup() {
	/* Threading Cleanup: */
	timer_thread_running = false;
	
	pthread_join(timer_thread, NULL); // Waits for timer_thread to finish whatever its doing
	int result = pthread_mutex_destroy(&timer_mutex);
	if (result != 0) {
		return 1;	
    }

    return 0;
}
