#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <locale.h>
#include <ctype.h>
#include <stdbool.h>

/* Headers de votre projet */
#include "chess.h"
#include "display.h"
#include "pieces.h"
#include "protocol.h"
#include "application.h"

/* ─── Structures ────────────────────────────────────────────────────────── */
typedef struct {
    char choix;    
    char idJoueur[128];
    char Mdp[128];
} MessageClient;

/* ─── Variables Globales ────────────────────────────────────────────────── */
int dsock_mec;
volatile int enRecherche = 0;

/* ─── Utilitaires Réseau ────────────────────────────────────────────────── */
static void gestion_erreur(int res, const char *chaine) {
    if (res == -1) { perror(chaine); exit(1); }
}

static int send_msg(int sock, const Message *m) {
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

static int recv_msg(int sock, Message *m) {
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

static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/* ─── Logique d'affichage ───────────────────────────────────────────────── */
static void affiche_plateau_recu(const Message *m) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            board[r][c] = (char)m->data[r * 8 + c];

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

static void efface_marqueurs(void) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c] == 'x') board[r][c] = ' ';
}

static void affiche_coups_recus(const Message *m, int src) {
    printf("\u25B6 Coups possibles pour %d :\n", src);
    for (int i = 0; i < m->nb; i++) {
        int label = m->data[i] / 10;
        int col   = m->data[i] % 10;
        int r     = 7 - label;
        if (board[r][col] == ' ') board[r][col] = 'x';
        printf("[%d%d] ", label, col);
    }
    printf("\n");
    display();
}

/* ─── Gestion du Tour ───────────────────────────────────────────────────── */
// --- Dans client.c ---

static int tour_client(int sock, int player_id) {
    const char *couleur = (player_id == 1) ? "Blanc" : "Noir";
    Message msg_out, msg_in;

    while (1) {
        efface_marqueurs();
        display();
        printf("\n\u25CF Tour des %s \u25CF\n", couleur);
        printf("Pièce (ex: 11) ou 0 pour quitter : ");
        
        int src;
        if (scanf("%d", &src) != 1) { flush_stdin(); continue; }
        if (src == 0) return -1;

        // 1. Demander les coups possibles
        msg_out.type = MSG_DEMANDE_COUPS;
        msg_out.data[0] = src;
        msg_out.nb = 1;
        if (send_msg(sock, &msg_out) < 0) return -1;

        // Attendre la réponse (ignorer les états de plateau parasites)
        do {
            if (recv_msg(sock, &msg_in) < 0) return -1;
        } while (msg_in.type != MSG_REPONSE_COUPS && msg_in.type != MSG_COUP_INVALIDE);

        if (msg_in.type == MSG_COUP_INVALIDE || msg_in.nb == 0) {
            printf("Aucun coup possible pour cette pièce.\n");
            continue;
        }

        // 2. Afficher et demander destination
        affiche_coups_recus(&msg_in, src);
        printf("Destination (ex: 31) ou -1 pour changer : ");
        int dst;
        if (scanf("%d", &dst) != 1) { flush_stdin(); continue; }
        if (dst == -1) continue;

        msg_out.type = MSG_JOUER_COUP;
        msg_out.data[0] = src;
        msg_out.data[1] = dst;
        if (send_msg(sock, &msg_out) < 0) return -1;

        // 3. Attendre validation
        while (1) {
            if (recv_msg(sock, &msg_in) < 0) return -1;
            
            if (msg_in.type == MSG_COUP_OK) {
                continue; // On attend encore le plateau mis à jour
            }
            if (msg_in.type == MSG_ETAT_PLATEAU) {
                affiche_plateau_recu(&msg_in);
                return 0; // TOUR FINI
            }
            if (msg_in.type == MSG_COUP_INVALIDE) {
                printf("Mouvement illégal !\n");
                break; // Repart au début de la boucle pour redemander src
            }
        }
    }
}

/* ─── Main ──────────────────────────────────────────────────────────────── */
int main(int argc, char **argv) {
    setlocale(LC_ALL, "");
    if (argc < 2) { printf("Usage: %s <port>\n", argv[0]); return 1; }

    dsock_mec = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in adServ = { .sin_family = AF_INET, .sin_port = htons(atoi(argv[1])) };
    inet_pton(AF_INET, "127.0.0.1", &adServ.sin_addr);
    connect(dsock_mec, (struct sockaddr*)&adServ, sizeof(adServ));

    char idJoueur[128], Mdp[128], choix;
    int connecte = 0;

    // 1. Phase Auth
    while (!connecte) {
        printf("Connexion (c) / Inscription (i) / Quitter (!) : ");
        scanf(" %c", &choix);
        if (choix == '!') exit(0);

        printf("ID : "); scanf("%s", idJoueur);
        printf("MDP : "); scanf("%s", Mdp);

        MessageClient auth = {0};
        auth.choix = choix;
        strcpy(auth.idJoueur, idJoueur);
        strcpy(auth.Mdp, Mdp);
        send(dsock_mec, &auth, sizeof(auth), 0);

        int rep;
        recv(dsock_mec, &rep, sizeof(rep), 0);
        if (rep == 1) connecte = 1;
        else printf("Erreur auth.\n");
    }

    // 2. Menu Principal
    printf("Connecté ! (j) pour chercher une partie : ");
    scanf(" %c", &choix);
    MessageClient menu = {0};
    menu.choix = choix;
    strcpy(menu.idJoueur, idJoueur);
    send(dsock_mec, &menu, sizeof(menu), 0);

    if (choix == 'j') {
        printf("Attente d'un adversaire...\n");
        int matchTrouve = 0;
        recv(dsock_mec, &matchTrouve, sizeof(matchTrouve), 0);
        if (matchTrouve == 1) {
            printf("Partie trouvée !\n");
            
            // 3. Boucle de Jeu
            Message msg;
            while (recv_msg(dsock_mec, &msg) >= 0) {
                if (msg.type == MSG_BLANC) {printf("Vous jouez les BLANCS\n");} 
                else if (msg.type == MSG_NOIR) {printf("Vous jouez les NOIRS\n");}
                else if (msg.type == MSG_ETAT_PLATEAU) {affiche_plateau_recu(&msg);}
                else if (msg.type == MSG_VOTRE_TOUR) {
                    if (tour_client(dsock_mec, msg.data[0]) < 0) {
                        printf("big flop là\n");
                        break;}
                }
            }
        }
    }

    close(dsock_mec);
    return 0;
}