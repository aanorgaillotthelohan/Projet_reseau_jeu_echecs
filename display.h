#ifndef DISPLAY_H
#define DISPLAY_H

#include "chess.h"

int  display_convert(char symbol);
void display(void);
void display_possible_moves(char b[8][8]);
void intro(void);
void append_possible_moves(void);
void remove_possible_moves(void);

#endif /* DISPLAY_H */