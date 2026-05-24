#include "chip-8.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <stdint.h>

int main(void) {
	if (open_rom("test") == -1) { // the "test" parameter is temporary
		return -1;
	}
	return 0;
}
