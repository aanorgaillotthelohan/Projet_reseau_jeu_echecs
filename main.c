#include "chess.h"
#include <locale.h>

int main(void)
{
    /* ── Initialisation ─────────────────────────────────────────────────── */
    setlocale(LC_ALL, "");          /* Active l'Unicode (remplace _setmode) */

    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;
    piece = (char *)malloc(sizeof(char));
    board[7][7] = 'R'; /* rangée 7 = Blanc, correction corruption éventuelle */

    int  x = 1;   /* Tour 1 = Blanc commence */
    char ch;
    char showcase_option;
    joueur* j;
    /* ── Écran titre ────────────────────────────────────────────────────── */
    system("clear");
    intro();
    wprintf(L"\nAppuyez sur Espace pour le mode replay, ou toute autre touche pour jouer...\n");
    showcase_option = getch_linux();   /* Remplace getch() de <conio.h> */

    system("clear");

    if (showcase_option == 32) /* Espace */
    {
        showcase_game();
    }
    else
    {
        do {
            remove_possible_moves();
            system("clear");
            display();

            if ((x % 2) != 0)
                player_white(j);   /* tours impairs : Blanc */
            else
                player_black(j);   /* tours pairs   : Noir  */

            x++;

            wprintf(L"\nAppuyez sur une touche pour continuer... (Echap pour quitter)\n");
            ch = getch_linux();   /* Remplace getch() de <conio.h> */

        } while (ch != 27); /* 27 = Echap */
    }

    free(piece);
    return 0;
}
