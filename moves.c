#include "chess.h"

typedef struct {
    char CoupJoueur[100];
}coup;

/*
 *  Convention plateau interne :
 *    board[0] = haut écran = Noir  (minuscules, player_id 2)
 *    board[7] = bas écran  = Blanc (majuscules, player_id 1)
 *    Labels affichés : label = 7 - rangée_interne
 */

/* ════════════════════════════════════════════════════════════════════════════
 *  FONCTIONS DE TEST DE PRÉSENCE
 * ════════════════════════════════════════════════════════════════════════════ */

int check(int x, int y)   /* Noir en (x,y) ? */
{
    if (x<0||x>7||y<0||y>7) return 0;
    switch (board[x][y]) {
    case 'p': case 'r': case 'h': case 'c': case 'k': case 'q': return 1;
    default: return 0;
    }
}

int check2(int x, int y)  /* Blanc en (x,y) ? */
{
    if (x<0||x>7||y<0||y>7) return 0;
    switch (board[x][y]) {
    case 'P': case 'R': case 'H': case 'C': case 'K': case 'Q': return 1;
    default: return 0;
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  is_square_attacked(r, c, attacker_id)
 *  Renvoie 1 si la case (r,c) est attaquée par le camp attacker_id.
 *  Utilisé pour :
 *    - filtrer les cases où le roi ne peut pas aller
 *    - détecter l'échec
 * ════════════════════════════════════════════════════════════════════════════ */
int is_square_attacked(int r, int c, int attacker_id)
{
    /* Les pièces attaquantes sont celles du camp attacker_id */
    char P, R, H, C, K, Q;   /* symboles de l'attaquant */
    if (attacker_id == 1) {   /* Blanc attaque */
        P='P'; R='R'; H='H'; C='C'; K='K'; Q='Q';
    } else {                   /* Noir attaque */
        P='p'; R='r'; H='h'; C='c'; K='k'; Q='q';
    }

    int pawn_dir = (attacker_id == 1) ? +1 : -1;  /* direction d'attaque du pion */

    /* ── Attaque par pion ── */
    int pr = r + pawn_dir;
    if (pr>=0 && pr<=7) {
        if (c-1>=0 && board[pr][c-1] == P) return 1;
        if (c+1<=7 && board[pr][c+1] == P) return 1;
    }

    /* ── Attaque par cavalier ── */
    const int kdr[] = {+2,+2,+1,-1,-2,-2,+1,-1};
    const int kdc[] = {+1,-1,+2,+2,+1,-1,-2,-2};
    for (int i = 0; i < 8; i++) {
        int nr = r+kdr[i], nc = c+kdc[i];
        if (nr>=0&&nr<=7&&nc>=0&&nc<=7 && board[nr][nc]==H) return 1;
    }

    /* ── Attaque en ligne / colonne (tour, dame) ── */
    /* gauche */
    for (int nc=c-1; nc>=0; nc--) {
        if (board[r][nc]==R || board[r][nc]==Q) return 1;
        if (board[r][nc]!=' ') break;
    }
    /* droite */
    for (int nc=c+1; nc<=7; nc++) {
        if (board[r][nc]==R || board[r][nc]==Q) return 1;
        if (board[r][nc]!=' ') break;
    }
    /* haut */
    for (int nr=r-1; nr>=0; nr--) {
        if (board[nr][c]==R || board[nr][c]==Q) return 1;
        if (board[nr][c]!=' ') break;
    }
    /* bas */
    for (int nr=r+1; nr<=7; nr++) {
        if (board[nr][c]==R || board[nr][c]==Q) return 1;
        if (board[nr][c]!=' ') break;
    }

    /* ── Attaque en diagonale (fou, dame) ── */
    const int ddr[] = {-1,-1,+1,+1};
    const int ddc[] = {-1,+1,-1,+1};
    for (int d = 0; d < 4; d++) {
        int nr=r+ddr[d], nc=c+ddc[d];
        while (nr>=0&&nr<=7&&nc>=0&&nc<=7) {
            if (board[nr][nc]==C || board[nr][nc]==Q) return 1;
            if (board[nr][nc]!=' ') break;
            nr+=ddr[d]; nc+=ddc[d];
        }
    }

    /* ── Attaque par roi adverse (pour éviter rois adjacents) ── */
    for (int dr=-1; dr<=1; dr++) for (int dc=-1; dc<=1; dc++) {
        if (dr==0 && dc==0) continue;
        int nr=r+dr, nc=c+dc;
        if (nr>=0&&nr<=7&&nc>=0&&nc<=7 && board[nr][nc]==K) return 1;
    }

    return 0;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  is_in_check(player_id)
 *  Renvoie 1 si le roi du camp player_id est actuellement en échec.
 * ════════════════════════════════════════════════════════════════════════════ */
int is_in_check(int player_id)
{
    char king_char = (player_id == 1) ? 'K' : 'k';
    int kr = -1, kc = -1;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c] == king_char) { kr=r; kc=c; }
    if (kr == -1) return 0;  /* roi introuvable (ne devrait pas arriver) */
    int attacker = (player_id == 1) ? 2 : 1;
    return is_square_attacked(kr, kc, attacker);
}

/* ════════════════════════════════════════════════════════════════════════════
 *  verify_possible_move : renvoie true si position N'EST PAS dans la liste
 * ════════════════════════════════════════════════════════════════════════════ */
bool verify_possible_move(int position)
{
    for (int i = 0; i < possible_moves_index; i++)
        if (position == possible_moves[i]) return false;
    return true;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  PROMOTION
 * ════════════════════════════════════════════════════════════════════════════ */
void promote_pawn(int r, int c, int player_id)
{
    char choice;
    wprintf(L"\n\u2605 Promotion ! D=Dame  T=Tour  F=Fou  C=Cavalier : ");
    fflush(stdout);
    int tmp; while ((tmp = getchar()) != '\n' && tmp != EOF);
    do { choice = (char)getchar(); }
    while (choice!='D'&&choice!='d'&&choice!='T'&&choice!='t'&&
           choice!='F'&&choice!='f'&&choice!='C'&&choice!='c');
    if (player_id == 1) {
        switch(choice){case 'D':case 'd':board[r][c]='Q';break;
                       case 'T':case 't':board[r][c]='R';break;
                       case 'F':case 'f':board[r][c]='C';break;
                       default:          board[r][c]='H';break;}
    } else {
        switch(choice){case 'D':case 'd':board[r][c]='q';break;
                       case 'T':case 't':board[r][c]='r';break;
                       case 'F':case 'f':board[r][c]='c';break;
                       default:          board[r][c]='h';break;}
    }
    wprintf(L"\n");
}

/* ════════════════════════════════════════════════════════════════════════════
 *  CHANGE  — déplace (r1,c1)→(r2,c2) pour player_id
 *  Gère aussi roque, prise en passant, promotion, flags.
 * ════════════════════════════════════════════════════════════════════════════ */
void change(int r1, int c1, int r2, int c2, int player_id)
{
    char temp = board[r1][c1];
    en_passant_capture_row = -1;
    en_passant_capture_col = -1;

    if (player_id == 1) {
        /* Roque blanc (rangée interne 7) */
        if (temp=='K' && r1==7 && c1==4) {
            if (c2==6){ board[7][4]=' ';board[7][6]='K';board[7][7]=' ';board[7][5]='R';
                        white_king_moved=true;white_rook_right_moved=true;return; }
            if (c2==2){ board[7][4]=' ';board[7][2]='K';board[7][0]=' ';board[7][3]='R';
                        white_king_moved=true;white_rook_left_moved=true;return; }
        }
        /* Prise en passant blanc */
        if (temp=='P' && c2!=c1 && board[r2][c2]==' ') {
            insert_eliminated_list(board[r1][c2], &eliminated_pieces_white);
            en_passant_capture_row=r1; en_passant_capture_col=c2;
            board[r1][c2]=' '; board[r1][c1]=' '; board[r2][c2]=temp;
            if (r2==0) promote_pawn(r2,c2,1);
            return;
        }
        /* Capture ou déplacement */
        if (check(r2,c2)) insert_eliminated_list(board[r2][c2], &eliminated_pieces_white);
        board[r1][c1]=' '; board[r2][c2]=temp;
        if (board[r2][c2]=='P' && r2==0) promote_pawn(r2,c2,1);
        if (temp=='K') white_king_moved=true;
        if (temp=='R'&&r1==7&&c1==0) white_rook_left_moved=true;
        if (temp=='R'&&r1==7&&c1==7) white_rook_right_moved=true;

    } else if (player_id == 2) {
        /* Roque noir (rangée interne 0) */
        if (temp=='k' && r1==0 && c1==4) {
            if (c2==6){ board[0][4]=' ';board[0][6]='k';board[0][7]=' ';board[0][5]='r';
                        black_king_moved=true;black_rook_right_moved=true;return; }
            if (c2==2){ board[0][4]=' ';board[0][2]='k';board[0][0]=' ';board[0][3]='r';
                        black_king_moved=true;black_rook_left_moved=true;return; }
        }
        /* Prise en passant noir */
        if (temp=='p' && c2!=c1 && board[r2][c2]==' ') {
            insert_eliminated_list(board[r1][c2], &eliminated_pieces_black);
            en_passant_capture_row=r1; en_passant_capture_col=c2;
            board[r1][c2]=' '; board[r1][c1]=' '; board[r2][c2]=temp;
            if (r2==7) promote_pawn(r2,c2,2);
            return;
        }
        /* Capture ou déplacement */
        if (check2(r2,c2)) insert_eliminated_list(board[r2][c2], &eliminated_pieces_black);
        board[r1][c1]=' '; board[r2][c2]=temp;
        if (board[r2][c2]=='p' && r2==7) promote_pawn(r2,c2,2);
        if (temp=='k') black_king_moved=true;
        if (temp=='r'&&r1==0&&c1==0) black_rook_left_moved=true;
        if (temp=='r'&&r1==0&&c1==7) black_rook_right_moved=true;
    } else {
        wprintf(L"Error : change()\n");
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  Helper : tente un coup simulé et renvoie 1 si le roi est en échec après
 *  (remet le plateau dans l'état initial dans tous les cas)
 * ════════════════════════════════════════════════════════════════════════════ */
static int move_leaves_king_in_check(int r1, int c1, int r2, int c2, int player_id)
{
    /* Sauvegarder */
    char saved[8][8];
    memcpy(saved, board, sizeof(board));

    /* Appliquer le coup temporairement (simplifié : pas de promotion/flags) */
    board[r2][c2] = board[r1][c1];
    board[r1][c1] = ' ';

    int result = is_in_check(player_id);

    /* Restaurer */
    memcpy(board, saved, sizeof(board));
    return result;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  PION BLANC  (départ rangée interne 6, avance vers 0)
 * ════════════════════════════════════════════════════════════════════════════ */
void pawn(int r1, int c1)
{
    possible_moves_index = 0;

    if (r1-1>=0 && board[r1-1][c1]==' ') {
        if (!move_leaves_king_in_check(r1,c1,r1-1,c1,1))
            possible_moves[possible_moves_index++] = (r1-1)*10+c1;
        if (r1==6 && board[r1-2][c1]==' ' &&
            !move_leaves_king_in_check(r1,c1,r1-2,c1,1))
            possible_moves[possible_moves_index++] = (r1-2)*10+c1;
    }
    if (r1-1>=0&&c1+1<=7&&check(r1-1,c1+1) &&
        !move_leaves_king_in_check(r1,c1,r1-1,c1+1,1))
        possible_moves[possible_moves_index++] = (r1-1)*10+c1+1;
    if (r1-1>=0&&c1-1>=0&&check(r1-1,c1-1) &&
        !move_leaves_king_in_check(r1,c1,r1-1,c1-1,1))
        possible_moves[possible_moves_index++] = (r1-1)*10+c1-1;
    /* Prise en passant */
    if (r1==3 && en_passant_row==3) {
        if (en_passant_col==c1+1 && !move_leaves_king_in_check(r1,c1,r1-1,c1+1,1))
            possible_moves[possible_moves_index++] = (r1-1)*10+c1+1;
        if (en_passant_col==c1-1 && !move_leaves_king_in_check(r1,c1,r1-1,c1-1,1))
            possible_moves[possible_moves_index++] = (r1-1)*10+c1-1;
    }

    wprintf(L"Mouvements possibles %lc : \n",(wchar_t)display_convert(board[r1][c1]));
    if (possible_moves_index>=1) for(int i=0;i<possible_moves_index;i++) wprintf(L" %d :",possible_moves[i]);
    else wprintf(L"Aucun mouvement possible -> ");
}

/* ════════════════════════════════════════════════════════════════════════════
 *  PION NOIR  (départ rangée interne 1, avance vers 7)
 * ════════════════════════════════════════════════════════════════════════════ */
void pawnb(int r1, int c1)
{
    possible_moves_index = 0;

    if (r1+1<=7 && board[r1+1][c1]==' ') {
        if (!move_leaves_king_in_check(r1,c1,r1+1,c1,2))
            possible_moves[possible_moves_index++] = (r1+1)*10+c1;
        if (r1==1 && board[r1+2][c1]==' ' &&
            !move_leaves_king_in_check(r1,c1,r1+2,c1,2))
            possible_moves[possible_moves_index++] = (r1+2)*10+c1;
    }
    if (r1+1<=7&&c1+1<=7&&check2(r1+1,c1+1) &&
        !move_leaves_king_in_check(r1,c1,r1+1,c1+1,2))
        possible_moves[possible_moves_index++] = (r1+1)*10+c1+1;
    if (r1+1<=7&&c1-1>=0&&check2(r1+1,c1-1) &&
        !move_leaves_king_in_check(r1,c1,r1+1,c1-1,2))
        possible_moves[possible_moves_index++] = (r1+1)*10+c1-1;
    /* Prise en passant */
    if (r1==4 && en_passant_row==4) {
        if (en_passant_col==c1+1 && !move_leaves_king_in_check(r1,c1,r1+1,c1+1,2))
            possible_moves[possible_moves_index++] = (r1+1)*10+c1+1;
        if (en_passant_col==c1-1 && !move_leaves_king_in_check(r1,c1,r1+1,c1-1,2))
            possible_moves[possible_moves_index++] = (r1+1)*10+c1-1;
    }

    wprintf(L"Mouvements possibles %lc : \n",(wchar_t)display_convert(board[r1][c1]));
    if (possible_moves_index>=1) for(int i=0;i<possible_moves_index;i++) wprintf(L" %d :",possible_moves[i]);
    else wprintf(L"Aucun mouvement possible -> ");
}

/* ════════════════════════════════════════════════════════════════════════════
 *  TOUR
 * ════════════════════════════════════════════════════════════════════════════ */
void rook(int r1, int c1, int player_id)
{
    possible_moves_index = 0;
    int (*enemy)(int,int) = (player_id==1)?check:check2;
    int (*ally)(int,int)  = (player_id==1)?check2:check;
    int n;

    n=c1; while(n>0&&(board[r1][n-1]==' '||enemy(r1,n-1)))
        { if(!ally(r1,n-1)&&!move_leaves_king_in_check(r1,c1,r1,n-1,player_id)) possible_moves[possible_moves_index++]=r1*10+(n-1); if(enemy(r1,n-1)) break; n--; }
    n=c1; while(n<7&&(board[r1][n+1]==' '||enemy(r1,n+1)))
        { if(!ally(r1,n+1)&&!move_leaves_king_in_check(r1,c1,r1,n+1,player_id)) possible_moves[possible_moves_index++]=r1*10+(n+1); if(enemy(r1,n+1)) break; n++; }
    n=r1; while(n>0&&(board[n-1][c1]==' '||enemy(n-1,c1)))
        { if(!ally(n-1,c1)&&!move_leaves_king_in_check(r1,c1,n-1,c1,player_id)) possible_moves[possible_moves_index++]=(n-1)*10+c1; if(enemy(n-1,c1)) break; n--; }
    n=r1; while(n<7&&(board[n+1][c1]==' '||enemy(n+1,c1)))
        { if(!ally(n+1,c1)&&!move_leaves_king_in_check(r1,c1,n+1,c1,player_id)) possible_moves[possible_moves_index++]=(n+1)*10+c1; if(enemy(n+1,c1)) break; n++; }

    wprintf(L"Mouvements possibles %lc : \n",(wchar_t)display_convert(board[r1][c1]));
    if (possible_moves_index>=1) for(int i=0;i<possible_moves_index;i++) wprintf(L" %d :",possible_moves[i]);
    else wprintf(L"Aucun mouvement possible -> ");
}

/* ════════════════════════════════════════════════════════════════════════════
 *  CAVALIER
 * ════════════════════════════════════════════════════════════════════════════ */
void knight(int r1, int c1, int player_id)
{
    possible_moves_index = 0;
    int (*enemy)(int,int) = (player_id==1)?check:check2;
    int (*ally)(int,int)  = (player_id==1)?check2:check;
    const int dr[]={+2,+2,+1,-1,-2,-2,+1,-1};
    const int dc[]={+1,-1,+2,+2,+1,-1,-2,-2};
    for (int i=0;i<8;i++){
        int nr=r1+dr[i],nc=c1+dc[i];
        if(nr>=0&&nr<=7&&nc>=0&&nc<=7&&!ally(nr,nc)&&(board[nr][nc]==' '||enemy(nr,nc)))
            if(!move_leaves_king_in_check(r1,c1,nr,nc,player_id))
                possible_moves[possible_moves_index++]=nr*10+nc;
    }
    wprintf(L"Mouvements possibles %lc : \n",(wchar_t)display_convert(board[r1][c1]));
    if (possible_moves_index>=1) for(int i=0;i<possible_moves_index;i++) wprintf(L" %d :",possible_moves[i]);
    else wprintf(L"Aucun mouvement possible -> ");
}

/* ─── Helper diagonale ─────────────────────────────────────────────────────── */
static void bishop_diag(int r1,int c1,int dr,int dc,int(*enemy)(int,int),int(*ally)(int,int),int player_id)
{
    int a=r1+dr,b=c1+dc;
    while(a>=0&&a<=7&&b>=0&&b<=7){
        if(ally(a,b)) break;
        if(board[a][b]==' '||enemy(a,b)){
            if(!move_leaves_king_in_check(r1,c1,a,b,player_id))
                possible_moves[possible_moves_index++]=a*10+b;
            if(enemy(a,b)) break;
        } else break;
        a+=dr; b+=dc;
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  FOU
 * ════════════════════════════════════════════════════════════════════════════ */
void bishop(int r1, int c1, int player_id)
{
    possible_moves_index = 0;
    int (*enemy)(int,int)=(player_id==1)?check:check2;
    int (*ally)(int,int) =(player_id==1)?check2:check;
    bishop_diag(r1,c1,-1,+1,enemy,ally,player_id);
    bishop_diag(r1,c1,+1,-1,enemy,ally,player_id);
    bishop_diag(r1,c1,+1,+1,enemy,ally,player_id);
    bishop_diag(r1,c1,-1,-1,enemy,ally,player_id);
    wprintf(L"Mouvements possibles %lc : \n",(wchar_t)display_convert(board[r1][c1]));
    if (possible_moves_index>=1) for(int i=0;i<possible_moves_index;i++) wprintf(L" %d :",possible_moves[i]);
    else wprintf(L"Aucun mouvement possible -> ");
}

/* ════════════════════════════════════════════════════════════════════════════
 *  ROI  +  ROQUE
 *  - Exclut les pièces alliées
 *  - Exclut les cases attaquées par l'adversaire
 *  - Roque : vérifie aussi que les cases traversées ne sont pas attaquées
 * ════════════════════════════════════════════════════════════════════════════ */
void king(int r1, int c1, int player_id)
{
    possible_moves_index = 0;
    int (*enemy)(int,int)=(player_id==1)?check:check2;
    int (*ally)(int,int) =(player_id==1)?check2:check;
    int attacker = (player_id==1)?2:1;

    const int dr[]={0,0,-1,-1,-1,+1,+1,+1};
    const int dc[]={+1,-1,0,+1,-1,0,+1,-1};
    for(int i=0;i<8;i++){
        int nr=r1+dr[i],nc=c1+dc[i];
        if(nr>=0&&nr<=7&&nc>=0&&nc<=7
           && !ally(nr,nc)
           && (board[nr][nc]==' '||enemy(nr,nc))
           && !is_square_attacked(nr,nc,attacker)
           && !move_leaves_king_in_check(r1,c1,nr,nc,player_id))
            possible_moves[possible_moves_index++]=nr*10+nc;
    }

    /* ── Roque blanc (rangée interne 7) ── */
    if (player_id==1 && !white_king_moved && r1==7 && c1==4
        && !is_square_attacked(7,4,2)) {          /* roi ne doit pas être en échec */
        /* Petit roque */
        if (!white_rook_right_moved
            && board[7][5]==' ' && board[7][6]==' ' && board[7][7]=='R'
            && !is_square_attacked(7,5,2)          /* case traversée */
            && !is_square_attacked(7,6,2))         /* case d'arrivée */
            possible_moves[possible_moves_index++] = 76;
        /* Grand roque */
        if (!white_rook_left_moved
            && board[7][1]==' ' && board[7][2]==' ' && board[7][3]==' ' && board[7][0]=='R'
            && !is_square_attacked(7,3,2)
            && !is_square_attacked(7,2,2))
            possible_moves[possible_moves_index++] = 72;
    }

    /* ── Roque noir (rangée interne 0) ── */
    if (player_id==2 && !black_king_moved && r1==0 && c1==4
        && !is_square_attacked(0,4,1)) {
        if (!black_rook_right_moved
            && board[0][5]==' ' && board[0][6]==' ' && board[0][7]=='r'
            && !is_square_attacked(0,5,1)
            && !is_square_attacked(0,6,1))
            possible_moves[possible_moves_index++] = 6;
        if (!black_rook_left_moved
            && board[0][1]==' ' && board[0][2]==' ' && board[0][3]==' ' && board[0][0]=='r'
            && !is_square_attacked(0,3,1)
            && !is_square_attacked(0,2,1))
            possible_moves[possible_moves_index++] = 2;
    }

    wprintf(L"Mouvements possibles %lc : \n",(wchar_t)display_convert(board[r1][c1]));
    if (possible_moves_index>=1) for(int i=0;i<possible_moves_index;i++) wprintf(L" %d :",possible_moves[i]);
    else wprintf(L"Aucun mouvement possible -> ");
}

/* ════════════════════════════════════════════════════════════════════════════
 *  DAME
 * ════════════════════════════════════════════════════════════════════════════ */
void queen(int r1, int c1, int player_id)
{
    possible_moves_index = 0;
    int (*enemy)(int,int)=(player_id==1)?check:check2;
    int (*ally)(int,int) =(player_id==1)?check2:check;
    int n;

    bishop_diag(r1,c1,-1,+1,enemy,ally,player_id);
    bishop_diag(r1,c1,+1,-1,enemy,ally,player_id);
    bishop_diag(r1,c1,+1,+1,enemy,ally,player_id);
    bishop_diag(r1,c1,-1,-1,enemy,ally,player_id);

    n=c1; while(n>0&&(board[r1][n-1]==' '||enemy(r1,n-1)))
        { if(!ally(r1,n-1)&&!move_leaves_king_in_check(r1,c1,r1,n-1,player_id)) possible_moves[possible_moves_index++]=r1*10+(n-1); if(enemy(r1,n-1)){break;} n--; }
    n=c1; while(n<7&&(board[r1][n+1]==' '||enemy(r1,n+1)))
        { if(!ally(r1,n+1)&&!move_leaves_king_in_check(r1,c1,r1,n+1,player_id)) possible_moves[possible_moves_index++]=r1*10+(n+1); if(enemy(r1,n+1)){break;} n++; }
    n=r1; while(n>0&&(board[n-1][c1]==' '||enemy(n-1,c1)))
        { if(!ally(n-1,c1)&&!move_leaves_king_in_check(r1,c1,n-1,c1,player_id)) possible_moves[possible_moves_index++]=(n-1)*10+c1; if(enemy(n-1,c1)){break;} n--; }
    n=r1; while(n<7&&(board[n+1][c1]==' '||enemy(n+1,c1)))
        { if(!ally(n+1,c1)&&!move_leaves_king_in_check(r1,c1,n+1,c1,player_id)) possible_moves[possible_moves_index++]=(n+1)*10+c1; if(enemy(n+1,c1)){break;} n++; }

    wprintf(L"Mouvements possibles %lc : \n",(wchar_t)display_convert(board[r1][c1]));
    if (possible_moves_index>=1) for(int i=0;i<possible_moves_index;i++) wprintf(L" %d :",possible_moves[i]);
    else wprintf(L"Aucun mouvement possible -> ");
}
