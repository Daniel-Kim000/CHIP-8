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
    ) == false) {
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
    //printf("Fetched Instruction: %04X \n", instr);
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
            uint8_t pixel = display_buffer[(i * 32) + j];
            if (pixel == 1) {
                SDL_SetRenderDrawColor(state->renderer, 255, 255, 255, 255);   
            }
            else {
                SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255);
            }
        
            SDL_FRect scaled_pixel = {(float)i * 10, (float)j * 10, 10.0f, 10.0f}; // Scale pixel up by 10x since we are on a 10x scale display
            SDL_RenderFillRect(state->renderer, &scaled_pixel);
        }
    }
    /*// TEST: draw a fixed white square, unrelated to CHIP-8 state
    SDL_SetRenderDrawColor(state->renderer, 255, 255, 255, 255);
    SDL_FRect test_rect = {100.0f, 100.0f, 50.0f, 50.0f};
    SDL_RenderFillRect(state->renderer, &test_rect);
    */

    SDL_RenderPresent(state->renderer); // Push backbuffer to display
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) { 
    AppState* state = (AppState*)appstate;
    SDL_Keycode key = (event->key).key;
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    else if (event->type == SDL_EVENT_KEY_DOWN) {
       switch (key) {
            case SDLK_1: update_input_array(1, true); break;
            case SDLK_2: update_input_array(2, true); break;
            case SDLK_3: update_input_array(3, true); break;
            case SDLK_4: update_input_array(12, true); break;

            case SDLK_Q: update_input_array(4, true); break;
            case SDLK_W: update_input_array(5, true); break;
            case SDLK_E: update_input_array(6, true); break;
            case SDLK_R: update_input_array(13, true); break;

            case SDLK_A: update_input_array(7, true); break;
            case SDLK_S: update_input_array(8, true); break;
            case SDLK_D: update_input_array(9, true); break;
            case SDLK_F: update_input_array(14, true); break;

            case SDLK_Z: update_input_array(10, true); break;
            case SDLK_X: update_input_array(0, true); break;
            case SDLK_C: update_input_array(11, true); break;
            case SDLK_V: update_input_array(15, true); break;

       }
    }
    else if (event->type == SDL_EVENT_KEY_UP) {
       switch (key) {
            case SDLK_1: update_input_array(1, false); break;
            case SDLK_2: update_input_array(2, false); break;
            case SDLK_3: update_input_array(3, false); break;
            case SDLK_4: update_input_array(12, false); break;

            case SDLK_Q: update_input_array(4, false); break;
            case SDLK_W: update_input_array(5, false); break;
            case SDLK_E: update_input_array(6, false); break;
            case SDLK_R: update_input_array(13, false); break;

            case SDLK_A: update_input_array(7, false); break;
            case SDLK_S: update_input_array(8, false); break;
            case SDLK_D: update_input_array(9, false); break;
            case SDLK_F: update_input_array(14, false); break;

            case SDLK_Z: update_input_array(10, false); break;
            case SDLK_X: update_input_array(0, false); break;
            case SDLK_C: update_input_array(11, false); break;
            case SDLK_V: update_input_array(15, false); break;

       }
 
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    SDL_free(appstate);
    cleanup();
}
