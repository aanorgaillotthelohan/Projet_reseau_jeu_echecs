#include <locale.h>
#include "chess.h"
#include "display.h"
#include "players.h"
#include "utils.h"
#include "showcase.h"

int main(void)
{
    setlocale(LC_ALL, "");

    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;
    piece    = (char *)malloc(sizeof(char));
    board[7][7] = 'R';

    int  tour = 1;
    char ch;
    char showcase_option;

    /* Initialiser deux joueurs locaux (standalone) */
    joueur j_blanc, j_noir;
    memset(&j_blanc, 0, sizeof(joueur));
    memset(&j_noir,  0, sizeof(joueur));
    strncpy(j_blanc.Couleur, "blanc", 20);
    strncpy(j_noir.Couleur,  "noir",  20);

    system("clear");
    intro();
    wprintf(L"\nAppuyez sur Espace pour le mode replay, ou toute autre touche pour jouer...\n");
    showcase_option = getch_linux();
    system("clear");

    if (showcase_option == 32) {
        showcase_game();
    } else {
        do {
            remove_possible_moves();
            system("clear");
            display();

            /* ── Lire le coup au clavier ── */
            wprintf(L"\nEntrez votre coup (format XYZW, ex: 6374) : ");
            int coup_saisi = 0;
            scanf("%d", &coup_saisi);

            int src, dst;

            if ((tour % 2) != 0) {
                /* Blanc joue */
                j_blanc.coupDuTour = coup_saisi;
                if (parse_coup(&j_blanc, &src, &dst) < 0) {
                    wprintf(L"\u26A0 Coup invalide : %d\n", coup_saisi);
                } else {
                    player_white(&j_blanc, src, dst);
                    tour++;
                }
            } else {
                /* Noir joue */
                j_noir.coupDuTour = coup_saisi;
                if (parse_coup(&j_noir, &src, &dst) < 0) {
                    wprintf(L"\u26A0 Coup invalide : %d\n", coup_saisi);
                } else {
                    player_black(&j_noir, src, dst);
                    tour++;
                }
            }

            wprintf(L"\nAppuyez sur une touche pour continuer... (Echap pour quitter)\n");
            ch = getch_linux();

        } while (ch != 27);
    }

    free(piece);
    return 0;
}