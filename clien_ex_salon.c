#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * Client d'échecs  –  envoie des coups au serveur.
 * Usage : ./client <port>
 *
 * Coup : entier XYZW (4 chiffres)
 *   XY = position source      (ligne affichée * 10 + colonne)
 *   ZW = position destination (ligne affichée * 10 + colonne)
 * Exemple : 6374  → pièce en (ligne 6, col 3) va en (ligne 7, col 4)
 */

static void gestion_erreur(int res, const char *chaine)
{
    if (res == -1) {
        printf("Erreur lors de %s\n", chaine);
        exit(1);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage : %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    printf("Connexion au serveur sur le port %d...\n", port);

    int dsock = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(dsock, "socket");

    struct sockaddr_in adServ;
    adServ.sin_family = AF_INET;
    adServ.sin_port   = htons(port);
    int res = inet_pton(AF_INET, "127.0.0.1", &adServ.sin_addr);
    gestion_erreur(res, "inet_pton");

    res = connect(dsock, (struct sockaddr *)&adServ, sizeof(adServ));
    gestion_erreur(res, "connect");

    /* ── Boucle d'envoi de coups ── */
    while (1) {
        printf("Entrez votre coup (XYZW, ex: 6253) ou 0 pour quitter : ");
        int coup = 0;
        if (scanf("%d", &coup) != 1) break;
        if (coup == 0) break;

        /* Chaque coup = une connexion distincte au serveur */
        close(dsock);
        dsock = socket(PF_INET, SOCK_STREAM, 0);
        gestion_erreur(dsock, "socket (reconnexion)");
        res = connect(dsock, (struct sockaddr *)&adServ, sizeof(adServ));
        gestion_erreur(res, "connect (reconnexion)");

        ssize_t nb = send(dsock, &coup, sizeof(coup), 0);
        gestion_erreur((int)nb, "send");

        printf("Coup %d envoyé.\n", coup);
    }

    close(dsock);
    return 0;
}