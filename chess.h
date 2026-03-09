#ifndef CHESS_H
#define CHESS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <locale.h>
#include <wchar.h>

// ─── Linked list for eliminated pieces ───────────────────────────────────────
typedef struct eliminated_pieces {
    char piece_type;
    struct eliminated_pieces *next;
} eliminated_piece;

typedef struct eliminated_pieces_lists {
    eliminated_piece *start;
} eliminated_pieces_list;

// ─── Global state ─────────────────────────────────────────────────────────────
extern int  possible_moves[100];
extern int  possible_moves_index;
extern int  previous_position;
extern int  new_position;
extern char board[8][8];
extern char *piece;
extern eliminated_pieces_list eliminated_pieces_white;
extern eliminated_pieces_list eliminated_pieces_black;

// ─── Roque : flags de déplacement ────────────────────────────────────────────
extern bool white_king_moved;
extern bool white_rook_left_moved;
extern bool white_rook_right_moved;
extern bool black_king_moved;
extern bool black_rook_left_moved;
extern bool black_rook_right_moved;

// ─── Prise en passant ─────────────────────────────────────────────────────────
// Colonne/rangée du pion adverse pouvant être pris en passant (-1 = aucun)
extern int en_passant_col;
extern int en_passant_row;
// Case du pion à supprimer après une prise en passant (-1 = aucune)
extern int en_passant_capture_row;
extern int en_passant_capture_col;

// ─── Utility (platform) ───────────────────────────────────────────────────────
char getch_linux(void);           // Remplace getch() de conio.h
void delay(int number_of_seconds);

// ─── Display ──────────────────────────────────────────────────────────────────
int  display_convert(char symbol);
void display(void);
void display_possible_moves(char b[8][8]);
void intro(void);
void append_possible_moves(void);
void remove_possible_moves(void);

// ─── Eliminated pieces list ───────────────────────────────────────────────────
eliminated_piece *create_piece(char value);
void insert_eliminated_list(char value, eliminated_pieces_list *l);
void display_eliminated_list(eliminated_pieces_list l);

// ─── Move logic ───────────────────────────────────────────────────────────────
void change(int r1, int c1, int r2, int c2, int player_id);
void promote_pawn(int r, int c, int player_id);
void pawn(int r1, int c1);
void pawnb(int r1, int c1);
void rook(int r1, int c1, int player_id);
void knight(int r1, int c1, int player_id);
void bishop(int r1, int c1, int player_id);
void king(int r1, int c1, int player_id);
void queen(int r1, int c1, int player_id);
bool verify_possible_move(int position);

// Renvoie 1 si la case (r,c) est attaquée par le camp attacker_id
int  is_square_attacked(int r, int c, int attacker_id);
// Renvoie 1 si le roi du camp player_id est en échec
int  is_in_check(int player_id);

// ─── Players ──────────────────────────────────────────────────────────────────
void player_white(void);
void player_black(void);
int  check(int x, int y);
int  check2(int x, int y);

// ─── Showcase ─────────────────────────────────────────────────────────────────
void showcase_game(void);
void showcase_board(char row, char col, char new_row, char new_col, int id);

#endif /* CHESS_H */
