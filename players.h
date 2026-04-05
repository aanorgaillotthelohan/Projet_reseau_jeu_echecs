#ifndef PLAYERS_H
#define PLAYERS_H

#include "chess.h"
#include "moves.h"

joueur *creejoueur(int socket, char NomDuJoueur[20]);

/*
 * parse_coup : décode j->coupDuTour (XYZW, labels affichés)
 *   *src = label_src*10 + col_src
 *   *dst = label_dst*10 + col_dst
 * Retourne 0 si OK, -1 si invalide.
 */
int parse_coup(joueur *j, int *src, int *dst);

/*
 * parse_pos : décode un entier XY (label affiché) en rangée interne + col.
 * Retourne 0 si OK, -1 si invalide.
 */
int parse_pos(int pos, int *r, int *c);

/* Mise à jour flags prise en passant */
void update_en_passant(char piece_moved, int r1, int r2, int c2);

/*
 * compute_moves_white / compute_moves_black
 * Calcule les coups légaux pour la pièce en (r1,c1) RANGÉE INTERNE.
 * Remplit possible_moves[]. Retourne 0 si OK, -1 si pas de pièce/coup.
 */
int compute_moves_white(int r1, int c1);
int compute_moves_black(int r1, int c1);

/*
 * player_white / player_black
 * src et dst en LABELS AFFICHÉS.
 * Retourne 0 si coup joué, -1 sinon (tour non consommé).
 */
int player_white(joueur *j, int src, int dst);
int player_black(joueur *j, int src, int dst);

#endif /* PLAYERS_H */