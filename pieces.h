#ifndef PIECES_H
#define PIECES_H

#include "chess.h"

eliminated_piece *create_piece(char value);
void insert_eliminated_list(char value, eliminated_pieces_list *l);
void display_eliminated_list(eliminated_pieces_list l);

#endif /* PIECES_H */