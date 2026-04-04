#include "players.h"
#include "display.h"

#define LABEL_TO_ROW(l) (7-(l))

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
 *  Décode j->coupDuTour (entier XYZW) :
 *    *src = ligne_départ  * 10 + col_départ
 *    *dst = ligne_arrivée * 10 + col_arrivée
 *  Retourne 0 si OK, -1 si invalide.
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
 *  update_en_passant
 * ════════════════════════════════════════════════════════════════════════════ */
void update_en_passant(char piece_moved, int r1, int r2, int c2)
{
    if      (piece_moved=='P' && r1==6 && r2==4) { en_passant_col=c2; en_passant_row=4; }
    else if (piece_moved=='p' && r1==1 && r2==3) { en_passant_col=c2; en_passant_row=3; }
    else { en_passant_col=-1; en_passant_row=-1; }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  player_white  –  Validation + exécution pour le camp blanc.
 *  Reçoit src et dst déjà décodés par parse_coup() (appelé dans main).
 *  Retourne 0 si le coup a été joué, -1 sinon.
 * ════════════════════════════════════════════════════════════════════════════ */
int player_white(joueur *j, int src, int dst)
{
    (void)j;
    int r1      = src / 10;
    int c1      = src % 10;
    char piece_moved = board[r1][c1];

    /* ── Vérification échec ── */
    if (is_in_check(1))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI BLANC ! \u26A0\u26A0\n");

    /* ── Valider la pièce source ── */
    if (r1 < 0 || r1 > 7 || c1 < 0 || c1 > 7) {
        wprintf(L"\u26A0 Position source invalide : %d\n", src);
        return -1;
    }

    /* ── Calcul des coups légaux ── */
    switch (board[r1][c1]) {
    case 'P': pawn(r1,c1);      break;
    case 'R': rook(r1,c1,1);    break;
    case 'H': knight(r1,c1,1);  break;
    case 'C': bishop(r1,c1,1);  break;
    case 'K': king(r1,c1,1);    break;
    case 'Q': queen(r1,c1,1);   break;
    case 'A': abandon(1);        return 0;
    default:
        wprintf(L"\u26A0 Pas de piece blanche en %d\n", src);
        return -1;
    }

    if (possible_moves_index < 1) {
        wprintf(L"\u26A0 Aucun coup legal pour la piece en %d\n", src);
        return -1;
    }

    /* ── Valider la destination ── */
    int r2 = LABEL_TO_ROW(dst / 10);
    int c2 = dst % 10;

    if (verify_possible_move(r2 * 10 + c2)) {
        wprintf(L"\u26A0 Destination %d invalide\n", dst);
        return -1;
    }

    /* ── Exécution ── */
    previous_position = r1 * 10 + c1;
    new_position      = r2 * 10 + c2;
    change(r1, c1, r2, c2, 1);
    update_en_passant(piece_moved, r1, r2, c2);

    wprintf(L"\u2705 Blanc a joue : %d -> %d\n", src, dst);

    if (is_in_check(2))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI NOIR ! \u26A0\u26A0\n");

    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  player_black  –  Validation + exécution pour le camp noir.
 *  Retourne 0 si le coup a été joué, -1 sinon.
 * ════════════════════════════════════════════════════════════════════════════ */
int player_black(joueur *j, int src, int dst)
{
    (void)j;
    int r1      = src / 10;
    int c1      = src % 10;
    char piece_moved = board[r1][c1];

    /* ── Vérification échec ── */
    if (is_in_check(2))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI NOIR ! \u26A0\u26A0\n");

    /* ── Valider la pièce source ── */
    if (r1 < 0 || r1 > 7 || c1 < 0 || c1 > 7) {
        wprintf(L"\u26A0 Position source invalide : %d\n", src);
        return -1;
    }

    /* ── Calcul des coups légaux ── */
    switch (board[r1][c1]) {
    case 'p': pawnb(r1,c1);    break;
    case 'r': rook(r1,c1,2);   break;
    case 'h': knight(r1,c1,2); break;
    case 'c': bishop(r1,c1,2); break;
    case 'k': king(r1,c1,2);   break;
    case 'q': queen(r1,c1,2);  break;
    case 'A': abandon(2);       return 0;
    default:
        wprintf(L"\u26A0 Pas de piece noire en %d\n", src);
        return -1;
    }

    if (possible_moves_index < 1) {
        wprintf(L"\u26A0 Aucun coup legal pour la piece en %d\n", src);
        return -1;
    }

    /* ── Valider la destination ── */
    int r2 = LABEL_TO_ROW(dst / 10);
    int c2 = dst % 10;

    if (verify_possible_move(r2 * 10 + c2)) {
        wprintf(L"\u26A0 Destination %d invalide\n", dst);
        return -1;
    }

    /* ── Exécution ── */
    previous_position = r1 * 10 + c1;
    new_position      = r2 * 10 + c2;
    change(r1, c1, r2, c2, 2);
    update_en_passant(piece_moved, r1, r2, c2);

    wprintf(L"\u2705 Noir a joue : %d -> %d\n", src, dst);

    if (is_in_check(1))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI BLANC ! \u26A0\u26A0\n");

    return 0;
}