#ifndef PLAYERS_H
#define PLAYERS_H

#include "chess.h"
#include "moves.h"

/* Création d'un joueur */
joueur *creejoueur(int socket, char NomDuJoueur[20]);

/*
 * parse_coup : décode j->coupDuTour (format entier XYZW)
 *   *src = ligne_départ  * 10 + col_départ
 *   *dst = ligne_arrivée * 10 + col_arrivée
 * Retourne 0 si OK, -1 si invalide.
 */
int parse_coup(joueur *j, int *src, int *dst);

/*
 * Mise à jour des flags prise en passant après un coup.
 */
void update_en_passant(char piece_moved, int r1, int r2, int c2);

/*
 * player_white / player_black
 *   Reçoivent src et dst déjà décodés par parse_coup (appelé dans main).
 *   Responsabilités : calcul des coups légaux, validation, exécution.
 *   Retournent 0 si le coup a été joué, -1 sinon.
 */
int player_white(joueur *j, int src, int dst);
int player_black(joueur *j, int src, int dst);

#endif /* PLAYERS_H */