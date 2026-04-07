#include "showcase.h"
#include "display.h"
#include "moves.h"
#include "utils.h"

void showcase_board(char row, char col, char new_row, char new_col, int id)
{
    int r1 = (int)(row     - '0');
    int c1 = (int)(col     - '0');
    int r2 = (int)(new_row - '0');
    int c2 = (int)(new_col - '0');

    if (r1 == 0 && c1 == 0 && r2 == 0 && c2 == 0 && id == 2) {
        change(7, 4, 7, 6, 2);
        change(7, 7, 7, 5, 2);
    } else if (r1 == 0 && c1 == 0 && r2 == 0 && c2 == 0 && id == 1) {
        change(0, 4, 0, 2, 1);
        change(0, 0, 0, 3, 1);
    } else {
        change(r1, c1, r2, c2, id);
        display();
    }
}

void showcase_game(void)
{
    FILE *fp = fopen("chess_game_notes.txt", "r");
    if (fp == NULL) {
        perror("Impossible d'ouvrir chess_game_notes.txt");
        exit(1);
    }

    char chunk[128];
    char white_in[2], white_out[2];
    char black_in[2], black_out[2];

    while (fgets(chunk, sizeof(chunk), fp) != NULL) {
        white_in[0]  = chunk[1];  white_in[1]  = chunk[2];
        white_out[0] = chunk[4];  white_out[1] = chunk[5];
        wprintf(L"%c%c.%c%c\n", white_in[0], white_in[1], white_out[0], white_out[1]);
        showcase_board(white_in[0], white_in[1], white_out[0], white_out[1], 2);
        delay(2);

        black_in[0]  = chunk[7];  black_in[1]  = chunk[8];
        black_out[0] = chunk[10]; black_out[1] = chunk[11];
        wprintf(L"%c%c.%c%c\n", black_in[0], black_in[1], black_out[0], black_out[1]);
        showcase_board(black_in[0], black_in[1], black_out[0], black_out[1], 1);
        delay(2);
    }

    fclose(fp);
}