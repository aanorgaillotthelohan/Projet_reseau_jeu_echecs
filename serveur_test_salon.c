#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <locale.h>

#include "chess.h"
#include "display.h"
#include "players.h"

/* ─── Utilitaire erreur ──────────────────────────────────────────────────── */
static void gestion_erreur(int res, const char *chaine)
{
    if (res == -1) {
        printf("Erreur lors de %s\n", chaine);
        exit(1);
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  main  –  Serveur de jeu d'échecs
 *
 *  Boucle : attend un coup réseau (entier XYZW) → parse_coup → display →
 *           player_white ou player_black selon le tour → tour suivant.
 *
 *  Format du coup envoyé par le client : entier 4 chiffres XYZW
 *    XY = position source      (ligne*10 + col)
 *    ZW = position destination (ligne*10 + col)
 *  Exemple : 6374 = pion blanc en (6,3) va en (7,4) [notation affichée]
 * ════════════════════════════════════════════════════════════════════════════ */
int main(void)
{
    setlocale(LC_ALL, "");

    /* ── Initialisation du plateau ── */
    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;
    piece = (char *)malloc(sizeof(char));
    board[7][7] = 'R';

    /* ── Initialisation des joueurs ── */
    joueur j_blanc, j_noir;
    memset(&j_blanc, 0, sizeof(joueur));
    memset(&j_noir,  0, sizeof(joueur));
    strncpy(j_blanc.Couleur, "blanc", 20);
    strncpy(j_noir.Couleur,  "noir",  20);

    /* ── Création de la socket serveur ── */
    int dsock = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(dsock, "socket");

    int opt = 1;
    setsockopt(dsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in ad;
    ad.sin_family      = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port        = htons(8080);

    int res = bind(dsock, (struct sockaddr *)&ad, sizeof(ad));
    gestion_erreur(res, "bind");

    res = listen(dsock, 5);
    gestion_erreur(res, "listen");

    printf("Serveur d'echecs en attente sur le port 8080...\n");
    display();

    struct sockaddr_in adClient;
    socklen_t lgA = sizeof(struct sockaddr_in);
    int tour = 1;  /* 1 = blanc, 2 = noir */

    /* ════════════════════════════════════════════════════════════════════════
     *  Boucle principale : chaque connexion = un coup
     * ════════════════════════════════════════════════════════════════════════ */
    while (1) {
        int dmec = accept(dsock, (struct sockaddr *)&adClient, &lgA);
        gestion_erreur(dmec, "accept");

        /* ── Recevoir le coup (entier) ── */
        int coup_recu = 0;
        ssize_t nb = recv(dmec, &coup_recu, sizeof(coup_recu), 0);

        if (nb <= 0) {
            printf("Client déconnecté\n");
            close(dmec);
            continue;
        }

        printf("Coup reçu : %d  (tour %d : %s)\n",
               coup_recu, tour, (tour % 2 != 0) ? "blanc" : "noir");

        /* ── Décoder le coup ── */
        joueur *j = (tour % 2 != 0) ? &j_blanc : &j_noir;
        j->coupDuTour = coup_recu;

        int src, dst;
        if (parse_coup(j, &src, &dst) < 0) {
            printf("⚠ Coup invalide : %d (format attendu XYZW)\n", coup_recu);
            close(dmec);
            continue;
        }

        /* ── Afficher le plateau ── */
        remove_possible_moves();
        system("clear");
        display();

        /* ── Jouer le coup ── */
        int ok;
        if (tour % 2 != 0)
            ok = player_white(&j_blanc, src, dst);
        else
            ok = player_black(&j_noir, src, dst);

        if (ok == 0)
            tour++;   /* coup valide → passer au tour suivant */

        close(dmec);
    }

    close(dsock);
    free(piece);
    return 0;
}