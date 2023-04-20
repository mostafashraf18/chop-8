#include "chip8.h"

/**
 * @file: chip8.c
 * @author: Mostafa Ashraf
 * @Date: 19-4-2023
 * 
 * Discussion:
 *
 * im trying to implemnt the spcefications of the chip8
 * xx Fetch
 * xx Decode
 * xx Execute
 */

#define FONT_ADDR 0x00
#define FONT_BYTES 5


unsigned char chip8_fonts[80] {
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

uint16_t opcode; //ist
uint8_t  memory[MEMORY_SIZE];
uint8_t V[16]; // 16- 8 bit genral  purpose registor
uint16_t I;  //16-bit index register
uint16_t PC; // points at the current instruction memory
uint8_t  gfx[G_ROW][G_COL]; //screen [0xF00]
uint8_t  delay_timer;
uint8_t  sound_timer;
uint16_t stack[STACK_SIZE];
uint16_t SP; //8-bit stack pointer
uint8_t  key[KEY_SIZE];


void chip8_init(){

	int i;
	PC = 0x200; // starts after the interpeter memory
	opcode = 0;
	I = 0;
	SP = 0;

	memset(memory, 0, sizeof(uint8_t) * MEMORY_SIZE);
	memset(V, 0, sizeof(uint8_t) * 16);
	memset(gfx, 0, sizeof(uint8_t) * GFX_SIZE);
	memset(stack, 0, sizeof(uint8_t) * STACK_SIZE);
	memset(key, 0, sizeof(uint8_t) * KEY_SIZE);

	for(i = 0; i < 80; i++) {
	  memory[FONT_ADDR + i] = chip8_fonts[i];
	}

        delay_timer = 0;
	sound_timer = 0;
	srand(time(NULL));
}

void load() {
  FILE *fgame;

  fgame = fopen(game, "rb");

  // Debug test
  if(NULL == fgame) {
    fprintf(stderr, "Unable to open game: %s\n", game);
    exit(42);
  }

  fread(&memory[0x200], 1, MAX_SIZE, fgame);

  fclose(fgame);

}

void emulate_cycle() {

  int i;
  uint8_t x, y, n;
  uint8_t kk;

}
