#ifndef MOVES_H
#define MOVES_H

#include "chess.h"

/* Test de présence */
int  check(int x, int y);    /* Noir en (x,y) ? */
int  check2(int x, int y);   /* Blanc en (x,y) ? */

/* Attaque / échec */
int  is_square_attacked(int r, int c, int attacker_id);
int  is_in_check(int player_id);

/* Validation */
bool verify_possible_move(int position);

/* Promotion */
void promote_pawn(int r, int c, int player_id);

/* Application du coup */
void change(int r1, int c1, int r2, int c2, int player_id);

/* Calcul des coups légaux par pièce */
void pawn(int r1, int c1);
void pawnb(int r1, int c1);
void rook(int r1, int c1, int player_id);
void knight(int r1, int c1, int player_id);
void bishop(int r1, int c1, int player_id);
void king(int r1, int c1, int player_id);
void queen(int r1, int c1, int player_id);

/* Abandon */
void abandon(int player_id);

#endif /* MOVES_H */