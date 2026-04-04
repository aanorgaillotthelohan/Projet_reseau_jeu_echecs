#ifndef CHESS_H
#define CHESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <locale.h>
#include <wchar.h>

/* ─── Liste chaînée des pièces éliminées ─────────────────────────────────── */
typedef struct eliminated_pieces {
    char piece_type;
    struct eliminated_pieces *next;
} eliminated_piece;

typedef struct eliminated_pieces_lists {
    eliminated_piece *start;
} eliminated_pieces_list;

/* ─── Structure joueur ───────────────────────────────────────────────────── */
typedef struct {
    int  socketJoueur;
    char NomJoueur[20];
    char Couleur[20];
    bool rejoinsSalon;
    char message[128];
    int  coupDuTour;
} joueur;

/* ─── Variables globales ─────────────────────────────────────────────────── */
extern int  possible_moves[100];
extern int  possible_moves_index;
extern int  previous_position;
extern int  new_position;
extern char board[8][8];
extern char *piece;
extern eliminated_pieces_list eliminated_pieces_white;
extern eliminated_pieces_list eliminated_pieces_black;

/* ─── Roque : flags de déplacement ──────────────────────────────────────── */
extern bool white_king_moved;
extern bool white_rook_left_moved;
extern bool white_rook_right_moved;
extern bool black_king_moved;
extern bool black_rook_left_moved;
extern bool black_rook_right_moved;

/* ─── Prise en passant ───────────────────────────────────────────────────── */
extern int en_passant_col;
extern int en_passant_row;
extern int en_passant_capture_row;
extern int en_passant_capture_col;

#endif /* CHESS_H */