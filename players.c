#include "chess.h"

/* label affiché → rangée interne */
#define LABEL_TO_ROW(l) (7-(l))
/* rangée interne → label affiché */
#define ROW_TO_LABEL(r) (7-(r))

static void flush_stdin(void)
{
    int tmp;
    while ((tmp = getchar()) != '\n' && tmp != EOF);
}

static void update_en_passant(char piece_moved, int r1, int r2, int c2)
{
    if (piece_moved=='P' && r1==6 && r2==4) { en_passant_col=c2; en_passant_row=4; }
    else if (piece_moved=='p' && r1==1 && r2==3) { en_passant_col=c2; en_passant_row=3; }
    else { en_passant_col=-1; en_passant_row=-1; }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  JOUEUR BLANC
 * ════════════════════════════════════════════════════════════════════════════ */
void player_white(void)
{
    int p1, input_control;
    int r1_label, c1, r1, r2, c2;
    char piece_moved;

    /* Alerte échec */
    if (is_in_check(1))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI BLANC ! \u26A0\u26A0\n");

    wprintf(L"\n\u26AA Blanc joue ...  (pieces en labels 0-1)\n");

again1:
    do {
        flush_stdin();
        wprintf(L"\nPosition [label*10+col] : ");
        input_control = scanf("%d", &p1);
        if (input_control==0||p1<0||p1>77||p1%10>7)
            wprintf(L"\u26A0 Saisie incorrecte\n");
    } while (input_control==0||p1<0||p1>77||p1%10>7);

    r1_label = p1/10;
    c1       = p1%10;
    r1       = LABEL_TO_ROW(r1_label);
    piece_moved = board[r1][c1];

    switch (board[r1][c1]) {
    case 'P': pawn(r1,c1);      break;
    case 'R': rook(r1,c1,1);    break;
    case 'H': knight(r1,c1,1);  break;
    case 'C': bishop(r1,c1,1);  break;
    case 'K': king(r1,c1,1);    break;
    case 'Q': queen(r1,c1,1);   break;
    default:  wprintf(L"\u26A0 Choisissez une piece blanche.\n"); goto again1;
    }

    if (possible_moves_index < 1) {
        wprintf(L" -> aucun coup legal, choisissez une autre piece.\n");
        goto again1;
    }

    system("clear");
    if (is_in_check(1)) wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI BLANC ! \u26A0\u26A0\n");
    append_possible_moves();
    display();
    wprintf(L"Mouvements possibles %lc :\n",(wchar_t)display_convert(board[r1][c1]));
    for (int i=0;i<possible_moves_index;i++){
        int pr=possible_moves[i]/10, pc=possible_moves[i]%10;
        wprintf(L" %d%d :", ROW_TO_LABEL(pr), pc);
    }

    int p2_enc;
    do {
        flush_stdin();
        wprintf(L"\nNouvelle position [label*10+col] : ");
        scanf("%d",&p2_enc);
        r2 = LABEL_TO_ROW(p2_enc/10);
        c2 = p2_enc%10;
    } while (verify_possible_move(r2*10+c2));

    previous_position = r1*10+c1;
    new_position      = r2*10+c2;
    change(r1,c1,r2,c2,1);
    update_en_passant(piece_moved,r1,r2,c2);

    /* Annoncer échec à l'adversaire */
    if (is_in_check(2))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI NOIR ! \u26A0\u26A0\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 *  JOUEUR NOIR
 * ════════════════════════════════════════════════════════════════════════════ */
void player_black(void)
{
    int p1, input_control;
    int r1_label, c1, r1, r2, c2;
    char piece_moved;

    if (is_in_check(2))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI NOIR ! \u26A0\u26A0\n");

    wprintf(L"\n\u26AB Noir joue ...  (pieces en labels 6-7)\n");

again2:
    do {
        flush_stdin();
        wprintf(L"\nPosition [label*10+col] : ");
        input_control = scanf("%d",&p1);
        if (input_control==0||p1<0||p1>77||p1%10>7)
            wprintf(L"\u26A0 Saisie incorrecte\n");
    } while (input_control==0||p1<0||p1>77||p1%10>7);

    r1_label = p1/10;
    c1       = p1%10;
    r1       = LABEL_TO_ROW(r1_label);
    piece_moved = board[r1][c1];

    switch (board[r1][c1]) {
    case 'p': pawnb(r1,c1);     break;
    case 'r': rook(r1,c1,2);    break;
    case 'h': knight(r1,c1,2);  break;
    case 'c': bishop(r1,c1,2);  break;
    case 'k': king(r1,c1,2);    break;
    case 'q': queen(r1,c1,2);   break;
    default:  wprintf(L"\u26A0 Choisissez une piece noire.\n"); goto again2;
    }

    if (possible_moves_index < 1) {
        wprintf(L" -> aucun coup legal, choisissez une autre piece.\n");
        goto again2;
    }

    system("clear");
    if (is_in_check(2)) wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI NOIR ! \u26A0\u26A0\n");
    append_possible_moves();
    display();
    wprintf(L"Mouvements possibles %lc :\n",(wchar_t)display_convert(board[r1][c1]));
    for (int i=0;i<possible_moves_index;i++){
        int pr=possible_moves[i]/10, pc=possible_moves[i]%10;
        wprintf(L" %d%d :", ROW_TO_LABEL(pr), pc);
    }

    int p2_enc;
    do {
        flush_stdin();
        wprintf(L"\nNouvelle position [label*10+col] : ");
        scanf("%d",&p2_enc);
        r2 = LABEL_TO_ROW(p2_enc/10);
        c2 = p2_enc%10;
    } while (verify_possible_move(r2*10+c2));

    previous_position = r1*10+c1;
    new_position      = r2*10+c2;
    change(r1,c1,r2,c2,2);
    update_en_passant(piece_moved,r1,r2,c2);

    if (is_in_check(1))
        wprintf(L"\n\u26A0\u26A0 ECHEC AU ROI BLANC ! \u26A0\u26A0\n");
}
