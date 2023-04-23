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

#define unknown_opcode(op) \
    do { \
        fprintf(stderr, "Unknown opcode: 0x%x\n", op); \
        fprintf(stderr, "kk: 0x%02x\n", kk); \
        exit(42); \
    } while (0)


#ifdef DEBUG
#define p(...) printf(__VA_ARGS__);
#else
#define p(...)
#endif


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

void draw_sprite(uint8_t x, uint8_t y, uint8_t n) {
  unsigned row  =y , col = x;
  unsigned byte_index;
  unsigned bit_index;
  V[0xF] = 0;
  for(byte_index = 0; byte_index < n; byte_index++) {
    uint8_t byte = memory[I + byte_index];
    for(bit_index = 0; bit_index < 8; bit_index++) {
      uint8_t bit = (byte >> bit_index) & 0x1;
      uint8_t *pixelp = &gfx[(rox + byte_index)% G_ROW][(col + (7- bit_index)) % G_COL];

      //if drawing to the screen would cause any pixel to be erased
      //set the collision flag to 1
      if ( bit == 1 && *pixelp == 1) V[0xF] = 1;

      //draw this  pixel  by XOR
      *pixelp = * pixelp ^ bit;
    }
  }
}

  
  
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

	chip8_draw_flag = true;
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
  uint16_t nnn;

  //fetch
  opcode = memory[PC] << 8 | memory[PC + 1];
  x = (opcode >> 8) & 0x000F;
  y = (opcode >> 4) & 0x000F;
  n = opcode & 0x000F;
  kk = opcode & 0x00FF;
  nnn = opcode & 0x0FFF;

  #ifdef DEBUG 
    printf("PC: 0x%04x Op: 0x%04x\n", PC, opcode);
  #endif

    //decode & execute: case on the highest order byte
    switch(opcode & 0xF000) {
    case 0x0000:
      switch (kk) {
      case 0x00E0: // clear the screen
	P("Clear the screen\n");
	memset(gfx, 0, sizeof(uint8_t) * GFX_SIZE);
	 chip8_draw_flag = true;
           PC += 2;
           break;
      case 0x00EE:
	p("ret\n");
	PC = stack[--SP];
	break;
      default:
	unknown_opcode(opcode);
      }
    
    break;
    case 0x1000: // jump to address nnn
      p("Jump to address 0x%x\n", nnn);
      PC = nnn;
      break;
    case 0x2000: // call the address to nnn
	p("Call address 0x%x\n", nnn);
	stack[SP++] = PC + 2;
        PC = nnn;
	break;
    case 0x3000: //3xkk: skip next instr if V[x] = kk
      p("skip next instruction if 0x%x = 0x%x\n", V[x], kk);
	  PC += (V[x] == kk) ?  4 : 2;
	break;
    case 0x4000: //4xkk: skip next instr if V[x] != kk
      p("skip next instruction if 0x%x != 0x%x\n", V[x], kk];
      PC +=  (V[x] != kk) ? 4 : 2;
      break;
    case 0x5000: // skip next instr if V[x] == V[y]
      p("skip next instruction if 0x%x == 0x%x\n", V[X], V[y]);
      PC += (V[x] == V[x]) ? 4 : 2;
      break;
    case 0x6000: //set v[x] = kk
      p("set V[0x%x] to 0x%x\n", x, kk);
      V[x] = kk;
      PC += 2;
      break;
    case 0x7000: // set V[x] = V[x] +kk
      p("set V[0x%d] to V[0x%d] + 0x%x\n", x,x, kk);
      V[x] += kk;
      PC += 2;
      break;
    case 0x8000: //Arithmetic stuff
      switch(n) {
      case 0x0:
	p("V[0x%x] = V[0x%x] = 0x%x\n", x,y, V[y]);
	V[x] =V[y];
	break;
      case 0x1:
	p("V[0x%x] |= V[0x%x] = 0x%x\n", x, y, V[y]);
	V[x] = V[x] | V[y];
	break;
      case 0x2:
	p("V[0x%x] &= V[0x%x] = 0x%x\n", x, y, V[y]);
	V[x] =V[x] & V[y];
	break;
      case 0x3:
	p("V[0x%x] ^= V[0x%x] = 0x%x\n", x, y, V[y]);
	V[x] = V[x] ^ V[y];
	break;
      case 0x4:
	p("V[0x%x] = V[0x%x] + V[0x%x] = 0x%x + 0x%x\n", x, x, y, V[x], V[y]);
	V[0xF] = ((int) V[x] + (int) V[y]) > 255 ? 1 : 0;
	V[x] = V[x] + V[y];
	break;
      case 0x5:
	p("V[0x%x] = V[0x%x] - V[0x%x] = 0x%x - 0x%x\n", x, x, y, V[x], V[y]);
	V[0xF] = (V[x] > V[y]) ? 1 : 0;
	V[x] = V[x] - V[y];
	break;
      case 0x6:
	 p("V[0x%x] = V[0x%x] >> 1 = 0x%x >> 1\n", x, x, V[x]);
         V[0xF] = V[x] & 0x1;
         V[x] = (V[x] >> 1);
        break;
      case 0x7:
	p("
		    





    

}
