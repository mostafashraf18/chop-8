/**
 * @file : chip8.h
 * @author: Mostafa Ashraf
 *
 * all time used things
 */

#ifndef __CHIP8_H__
#define __CHIP8_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define MEMORY_SIZE 4096
#define G_COL 32
#define G_ROW 64
#define GFX_SIZE (G_ROW * G_COL)
#define STACK_SIZE 16
#define KEY_SIZE 16

#define MAX_SIZE (0x1000-0x200)

void chip8_init();
void load(char *game);
void emulate_cycle();
void setkeys();
void tick();

#endif
