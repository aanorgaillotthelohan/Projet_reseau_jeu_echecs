#include "chess.h"
#include <stdlib.h>

#define LABEL_TO_ROW(l) (7-(l))
#define ROW_TO_LABEL(r) (7-(r))

joueur* creejoueur(int socket, char NomDuJoueur[20]){
    joueur* j = malloc(sizeof(joueur));
    if(j == NULL) return NULL;
    strncpy(j->NomJoueur, NomDuJoueur, 20);
    j->socketJoueur = socket;
    strncpy(j->Couleur, "blanc", 20);
    j->rejoinsSalon = false;
    return j;
}

static void update_en_passant(char piece_moved, int r1, int r2, int c2)
{
    if      (piece_moved=='P' && r1==6 && r2==4) { en_passant_col=c2; en_passant_row=4; }
    else if (piece_moved=='p' && r1==1 && r2==3) { en_passant_col=c2; en_passant_row=3; }
    else { en_passant_col=-1; en_passant_row=-1; }
}

/*
 * parse_coup : découpe j->coupDuTour au format "XYZW"
 *   XY = position source      (label*10 + col), ex: "63" → label 6, col 3
 *   ZW = position destination (label*10 + col), ex: "74" → label 7, col 4
 * Retourne 0 si OK, -1 si invalide.
 */
static int parse_coup(joueur *j, int *src, int *dst)
{
    char *c = j->coupDuTour;
    if(strlen(c) < 4)          return -1;
    if(c[0]<'0' || c[0]>'7')   return -1;
    if(c[1]<'0' || c[1]>'7')   return -1;
    if(c[2]<'0' || c[2]>'7')   return -1;
    if(c[3]<'0' || c[3]>'7')   return -1;

    *src = (c[0]-'0') * 10 + (c[1]-'0');  /* ex: '6','3' → 63 */
    *dst = (c[2]-'0') * 10 + (c[3]-'0');  /* ex: '7','4' → 74 */
    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  JOUEUR BLANC
 * ════════════════════════════════════════════════════════════════════════════ */
void player_white(joueur *j)
{
    int p1, p2_enc;
    int r1_label, c1, r1, r2, c2;
    char piece_moved;

    if(is_in_check(1))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI BLANC ! \u26A0\u26A0\n");

    wprintf(L"\n\u26AA Blanc joue ... coup recu : \"%s\"\n", j->coupDuTour);

    /* ── Décoder le coup ── */
    int src, dst;
    if(parse_coup(j, &src, &dst) < 0){
        wprintf(L"\u26A0 coupDuTour invalide : \"%s\" (format attendu: XYZW)\n",
                j->coupDuTour);
        return;
    }
    p1     = src;
    p2_enc = dst;

    if(p1 < 0 || p1 > 77 || p1 % 10 > 7){
        wprintf(L"\u26A0 Position source invalide : %d\n", p1);
        return;
    }

    r1_label    = p1 / 10;
    c1          = p1 % 10;
    r1          = LABEL_TO_ROW(r1_label);
    piece_moved = board[r1][c1];

    /* ── Calculer les coups légaux ── */
    switch(board[r1][c1]){
    case 'P': pawn(r1,c1);      break;
    case 'R': rook(r1,c1,1);    break;
    case 'H': knight(r1,c1,1);  break;
    case 'C': bishop(r1,c1,1);  break;
    case 'K': king(r1,c1,1);    break;
    case 'Q': queen(r1,c1,1);   break;
    case 'A': abandon(1);        return;
    default:
        wprintf(L"\u26A0 Pas de piece blanche en %d\n", p1);
        return;
    }

    if(possible_moves_index < 1){
        wprintf(L"\u26A0 Aucun coup legal pour la piece en %d\n", p1);
        return;
    }

    /* ── Valider la destination ── */
    r2 = LABEL_TO_ROW(p2_enc / 10);
    c2 = p2_enc % 10;

    if(verify_possible_move(r2 * 10 + c2)){
        wprintf(L"\u26A0 Destination %d invalide (coup: \"%s\")\n",
                p2_enc, j->coupDuTour);
        return;
    }

    /* ── Jouer ── */
    previous_position = r1 * 10 + c1;
    new_position      = r2 * 10 + c2;
    change(r1, c1, r2, c2, 1);
    update_en_passant(piece_moved, r1, r2, c2);

    wprintf(L"\u2705 Blanc a joue : %s\n", j->coupDuTour);

    if(is_in_check(2))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI NOIR ! \u26A0\u26A0\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 *  JOUEUR NOIR
 * ════════════════════════════════════════════════════════════════════════════ */
void player_black(joueur *j)
{
    int p1, p2_enc;
    int r1_label, c1, r1, r2, c2;
    char piece_moved;

    if(is_in_check(2))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI NOIR ! \u26A0\u26A0\n");

    wprintf(L"\n\u26AB Noir joue ... coup recu : \"%s\"\n", j->coupDuTour);

    /* ── Décoder le coup ── */
    int src, dst;
    if(parse_coup(j, &src, &dst) < 0){
        wprintf(L"\u26A0 coupDuTour invalide : \"%s\" (format attendu: XYZW)\n",
                j->coupDuTour);
        return;
    }
    p1     = src;
    p2_enc = dst;

    if(p1 < 0 || p1 > 77 || p1 % 10 > 7){
        wprintf(L"\u26A0 Position source invalide : %d\n", p1);
        return;
    }

    r1_label    = p1 / 10;
    c1          = p1 % 10;
    r1          = LABEL_TO_ROW(r1_label);
    piece_moved = board[r1][c1];

    /* ── Calculer les coups légaux ── */
    switch(board[r1][c1]){
    case 'p': pawnb(r1,c1);    break;
    case 'r': rook(r1,c1,2);   break;
    case 'h': knight(r1,c1,2); break;
    case 'c': bishop(r1,c1,2); break;
    case 'k': king(r1,c1,2);   break;
    case 'q': queen(r1,c1,2);  break;
    case 'A': abandon(2);       return;
    default:
        wprintf(L"\u26A0 Pas de piece noire en %d\n", p1);
        return;
    }

    if(possible_moves_index < 1){
        wprintf(L"\u26A0 Aucun coup legal pour la piece en %d\n", p1);
        return;
    }

    /* ── Valider la destination ── */
    r2 = LABEL_TO_ROW(p2_enc / 10);
    c2 = p2_enc % 10;

    if(verify_possible_move(r2 * 10 + c2)){
        wprintf(L"\u26A0 Destination %d invalide (coup: \"%s\")\n",
                p2_enc, j->coupDuTour);
        return;
    }

    /* ── Jouer ── */
    previous_position = r1 * 10 + c1;
    new_position      = r2 * 10 + c2;
    change(r1, c1, r2, c2, 2);
    update_en_passant(piece_moved, r1, r2, c2);

    wprintf(L"\u2705 Noir a joue : %s\n", j->coupDuTour);

    if(is_in_check(1))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI BLANC ! \u26A0\u26A0\n");
}