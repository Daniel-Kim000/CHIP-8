#define SDL_MAIN_USE_CALLBACKS 1
#include "chip-8.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
/*
int main(int argc, char* argv[]) {
	setup_state();
	if (open_rom("test") == -1) { // the "test" parameter is temporary
		return -1;
	}
	bool running = true;
	while (running) {
		uint16_t instr = fetch();
		printf("Fetched Instruction: %04X \n", instr);
		increment_pc();		
		decode_and_exec(instr);
		running = false; // only here for now
	}
	return 0;
}
*/

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
} AppState;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) { 
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD) == false) {
        return SDL_APP_FAILURE;
    }
    AppState* state = SDL_calloc(1, sizeof(AppState)); // Use SDL_calloc for cross-platform compatibility
    if (state == NULL) {
        return SDL_APP_FAILURE;
    }
    *appstate = state; // state persists across callback functions

    if (SDL_CreateWindowAndRenderer(
            "CHIP-8 Interpreter",
            640, // The display window is normally 64 x 32, but for the sake of this project, it will be upscaled by 10x
            320,
            SDL_WINDOW_RESIZABLE,
            &state->window,
            &state->renderer
    ) == NULL) {
        return SDL_APP_FAILURE;
    }
    setup_state();
    if (open_rom("test") == -1) { // TODO: "test" is a temporary parameter, will be changed later
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

// Runs once every frame
SDL_AppResult SDL_AppIterate(void* appstate) {
    AppState* state = (AppState*)appstate;
    uint16_t instr = fetch();
    printf("Fetched Instruction: %04X \n", instr);
    increment_pc();		
    decode_and_exec(instr);
    
    /*SDL_Texture* texture = SDL_CreateTexture(
        state->renderer
    );*/

    SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255); // Set backbuffer color to black
    SDL_RenderClear(state->renderer);   // Clear screen with the backbuffer (do we need this?)
    // Draw/render calls go here
    /* Ideas for drawing to the screen:
     * Since we're scaling up the display by 10x, maybe use SDL_RenderFillRect to create solid squares that represent individual pixels?
     * SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255) = set draw color to black
     * SDL_SetRenderDrawColor(state->renderer, 255, 255, 255, 255) = set draw color to white
     * Consider SDL_RenderFillRects if performance is a concern
     * Algorithm idea:
        * Call function from chip8.c that returns the display buffer (or just get it somehow)
        * For each entry, if it's equal to 1, set that pixel's 10x10 area to white using RenderFillRect; otherwise, set it to black
    */
    uint8_t* display_buffer = get_display_buffer();
    for (int j = 0; j < 32; j++) {
        for (int i = 0; i < 64; i++) {
            uint8_t pixel = display_buffer[(i * 64) + j];
            if (pixel == 1) {
                SDL_SetRenderDrawColor(state->renderer, 255, 255, 255, 255);   
            }
            else {
                SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
            }
            SDL_Rect scaled_pixel = {i * 10, j * 10, 10, 10}; // Scale pixel up by 10x since we are on a 10x scale display
            SDL_RenderFillRect(state->renderer, &scaled_pixel);
        }
    }

    SDL_RenderPresent(state->renderer); // Push backbuffer to display
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    SDL_free(appstate);
    cleanup();
}
