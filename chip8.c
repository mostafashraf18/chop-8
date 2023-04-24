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
  /*
   *Dxyn - DRW Vx, Vy, nibble Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
   */
  unsigned row  =y , col = x;
  unsigned byte_index;
  unsigned bit_index;
  // set to collision
  V[0xF] = 0;
  //The interpreter reads n bytes from memory
  for(byte_index = 0; byte_index < n; byte_index++) {
    uint8_t byte = memory[I + byte_index];
    
    for(bit_index = 0; bit_index < 8; bit_index++) {
      uint8_t bit = (byte >> bit_index) & 0x1;
      uint8_t *pixelp = &gfx[(rox + byte_index)% G_ROW][(col + (7- bit_index)) % G_COL];

      //if drawing to the screen would cause any pixel to be erased
      //set the collision flag to 1
      if ( bit == 1 && *pixelp == 1)
	V[0xF] = 1;

      //Sprites are XORed onto the existing screen
      //draw this  pixel  by XOR
      *pixelp = * pixelp ^ bit;
    }
  }
}

static void debug_draw() {
    int x, y;

    for (y = 0; y < G_ROw; y++) {
        for (x = 0; x < G_COL; x++) {
            if (gfx[y][x] == 0) printf("0");
            else printf(" ");
        }
        printf("\n");
    }
    printf("\n");
}


//debug THE REGISTORS
static void print_state() {
    printf("------------------------------------------------------------------\n");
    printf("\n");

    printf("V0: 0x%02x  V4: 0x%02x  V8: 0x%02x  VC: 0x%02x\n", 
           V[0], V[4], V[8], V[12]);
    printf("V1: 0x%02x  V5: 0x%02x  V9: 0x%02x  VD: 0x%02x\n",
           V[1], V[5], V[9], V[13]);
    printf("V2: 0x%02x  V6: 0x%02x  VA: 0x%02x  VE: 0x%02x\n",
           V[2], V[6], V[10], V[14]);
    printf("V3: 0x%02x  V7: 0x%02x  VB: 0x%02x  VF: 0x%02x\n",
           V[3], V[7], V[11], V[15]);

    printf("\n");
    printf("PC: 0x%04x\n", PC);
    printf("\n");
    printf("\n");
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
	p("V[0x%x] = V[0x%x] - V[0x%x] = 0x%x - 0x%x\n", x, x, y, V[x],V[y]);
	V[0xF] = (  V[y] > V[x]) ? 1 : 0;
	V[x] = V[x] - V[y];
	break;
      case 0xE:
	p("V[0x%x] = V[0x%x] << 1 = 0x%x << 1\n", x, x, V[x]);
	V[0xF] = (V[x] >> 7) & 0x1;
	V[x] = (V[x] << 1);
	break;
      default:
	unknown_opcode(opcode);
      }
      PC += 2;
      break;
    case 0x9000: // skip instructions if Vx != Vy
      switch (n) {
      case 0x0:
	p("skip next instruction if 0x%x != 0x%x\n", V[x]. V[y]);
	PC += (V[x] != V[y]) ? 4 : 2;
	break;
      default:
	unknown_opcode(opcode);
      }
      break;
    case 0xA000: // set I to address nnn
      p("set I to 0x %x\n", nnn);
      I = nnn;
      PC += 2;
      break;
    case 0xB000: // jump to location nnn + V[0]
      p("jump to 0x%x + V[0] (0x%x)\n", nnn, V[0]);
      PC = nnn + V[0] ;
      break;
    case 0xC000: //V[x] = random Byte AND kk
      p("V[0%x] = rand Byte\n", x);
      V[x] = (rand() % 256) & kk;
      PC += 2;
      break;
    case 0xD000: // Display an n-byte sprite starting ar memory
      //location I at (Vx, Vy) on the screen, VF = collision
      p("Draw sprite at (V[0x%x], V[0x%x]) = (0x%x, 0x%x) of height %d", x, y, V[x], V[y], n);
      draw_sprite(V[x], V[y],n);
      PC += 2;
      chip8_draw_flag = true;
      break;
    case 0xE000: // Skip next instruction if key with the value of Vx is pressed
      switch(kk) {
      case 0x9E: // skip next if key[Vx] is pressed
	p("Skip next instruction if key[%d] is pressed\n", x);
	PC += (key[V[x]]) ? 4 : 2;
	break;
      case 0xA1: //Skip next instruction if key with the value of Vx is not pressed.
	p("Skip next instruction if key[%d] is NOT pressed\n", x);
	PC += (!key[V[x]]) ? 4 : 2;
                    break;
                default:
                    unknown_opcode(opcode);
      }
      break;
    case 0xF000: // the misc
      switch (kk) {
      case 0x07: // sets VX to the current value of the delay timer
	p("V[0x%x] = delay timer = %d\n", x, delay_timer);
	V[x] = delay_timer;
	 PC += 2;
         break;
      case 0x0A: //Wait for a key press, store the value of the key in Vx.
                    i = 0;
                    printf("Wait for key instruction\n");
                    while (true) {
                        for (i = 0; i < KEY_SIZE; i++) {
                            if (key[i]) {
                                V[x] = i;
                                goto got_key_press;
                            }
                        }
                    }
                    got_key_press:
                    PC += 2;
                    break; 
      case 0x15:
	 p("delay timer = V[0x%x] = %d\n", x, V[x]);
         delay_timer = V[x];
         PC += 2;
         break;
      case 0x18:
	 p("sound timer = V[0x%x] = %d\n", x, V[x]);
         sound_timer = V[x];
         PC += 2;
         break;
      case 0x1E: //Set I = I + Vx
	p(" Set I  = 0x%x + 0x%x\n", I, V[x]);
	V[0xF] = (I + V[x] > 0xfff) ? 1 : 0;
        I = I + V[x];
        PC += 2;
        break;
      case 0x29: //Set I = location of sprite for digit Vx
	p(" Set I = Location of V[0x%x] = 0x%x\n", x, V[x]);
	I = 5 * V[x];
	PC += 2;
	break;
      case 0x33:
	p("Store BCD for %d starting at address 0x%x\n", V[x], I);
	 memory[I] = (V[x] % 1000) / 100; // hundred's digit
         memory[I+1] = (V[x] % 100) / 10;   // ten's digit
         memory[I+2] = (V[x] % 10);         // one's digit
         PC += 2;
         break;
      case 0x55:
	 p("Copy sprite from registers 0 to 0x%x into memory at address 0x%x\n", x, I);
         for (i = 0; i <= x; i++) { memory[I + i] = V[i]; }
         I += x + 1;
         PC += 2;
         break;
      case 0x65:
	 p("Copy sprite from memory at address 0x%x into registers 0 to 0x%x\n", x, I);
         for (i = 0; i <= x; i++) { V[i] = memory[I + i]; }
         I += x + 1;
         PC += 2;
	 break;
      default:
	unknown_opcode(opcode);
      }
      break;
    default:
      unknown_opcode(opcode);
  }

    #ifdef DEBUG
    print_state();
    #endif

}

void tick() {
  if (delay_timer > 0) {
        --delay_timer;
    }
    if (sound_timer > 0) {
        --sound_timer;
        if (sound_timer == 0) {
            printf("BEEP!\n");
        }
    }
}
