#include <locale.h>
#include "chess.h"
#include "display.h"
#include "players.h"
#include "utils.h"
#include "showcase.h"

/* ─── Vide le buffer stdin ──────────────────────────────────────────────── */
static void flush_stdin(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* ════════════════════════════════════════════════════════════════════════════
 *  tour_joueur
 *  Gère un tour complet pour player_id (1=blanc, 2=noir) en 2 temps :
 *    1. Saisie de la pièce source → affichage des coups possibles
 *    2. Saisie de la destination → exécution
 *  Si l'entrée est invalide à n'importe quelle étape, on redemande
 *  sans passer au joueur suivant.
 * ════════════════════════════════════════════════════════════════════════════ */
static void tour_joueur(joueur *j, int player_id)
{
    int r1, c1, r2, c2;
    int src_label, dst_label;

    const char *couleur = (player_id == 1) ? "Blanc" : "Noir";

    while (1) {  /* boucle jusqu'à un coup valide */

        /* ── Étape 1 : saisie de la pièce source ── */
        while (1) {
            system("clear");
            remove_possible_moves();
            display();

            if (player_id == 1 && is_in_check(1))
                wprintf(L"\n\u26A0\u26A0  ECHEC AU ROI BLANC !  \u26A0\u26A0\n");
            if (player_id == 2 && is_in_check(2))
                wprintf(L"\n\u26A0\u26A0  ECHEC AU ROI NOIR !  \u26A0\u26A0\n");

            wprintf(L"\n\u25CF %s — Selectionnez votre piece (ex: 13) : ", couleur);
            fflush(stdout);

            int saisie = -1;
            if (scanf("%d", &saisie) != 1) { flush_stdin(); continue; }
            flush_stdin();

            /* Conversion label → rangée interne */
            if (parse_pos(saisie, &r1, &c1) < 0) {
                wprintf(L"\u26A0 Coordonnees invalides : %d  (format LigneCol, ex: 13)\n", saisie);
                wprintf(L"Appuyez sur Entree pour reessayer...\n");
                flush_stdin();
                continue;
            }
            src_label = saisie;

            /* Calcul des coups légaux */
            int ok;
            if (player_id == 1) ok = compute_moves_white(r1, c1);
            else                 ok = compute_moves_black(r1, c1);

            if (ok < 0) {
                wprintf(L"Appuyez sur Entree pour reessayer...\n");
                flush_stdin();
                continue;
            }

            /* Afficher le plateau avec les cases possibles marquées */
            append_possible_moves();
            system("clear");
            display();

            /* Afficher la liste lisible des destinations */
            wprintf(L"\u25B6 Cases disponibles pour la piece en %d :\n  ", src_label);
            for (int i = 0; i < possible_moves_index; i++) {
                int ri = possible_moves[i] / 10;
                int ci = possible_moves[i] % 10;
                int label_dst = 7 - ri;
                wprintf(L"[%d%d] ", label_dst, ci);
            }
            wprintf(L"\n");
            break;  /* pièce valide avec au moins un coup → étape 2 */
        }

        /* ── Étape 2 : saisie de la destination ── */
        wprintf(L"\n\u25CF %s — Entrez la destination (ou 0 pour changer de piece) : ", couleur);
        fflush(stdout);

        int saisie_dst = -1;
        if (scanf("%d", &saisie_dst) != 1) { flush_stdin(); continue; }
        flush_stdin();

        /* Permettre de rechoisir la pièce */
        if (saisie_dst == 0) continue;

        if (parse_pos(saisie_dst, &r2, &c2) < 0) {
            wprintf(L"\u26A0 Coordonnees invalides : %d\n", saisie_dst);
            wprintf(L"Appuyez sur Entree pour reessayer...\n");
            flush_stdin();
            continue;
        }
        dst_label = saisie_dst;

        /* Valider que la destination est dans les coups légaux */
        if (verify_possible_move(r2 * 10 + c2)) {
            wprintf(L"\u26A0 Case %d non accessible pour cette piece. Reessayez.\n", dst_label);
            wprintf(L"Appuyez sur Entree...\n");
            flush_stdin();
            continue;
        }

        /* ── Exécution du coup ── */
        int res;
        if (player_id == 1) res = player_white(j, src_label, dst_label);
        else                 res = player_black(j, src_label, dst_label);

        if (res == 0) return;  /* coup joué → fin du tour */
        /* sinon on reboucle */
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  main
 * ════════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    setlocale(LC_ALL, "");

    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;
    piece = (char *)malloc(sizeof(char));
    board[7][7] = 'R';

    joueur j_blanc, j_noir;
    memset(&j_blanc, 0, sizeof(joueur));
    memset(&j_noir,  0, sizeof(joueur));
    strncpy(j_blanc.Couleur, "blanc", 20);
    strncpy(j_noir.Couleur,  "noir",  20);

    system("clear");
    intro();
    wprintf(L"\nAppuyez sur Espace pour le mode replay, ou toute autre touche pour jouer...\n");
    char showcase_option = getch_linux();
    system("clear");

    if (showcase_option == 32) {
        showcase_game();
        free(piece);
        return 0;
    }

    int tour = 1;
    char ch;

    do {
        if ((tour % 2) != 0)
            tour_joueur(&j_blanc, 1);
        else
            tour_joueur(&j_noir, 2);

        tour++;

        remove_possible_moves();
        system("clear");
        display();
        wprintf(L"\nAppuyez sur une touche pour continuer... (Echap pour quitter)\n");
        ch = getch_linux();

    } while (ch != 27);

    free(piece);
    return 0;
}