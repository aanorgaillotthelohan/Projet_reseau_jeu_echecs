#include "players.h"
#include "display.h"

/*
 *  Convention d'affichage :
 *    label affiché 0 = board[7] = pièces blanches  (haut écran)
 *    label affiché 1 = board[6] = pions blancs
 *    label affiché 6 = board[1] = pions noirs
 *    label affiché 7 = board[0] = pièces noires     (bas écran)
 *
 *  Le joueur tape le label qu'il VOIT → on convertit via LABEL_TO_ROW.
 *  possible_moves[] stocke des rangées INTERNES (board[r]).
 *  Donc après conversion src/dst en rangées internes, verify_possible_move
 *  fonctionne directement.
 */
#define LABEL_TO_ROW(l) (7 - (l))

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
    return j;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  parse_coup
 *  Décode j->coupDuTour (entier XYZW, labels affichés) :
 *    *src = label_ligne_src * 10 + col_src   (non converti)
 *    *dst = label_ligne_dst * 10 + col_dst   (non converti)
 *  La conversion LABEL_TO_ROW est faite dans player_white/black.
 *  Retourne 0 si OK, -1 si invalide.
 * ════════════════════════════════════════════════════════════════════════════ */
int parse_coup(joueur *j, int *src, int *dst)
{
    int c = j->coupDuTour;
    int label_src = (c / 1000) % 10;
    int col_src   = (c / 100)  % 10;
    int label_dst = (c / 10)   % 10;
    int col_dst   =  c         % 10;

    if (c < 4
        || label_src < 0 || label_src > 7
        || label_dst < 0 || label_dst > 7
        || col_src   < 0 || col_src   > 7
        || col_dst   < 0 || col_dst   > 7)
        return -1;

    *src = label_src * 10 + col_src;
    *dst = label_dst * 10 + col_dst;
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  parse_pos : décode un entier XY (label affiché) → rangée interne + col
 *  Retourne 0 si OK, -1 si invalide.
 * ════════════════════════════════════════════════════════════════════════════ */
int parse_pos(int pos, int *r, int *c)
{
    int label = pos / 10;
    int col   = pos % 10;
    if (label < 0 || label > 7 || col < 0 || col > 7) return -1;
    *r = LABEL_TO_ROW(label);
    *c = col;
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  update_en_passant
 * ════════════════════════════════════════════════════════════════════════════ */
void update_en_passant(char piece_moved, int r1, int r2, int c2)
{
    if      (piece_moved == 'P' && r1 == 6 && r2 == 4)
        { en_passant_col = c2; en_passant_row = 4; }
    else if (piece_moved == 'p' && r1 == 1 && r2 == 3)
        { en_passant_col = c2; en_passant_row = 3; }
    else
        { en_passant_col = -1; en_passant_row = -1; }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  compute_moves_white
 *  Calcule les coups légaux pour la pièce blanche en (r1,c1) (rangée interne).
 *  Retourne 0 si OK, -1 si pas de pièce blanche / aucun coup légal.
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
    case 'A': abandon(1);         return 0;
    default:
        wprintf(L"\u26A0 Pas de piece blanche en ligne %d col %d\n",
                LABEL_TO_ROW(r1), c1);
        return -1;
    }
    if (possible_moves_index < 1) {
        wprintf(L"\u26A0 Aucun coup legal pour cette piece\n");
        return -1;
    }
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  compute_moves_black
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
    case 'A': abandon(2);         return 0;
    default:
        wprintf(L"\u26A0 Pas de piece noire en ligne %d col %d\n",
                LABEL_TO_ROW(r1), c1);
        return -1;
    }
    if (possible_moves_index < 1) {
        wprintf(L"\u26A0 Aucun coup legal pour cette piece\n");
        return -1;
    }
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  player_white
 *  Reçoit src et dst en LABELS AFFICHÉS (ex: src=11 → label 1, col 1).
 *  Convertit en rangées internes, valide, exécute.
 *  Retourne 0 si coup joué, -1 sinon (tour non consommé).
 * ════════════════════════════════════════════════════════════════════════════ */
int player_white(joueur *j, int src, int dst)
{
    (void)j;

    if (is_in_check(1))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI BLANC ! \u26A0\u26A0\n");

    /* Conversion label → rangée interne */
    int r1, c1, r2, c2;
    if (parse_pos(src, &r1, &c1) < 0) {
        wprintf(L"\u26A0 Position source invalide : %d\n", src);
        return -1;
    }
    if (parse_pos(dst, &r2, &c2) < 0) {
        wprintf(L"\u26A0 Position destination invalide : %d\n", dst);
        return -1;
    }

    char piece_moved = board[r1][c1];

    /* Calcul des coups légaux (déjà fait dans main, mais on re-valide dst) */
    if (verify_possible_move(r2 * 10 + c2)) {
        wprintf(L"\u26A0 Destination %d invalide\n", dst);
        return -1;
    }

    /* Exécution */
    previous_position = r1 * 10 + c1;
    new_position      = r2 * 10 + c2;
    change(r1, c1, r2, c2, 1);
    update_en_passant(piece_moved, r1, r2, c2);

    wprintf(L"\u2705 Blanc joue : %d \u2192 %d\n", src, dst);

    if (is_in_check(2))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI NOIR ! \u26A0\u26A0\n");

    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  player_black
 * ════════════════════════════════════════════════════════════════════════════ */
int player_black(joueur *j, int src, int dst)
{
    (void)j;

    if (is_in_check(2))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI NOIR ! \u26A0\u26A0\n");

    int r1, c1, r2, c2;
    if (parse_pos(src, &r1, &c1) < 0) {
        wprintf(L"\u26A0 Position source invalide : %d\n", src);
        return -1;
    }
    if (parse_pos(dst, &r2, &c2) < 0) {
        wprintf(L"\u26A0 Position destination invalide : %d\n", dst);
        return -1;
    }

    char piece_moved = board[r1][c1];

    if (verify_possible_move(r2 * 10 + c2)) {
        wprintf(L"\u26A0 Destination %d invalide\n", dst);
        return -1;
    }

    previous_position = r1 * 10 + c1;
    new_position      = r2 * 10 + c2;
    change(r1, c1, r2, c2, 2);
    update_en_passant(piece_moved, r1, r2, c2);

    wprintf(L"\u2705 Noir joue : %d \u2192 %d\n", src, dst);

    if (is_in_check(1))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI BLANC ! \u26A0\u26A0\n");

    return 0;
}