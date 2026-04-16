#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <locale.h>
#include <wchar.h>

#include "chess.h"
#include "display.h"
#include "pieces.h"
#include "protocol.h"
#include "application.h"
void gestion_erreur(int res, char* chaine){
    if(res == -1 ){
        printf("Erreur lors de %s \n", chaine);
        exit(1);
    }
}

static void gestion_erreur(int res, const char *chaine)
{
    if (res == -1) { perror(chaine); exit(1); }
}


static int send_msg(int sock, const Message *m)
{
    ssize_t total = 0;
    ssize_t size  = (ssize_t)sizeof(Message);
    const char *ptr = (const char *)m;
    while (total < size) {
        ssize_t n = send(sock, ptr + total, (size_t)(size - total), 0);
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

static int recv_msg(int sock, Message *m)
{
    ssize_t total = 0;
    ssize_t size  = (ssize_t)sizeof(Message);
    char *ptr = (char *)m;
    while (total < size) {
        ssize_t n = recv(sock, ptr + total, (size_t)(size - total), 0);
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

/* ─── Vide le buffer stdin ───────────────────────────────────────────────── */
static void flush_stdin(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* ─── Reconstruit et affiche le plateau reçu ────────────────────────────── */
static void affiche_plateau_recu(const Message *m)
{
    /* Reconstruire board local depuis data[0..63] */
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            board[r][c] = (char)m->data[r * 8 + c];

    /* Reconstruire les listes de pièces éliminées */
    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;

    int nb_w = m->data[64];
    int nb_b = m->data[65];
    int pos  = 66;

    for (int i = 0; i < nb_w && pos < MSG_DATA_MAX; i++, pos++)
        insert_eliminated_list((char)m->data[pos], &eliminated_pieces_white);
    for (int i = 0; i < nb_b && pos < MSG_DATA_MAX; i++, pos++)
        insert_eliminated_list((char)m->data[pos], &eliminated_pieces_black);


    display();
}


static void affiche_coups_recus(const Message *m, int src)
{
    printf("\u25B6 Coup(s) possible(s) avec cette pièce %d :\n  ", src);

    /* Marquer les cases sur le plateau local */
    for (int i = 0; i < m->nb; i++) {
        int label = m->data[i] / 10;
        int col   = m->data[i] % 10;
        int r     = 7 - label;
        printf("[%d%d] ", label, col);
        if (board[r][col] == ' ')
            board[r][col] = 'x';
    }
    printf("\n");
    display();

    printf("\u25B6 Coup(s) possible(s) avec cette pièce %d :\n  ", src);
    for (int i = 0; i < m->nb; i++)
        printf("[%d%d] ", m->data[i] / 10, m->data[i] % 10);
    printf("\n");
}

/* ─── Efface les marqueurs 'x' du plateau local ─────────────────────────── */
static void efface_marqueurs(void)
{
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c] == 'x')
                board[r][c] = ' ';
}

/* ════════════════════════════════════════════════════════════════════════════
 *  tour_client  —  gère un tour complet en 2 temps.
 *  Retourne 0 si le coup a été joué, -1 si connexion perdue.
 * ════════════════════════════════════════════════════════════════════════════ */
static int tour_client(int sock, int player_id)
{
    const char *couleur = (player_id == 1) ? "Blanc" : "Noir";
    Message msg_out, msg_in;

    while (1) {

        /* ── Étape 1 : saisie de la pièce source ── */
        efface_marqueurs();
        display();

        printf("\n\u25CF %s — Selectionnez la pièce à jouer, suivre le format ligne puis colonne (ex: 11): ", couleur);
        fflush(stdout);

        int src = -1;
        if (scanf("%d", &src) != 1) { flush_stdin(); continue; }
        flush_stdin();

        if (src < 0 || src > 77 || src % 10 > 7) {
            printf("\u26A0 Coordonnees invalides. Format : LigneColonne (ex: 11)\n");
            printf("Appuyez sur Entree...\n"); flush_stdin();
            continue;
        }

        /* Envoyer la demande de coups au serveur */
        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.type    = MSG_DEMANDE_COUPS;
        msg_out.data[0] = src;
        msg_out.nb      = 1;
        if (send_msg(sock, &msg_out) < 0) return -1;

        /* Recevoir la réponse */
        if (recv_msg(sock, &msg_in) < 0) return -1;

        if (msg_in.type == MSG_COUP_INVALIDE || msg_in.nb == 0) {
            printf("\u26A0 Pas de piece valide en %d ou aucun coup legal.\n", src);
            printf("Appuyez sur Entree...\n"); flush_stdin();
            continue;
        }

        /* Afficher le plateau avec les coups possibles */
        affiche_coups_recus(&msg_in, src);

        /* ── Étape 2 : saisie de la destination ── */
        printf("\n\u25CF %s — Entrez la destination (ou 0 pour changer de piece) : ", couleur);
        fflush(stdout);

        int dst = -1;
        if (scanf("%d", &dst) != 1) { flush_stdin(); continue; }
        flush_stdin();

        if (dst == 0) continue;   /* rechoisir la pièce */

        if (dst < 0 || dst > 77 || dst % 10 > 7) {
            printf("\u26A0 Coordonnees invalides.\n");
            printf("Appuyez sur Entree...\n"); flush_stdin();
            continue;
        }


        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.type    = MSG_JOUER_COUP;
        msg_out.data[0] = src;
        msg_out.data[1] = dst;
        msg_out.nb      = 2;
        if (send_msg(sock, &msg_out) < 0) return -1;


        while (1) {
            if (recv_msg(sock, &msg_in) < 0) return -1;

            if (msg_in.type == MSG_COUP_INVALIDE) {
                printf("\u26A0 Coup invalide, reessayez.\n");
                printf("Appuyez sur Entree...\n"); flush_stdin();
                break;   
            }
            if (msg_in.type == MSG_COUP_OK) {
                continue;   // attendre MSG_ETAT_PLATEAU 
            }
            if (msg_in.type == MSG_ECHEC_BLANC) {
                printf("\n\u26A0\u26A0  ECHEC AU ROI BLANC !  \u26A0\u26A0\n");
                continue;
            }
            if (msg_in.type == MSG_ECHEC_NOIR) {
                printf("\n\u26A0\u26A0  ECHEC AU ROI NOIR !  \u26A0\u26A0\n");
                continue;
            }
            if (msg_in.type == MSG_ETAT_PLATEAU) {
                efface_marqueurs();
                affiche_plateau_recu(&msg_in);
                printf("\u2705 Coup joue : %d -> %d\n", src, dst);
                return 0;   // tour terminé 
            }
        }
        /* Si on arrive ici c'est un coup invalide → on reboucle */
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  main
 * ════════════════════════════════════════════════════════════════════════════ */
int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    if (argc < 2) {
        printf("Usage : %s <port>\n", argv[0]);
        return 1;
    }

    /* Initialiser le board local (sera écrasé par le serveur) */
    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;
    piece = (char *)malloc(sizeof(char));
    board[7][7] = 'R';

    int port = atoi(argv[1]);
    printf("Connexion au serveur sur le port %d...\n", port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(sock, "socket");

    struct sockaddr_in adServ;
    adServ.sin_family = AF_INET;
    adServ.sin_port   = htons(port);
    gestion_erreur(inet_pton(AF_INET, "127.0.0.1", &adServ.sin_addr), "inet_pton");
    gestion_erreur(connect(sock, (struct sockaddr *)&adServ, sizeof(adServ)), "connect");

    printf("Connecte au serveur.\n");

    Message msg;

    while (1) {
        /* Recevoir l'état du plateau */
        if (recv_msg(sock, &msg) < 0) {
            printf("Connexion perdue.\n");
            break;
        }
        if (msg.type == MSG_BLANC){
            printf("Couleur de cette partie : Blanc");
        }
        if (msg.type == MSG_NOIR){
            printf("Couleur de cette partie : Noir");
        }
        if (msg.type == MSG_ETAT_PLATEAU) {
            affiche_plateau_recu(&msg);
            printf("affichage réussi\n");
            continue;
        }

        if (msg.type == MSG_VOTRE_TOUR) {
            int player_id = msg.data[0];
            const char *couleur = (player_id == 1) ? "BLANC" : "NOIR";
            printf("\n══ C'est au tour des %s ══\n", couleur);

            if (tour_client(sock, player_id) < 0) {
                printf("Connexion perdue.\n");
                break;
            }
            continue;
        }

        /* Messages inattendus — on les ignore */
    }

    close(sock);
    free(piece);
    return 0;
}