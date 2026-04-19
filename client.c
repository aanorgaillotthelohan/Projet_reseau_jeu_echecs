#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <locale.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <wchar.h>

#include "chess.h"          /* board, joueur, eliminated_pieces_*, … */
#include "application.h"    /* (inclus pour la cohérence du projet)   */
#include "protocol.h"       /* Message, MSG_*, MSG_DATA_MAX            */
#include "pieces.h"
#include "display.h"
typedef struct {
    char choix;           /* 'c', 'i', 'j', 's', 'h', '!' */
    char idJoueur[128];
    char Mdp[128];
} MessageClient;


int dsock_mec;                 /* socket principal vers le serveur */
volatile int enRecherche = 0;  /* 1 = en attente de match          */
volatile int matchTrouve = 0;  /* [NEW] partagé avec le listener   */


static void gestion_erreur(int res, const char *chaine)
{
    if (res == -1) { perror(chaine); exit(1); }
}

/* [CES] — Vide le buffer stdin jusqu'au '\n' */
static void flush_stdin(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
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


static void *listener(void *arg)
{
    (void)arg;
    while (1) {
        if (enRecherche) {
            int rep = 0;
            ssize_t r = recv(dsock_mec, &rep, sizeof(rep), 0);
            if (r <= 0) {
                printf("\nDéconnecté du serveur.\n");
                exit(1);
            }
            if (rep == 1) {
                printf("\n=== Match trouvé ! La partie va commencer… ===\n");
                matchTrouve = 1;   /* [NEW] signale au main */
                enRecherche = 0;
            }
        } else {
            sleep(100000);   /* 100 ms — ne pas saturer le CPU [CV] */
        }
    }
    return NULL;
}


static void affiche_plateau_recu(const Message *m)
{
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

/* Marque et affiche les coups possibles sur le plateau local */
static void affiche_coups_recus(const Message *m, int src)
{
    printf("\u25B6 Coups possibles pour la pièce en %d :\n  ", src);
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

    /* Répéter la liste de façon compacte sans re-afficher le plateau */
    printf("\u25B6 Destinations disponibles : ");
    for (int i = 0; i < m->nb; i++)
        printf("[%d%d] ", m->data[i] / 10, m->data[i] % 10);
    printf("\n");
}

/* Efface les marqueurs 'x' posés lors de l'affichage */
static void efface_marqueurs(void)
{
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c] == 'x')
                board[r][c] = ' ';
}


static int tour_client(int sock, int player_id)
{
    const char *couleur = (player_id == 1) ? "Blanc" : "Noir";
    Message msg_out, msg_in;

    while (1) {

        /* ── Étape 1 : sélectionner la pièce ── */
        efface_marqueurs();
        display();

        printf("\n\u25CF %s — Sélectionnez la pièce (LigneColonne, ex: 63) : ", couleur);
        fflush(stdout);

        int src = -1;
        if (scanf("%d", &src) != 1) { flush_stdin(); continue; }
        flush_stdin();

        if (src < 0 || src > 77 || src % 10 > 7) {
            printf("\u26A0 Coordonnées invalides (ex: 63 = ligne 6, colonne 3)\n");
            printf("Appuyez sur Entrée…\n"); flush_stdin();
            continue;
        }

        /* Demander les coups légaux au serveur */
        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.type    = MSG_DEMANDE_COUPS;
        msg_out.data[0] = src;
        msg_out.nb      = 1;
        if (send_msg(sock, &msg_out) < 0) return -1;

        if (recv_msg(sock, &msg_in) < 0) return -1;

        if (msg_in.type == MSG_COUP_INVALIDE || msg_in.nb == 0) {
            printf("\u26A0 Pas de pièce valide en %d ou aucun coup légal.\n", src);
            printf("Appuyez sur Entrée…\n"); flush_stdin();
            continue;
        }

        affiche_coups_recus(&msg_in, src);

        /* ── Étape 2 : saisir la destination ── */
        printf("\n\u25CF %s — Destination (ou 0 pour rechoisir la pièce) : ", couleur);
        fflush(stdout);

        int dst = -1;
        if (scanf("%d", &dst) != 1) { flush_stdin(); continue; }
        flush_stdin();

        if (dst == 0) continue;   /* rechoisir la pièce */

        if (dst < 0 || dst > 77 || dst % 10 > 7) {
            printf("\u26A0 Coordonnées invalides.\n");
            printf("Appuyez sur Entrée…\n"); flush_stdin();
            continue;
        }

        /* Envoyer le coup */
        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.type    = MSG_JOUER_COUP;
        msg_out.data[0] = src;
        msg_out.data[1] = dst;
        msg_out.nb      = 2;
        if (send_msg(sock, &msg_out) < 0) return -1;

        /* Attendre la confirmation du serveur */
        while (1) {
            if (recv_msg(sock, &msg_in) < 0) return -1;

            if (msg_in.type == MSG_COUP_INVALIDE) {
                printf("\u26A0 Coup invalide, réessayez.\n");
                printf("Appuyez sur Entrée…\n"); flush_stdin();
                break;   /* retour à la sélection de pièce */
            }
            if (msg_in.type == MSG_COUP_OK) {
                continue;   /* attendre MSG_ETAT_PLATEAU */
            }
            if (msg_in.type == MSG_ECHEC_BLANC) {
                printf("\n\u26A0\u26A0  ÉCHEC AU ROI BLANC !  \u26A0\u26A0\n");
                continue;
            }
            if (msg_in.type == MSG_ECHEC_NOIR) {
                printf("\n\u26A0\u26A0  ÉCHEC AU ROI NOIR !  \u26A0\u26A0\n");
                continue;
            }
            if (msg_in.type == MSG_ETAT_PLATEAU) {
                efface_marqueurs();
                affiche_plateau_recu(&msg_in);
                printf("\u2705 Coup joué : %d → %d\n", src, dst);
                return 0;   /* tour terminé avec succès */
            }
        }
        /* coup invalide → reboucle sur la sélection de pièce */
    }
}

 

static void boucle_jeu(int sock)
{
    Message msg;

    while (1) {
        if (recv_msg(sock, &msg) < 0) {
            printf("Connexion perdue.\n");
            return;
        }

        /* Informations de couleur envoyées en début de partie */
        if (msg.type == MSG_BLANC) {
            printf("\u2654 Vous jouez les BLANCS.\n");
            continue;
        }
        if (msg.type == MSG_NOIR) {
            printf("\u265A Vous jouez les NOIRS.\n");
            continue;
        }

        /* Mise à jour du plateau (hors tour) */
        if (msg.type == MSG_ETAT_PLATEAU) {
            affiche_plateau_recu(&msg);
            continue;
        }

        /* C'est notre tour */
        if (msg.type == MSG_VOTRE_TOUR) {
            int player_id = msg.data[0];
            const char *c = (player_id == 1) ? "BLANCS" : "NOIRS";
            printf("\n══ Tour des %s ══\n", c);

            if (tour_client(sock, player_id) < 0) {
                printf("Connexion perdue.\n");
                return;
            }
            continue;
        }

        /* Messages inattendus ignorés silencieusement */
    }
}


int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");   /* [CES] */

    if (argc < 2) {
        printf("Usage : %s <port>\n", argv[0]);
        return 1;
    }

    /* Initialiser les globaux échecs [CES] */
    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;
    piece = (char *)malloc(sizeof(char));
    board[7][7] = 'R';

    /* Connexion au serveur [CV] */
    dsock_mec = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(dsock_mec, "socket");

    struct sockaddr_in adServ;
    adServ.sin_family = AF_INET;
    adServ.sin_port   = htons(atoi(argv[1]));
    gestion_erreur(inet_pton(AF_INET, "127.0.0.1", &adServ.sin_addr), "inet_pton");
    gestion_erreur(connect(dsock_mec, (struct sockaddr *)&adServ, sizeof(adServ)), "connect");

    printf("Connecté au serveur sur le port %s.\n", argv[1]);
    printf("Vous pouvez quitter avec '!'\n");

    /* Lancer le listener [CV] */
    pthread_t tid_listener;
    pthread_create(&tid_listener, NULL, listener, NULL);
    pthread_detach(tid_listener);

    char idJoueur[128], Mdp[128];
    char choix;
    int  connecte    = 0;
    int  jeuEnCours  = 1;

    while (jeuEnCours) {

        // authentification
        while (!connecte) {
            printf("Choix (j=jouer / s=spectateur / h=historique / !=quitter) : ");
            scanf(" %c", &choix);
            choix = tolower(choix);
            flush_stdin();

            if (choix == '!') { jeuEnCours = 0; break; }

            if (choix == 'i') {
                printf("Identifiant : "); scanf("%s", idJoueur); flush_stdin();
                printf("Mot de passe : "); scanf("%s", Mdp);    flush_stdin();

                MessageClient msg = {'i', "", ""};
                strncpy(msg.idJoueur, idJoueur, 128);
                strncpy(msg.Mdp,      Mdp,      128);
                send(dsock_mec, &msg, sizeof(msg), 0);

                int rep = 0;
                recv(dsock_mec, &rep, sizeof(rep), 0);
                if (rep == 1) { printf("Inscription réussie ! Connecté.\n"); connecte = 1; }
                else            printf("Identifiant déjà pris.\n");
            }
            else if (choix == 'c') {
                printf("Identifiant : "); scanf("%s", idJoueur); flush_stdin();
                printf("Mot de passe : "); scanf("%s", Mdp);    flush_stdin();

                MessageClient msg = {'c', "", ""};
                strncpy(msg.idJoueur, idJoueur, 128);
                strncpy(msg.Mdp,      Mdp,      128);
                send(dsock_mec, &msg, sizeof(msg), 0);

                int rep = 0;
                recv(dsock_mec, &rep, sizeof(rep), 0);
                if (rep == 1) { printf("Connecté !\n"); connecte = 1; }
                else            printf("Erreur d'identification.\n");
            }
            else {
                printf("Choix invalide.\n");
            }
        }

        if (!jeuEnCours) break;

        // menu principal

        int choixValid = 0;
        while (!choixValid) {
            printf("Choix (j=jouer / s=spectateur / h=historique / !=quitter) : ");
            scanf(" %c", &choix);
            choix = tolower(choix);
            flush_stdin();

            if (choix == '!') {
                jeuEnCours = 0;
                /* Informer le serveur [CV] */
                MessageClient msg = {'!', "", ""};
                strncpy(msg.idJoueur, idJoueur, 128);
                send(dsock_mec, &msg, sizeof(msg), 0);
                break;
            }
            else if (choix == 'j' || choix == 's' || choix == 'h') {
                choixValid = 1;
            }
            else {
                printf("Choix invalide.\n");
            }
        }

        if (!jeuEnCours) break;


        MessageClient msg_choix = {choix, "", ""};
        strncpy(msg_choix.idJoueur, idJoueur, 128);
        send(dsock_mec, &msg_choix, sizeof(msg_choix), 0);


        if (choix == 'j') {
            printf("Recherche de partie… (appuyez sur 'r' + Entrée pour annuler)\n");
            enRecherche = 1;
            matchTrouve = 0;

            char input[16];
            while (enRecherche && !matchTrouve) {
                if (fgets(input, sizeof(input), stdin)) {
                    if (tolower(input[0]) == 'r') {
                        printf("Recherche annulée.\n");
                        enRecherche = 0;
                    }
                }
            }

            if (matchTrouve) {
                boucle_jeu(dsock_mec);
                /* Après la partie, permettre de rejouer */
                connecte   = 1;   /* rester connecté */
                matchTrouve = 0;
            }
        }
        else if (choix == 's') {
            printf("Mode spectateur non implémenté.\n");
        }
        else if (choix == 'h') {
            printf("Historique non implémenté.\n");
        }
    }

    close(dsock_mec);
    free(piece);
    return 0;
}
