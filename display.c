#include "display.h"
#include "pieces.h"

int display_convert(char symbol)
{
    switch (symbol) {
    case 'p': return 0x265F;
    case 'r': return 0x265C;
    case 'h': return 0x265E;
    case 'c': return 0x265D;
    case 'k': return 0x265A;
    case 'q': return 0x265B;
    case 'P': return 0x2659;
    case 'R': return 0x2656;
    case 'H': return 0x2658;
    case 'C': return 0x2657;
    case 'K': return 0x2654;
    case 'Q': return 0x2655;
    case 'x': return 0x2716;
    default:  return 0x0020;
    }
}

void intro(void)
{
    printf("     \n");
    printf("      __ \n");
    printf("     /  \\ \n");
    printf("     \\__/                             _               \n");
    printf("    /____\\             __          __| |__   ___  ___ ___            \n");
    printf("     |__|            / __| ___   / __| '_ \\ / _ \\/ __/ __|                 \n");
    printf("    (====)          | (__ |___| | (__| | | |  __/\\__ \\__ \\      \n");
    printf("    }===={           \\___|       \\___|_| |_|\\___||___/___/          - Kahouli Wajd.\n");
    printf("  (________) \n");
}

void display(void)
{
    int i, j;

    if (new_position == -99) { board[7][7] = 'R'; }

    display_eliminated_list(eliminated_pieces_white);

    printf("    ");
    for (i = 0; i < 8; i++) { printf("   %d", i); }
    printf("\n");

    for (int label = 0; label < 8; label++) {
        int k = 7 - label;
        printf("    ");
        for (i = 0; i < 34; i++) { printf("-"); }
        printf("\n %d  ", label);
        for (j = 0; j < 8; j++) {
            printf("||%lc ", (wchar_t)display_convert(board[k][j]));
        }
        printf("||\n");
    }
    printf("    ");
    for (i = 0; i < 34; i++) { printf("-"); }
    printf("\n");

    display_eliminated_list(eliminated_pieces_black);
    printf("\n");
}

void display_possible_moves(char b[8][8])
{
    int i, j, k;
    printf(" ");
    for (i = 0; i < 8; i++) { printf("   %d", i); }
    printf("\n");

    for (k = 0; k < 8; k++) {
        printf("  ");
        for (i = 0; i < 34; i++) { printf("-"); }
        printf("\n%d ", k);
        for (j = 0; j < 8; j++) {
            printf("||%lc ", (wchar_t)display_convert(b[k][j]));
        }
        printf("||\n");
    }
    printf("  ");
    for (i = 0; i < 34; i++) { printf("-"); }
    printf("\n");
}

void append_possible_moves(void)
{
    int x, y;
    for (int i = 0; i < possible_moves_index; i++) {
        x = possible_moves[i] / 10;
        y = possible_moves[i] % 10;
        if (board[x][y] == ' ') { board[x][y] = 'x'; }
    }
}

void remove_possible_moves(void)
{
    for (int i = 0; i < possible_moves_index; i++) {
        int x = possible_moves[i] / 10;
        int y = possible_moves[i] % 10;
        if ((x * 10 + y) != new_position || new_position == -1) {
            if (board[x][y] == 'x')
                board[x][y] = ' ';
        }
    }
}