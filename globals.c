#include "chess.h"

// ─── Global variable definitions ──────────────────────────────────────────────
int  possible_moves[100];
int  possible_moves_index = 0;
int  previous_position    = 0;
int  new_position         = -99;
char *piece               = NULL;

eliminated_pieces_list eliminated_pieces_white = { NULL };
eliminated_pieces_list eliminated_pieces_black = { NULL };

// ─── Roque ────────────────────────────────────────────────────────────────────
// Blanc = rangée 7 (bas de l'affichage), Noir = rangée 0 (haut)
bool white_king_moved        = false;
bool white_rook_left_moved   = false;
bool white_rook_right_moved  = false;
bool black_king_moved        = false;
bool black_rook_left_moved   = false;
bool black_rook_right_moved  = false;

// ─── Prise en passant ─────────────────────────────────────────────────────────
int en_passant_col          = -1;
int en_passant_row          = -1;
int en_passant_capture_row  = -1;
int en_passant_capture_col  = -1;

//  Rangée 0 = haut de l'écran = Noir  (minuscules)
//  Rangée 7 = bas  de l'écran = Blanc (majuscules)
//  Les Blancs avancent vers les rangées décroissantes (vers 0)
//  Les Noirs  avancent vers les rangées croissantes  (vers 7)
char board[8][8] = {
    { 'r', 'h', 'c', 'q', 'k', 'c', 'h', 'r' },  // rangée 0 : pièces noires
    { 'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p' },  // rangée 1 : pions noirs
    { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
    { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
    { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
    { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
    { 'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P' },  // rangée 6 : pions blancs
    { 'R', 'H', 'C', 'Q', 'K', 'C', 'H', 'R' },  // rangée 7 : pièces blanches
};