#include "chess.h"
#include "players.h"
#include <stdlib.h>

#define LABEL_TO_ROW(l) (7-(l))
#define ROW_TO_LABEL(r) (7-(r))

/* ════════════════════════════════════════════════════════════════════════════
 *  creejoueur
 * ════════════════════════════════════════════════════════════════════════════ */
joueur *creejoueur(int socket, char NomDuJoueur[20])
{
    joueur *j = malloc(sizeof(joueur));
    if (j == NULL) return NULL;
    strncpy(j->NomJoueur, NomDuJoueur, 20);
    j->socketJoueur = socket;
    strncpy(j->Couleur, "blanc", 20);
    j->rejoinsSalon = false;
    j->coupDuTour   = 0;
    return j;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  update_en_passant  (non-static : exportée via players.h)
 * ════════════════════════════════════════════════════════════════════════════ */
void update_en_passant(char piece_moved, int r1, int r2, int c2)
{
    if      (piece_moved == 'P' && r1 == 6 && r2 == 4) { en_passant_col = c2; en_passant_row = 4; }
    else if (piece_moved == 'p' && r1 == 1 && r2 == 3) { en_passant_col = c2; en_passant_row = 3; }
    else { en_passant_col = -1; en_passant_row = -1; }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  parse_coup : décode j->coupDuTour (entier XYZW)
 *    src = label_src*10 + col_src
 *    dst = label_dst*10 + col_dst
 * ════════════════════════════════════════════════════════════════════════════ */
int parse_coup(joueur *j, int *src, int *dst)
{
    int c = j->coupDuTour;
    int ligne_dprt    = (c / 1000) % 10;
    int col_dprt      = (c / 100)  % 10;
    int ligne_arrivee = (c / 10)   % 10;
    int col_arrivee   =  c         % 10;

    if (c < 4
        || ligne_dprt    < 0 || ligne_dprt    > 7
        || ligne_arrivee < 0 || ligne_arrivee > 7
        || col_dprt      < 0 || col_dprt      > 7
        || col_arrivee   < 0 || col_arrivee   > 7)
        return -1;

    *src = ligne_dprt    * 10 + col_dprt;
    *dst = ligne_arrivee * 10 + col_arrivee;
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  parse_pos : décode un entier XY (label affiché, coordonnées saisies par
 *  le client) en rangée INTERNE et colonne.
 *  XY = label * 10 + col   (ex: 63 → label 6, col 3 → rangée interne 1)
 * ════════════════════════════════════════════════════════════════════════════ */
int parse_pos(int pos, int *r, int *c)
{
    if (pos < 0 || pos > 77 || pos % 10 > 7)
        return -1;
    int label = pos / 10;
    int col   = pos % 10;
    if (label < 0 || label > 7 || col < 0 || col > 7)
        return -1;
    *r = LABEL_TO_ROW(label);   /* conversion label → rangée interne */
    *c = col;
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  compute_moves_white(r1, c1)  —  rangée INTERNE
 *  Remplit possible_moves[], retourne 0 si OK, -1 si pas de pièce blanche.
 * ════════════════════════════════════════════════════════════════════════════ */
int compute_moves_white(int r1, int c1)
{
    switch (board[r1][c1]) {
    case 'P': pawn(r1, c1);      break;
    case 'R': rook(r1, c1, 1);   break;
    case 'H': knight(r1, c1, 1); break;
    case 'C': bishop(r1, c1, 1); break;
    case 'K': king(r1, c1, 1);   break;
    case 'Q': queen(r1, c1, 1);  break;
    default:  return -1;
    }
    if (possible_moves_index < 1) return -1;
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  compute_moves_black(r1, c1)  —  rangée INTERNE
 * ════════════════════════════════════════════════════════════════════════════ */
int compute_moves_black(int r1, int c1)
{
    switch (board[r1][c1]) {
    case 'p': pawnb(r1, c1);     break;
    case 'r': rook(r1, c1, 2);   break;
    case 'h': knight(r1, c1, 2); break;
    case 'c': bishop(r1, c1, 2); break;
    case 'k': king(r1, c1, 2);   break;
    case 'q': queen(r1, c1, 2);  break;
    default:  return -1;
    }
    if (possible_moves_index < 1) return -1;
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  player_white(j, src, dst)
 *  src, dst en LABELS AFFICHÉS  (ex: 63, 54)
 *  Retourne 0 si coup joué, -1 sinon.
 * ════════════════════════════════════════════════════════════════════════════ */
int player_white(joueur *j, int src, int dst)
{
    int r1, c1, r2, c2;

    if (is_in_check(1))
        printf("\n⚠⚠ ECHEC AU ROI BLANC ! ⚠⚠\n");

    printf("\n⚪ Blanc joue ... coup recu : \"%d%d\"\n", src, dst);

    if (parse_pos(src, &r1, &c1) < 0) {
        printf("⚠ Position source invalide : %d\n", src);
        return -1;
    }

    char piece_moved = board[r1][c1];

    if (compute_moves_white(r1, c1) < 0) {
        printf("⚠ Pas de piece blanche en %d ou aucun coup legal\n", src);
        return -1;
    }

    /* destination : le client envoie aussi en label */
    if (parse_pos(dst, &r2, &c2) < 0) {
        printf("⚠ Destination invalide : %d\n", dst);
        return -1;
    }

    if (verify_possible_move(r2 * 10 + c2)) {
        printf("⚠ Destination %d invalide\n", dst);
        return -1;
    }

    previous_position = r1 * 10 + c1;
    new_position      = r2 * 10 + c2;
    change(r1, c1, r2, c2, 1);
    update_en_passant(piece_moved, r1, r2, c2);

    printf("✅ Blanc a joue : %d -> %d\n", src, dst);

    if (is_in_check(2))
        printf("\n⚠⚠ ECHEC AU ROI NOIR ! ⚠⚠\n");

    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  player_black(j, src, dst)
 *  src, dst en LABELS AFFICHÉS
 * ════════════════════════════════════════════════════════════════════════════ */
int player_black(joueur *j, int src, int dst)
{
    int r1, c1, r2, c2;

    if (is_in_check(2))
        printf("\n⚠⚠ ECHEC AU ROI NOIR ! ⚠⚠\n");

    printf("\n⚫ Noir joue ... coup recu : \"%d%d\"\n", src, dst);

    if (parse_pos(src, &r1, &c1) < 0) {
        printf("⚠ Position source invalide : %d\n", src);
        return -1;
    }

    char piece_moved = board[r1][c1];

    if (compute_moves_black(r1, c1) < 0) {
        printf("⚠ Pas de piece noire en %d ou aucun coup legal\n", src);
        return -1;
    }

    if (parse_pos(dst, &r2, &c2) < 0) {
        printf("⚠ Destination invalide : %d\n", dst);
        return -1;
    }

    if (verify_possible_move(r2 * 10 + c2)) {
        printf("⚠ Destination %d invalide\n", dst);
        return -1;
    }

    previous_position = r1 * 10 + c1;
    new_position      = r2 * 10 + c2;
    change(r1, c1, r2, c2, 2);
    update_en_passant(piece_moved, r1, r2, c2);

    printf("✅ Noir a joue : %d -> %d\n", src, dst);

    if (is_in_check(1))
        printf("\n⚠⚠ ECHEC AU ROI BLANC ! ⚠⚠\n");

    return 0;
}