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
#include <stdbool.h>
#include <time.h>


#include "chess.h"
#include "application.h"
#include "salon.h"
#include "protocol.h"
#include "display.h"
#include "players.h"
#include "pieces.h"

#define MAX_CLIENTS 50
#define PORT 8080

typedef struct {
    char choix;
    char idJoueur[128];
    char Mdp[128];
} MessageClient;

joueur JoueursEnAttente[MAX_CLIENTS];
int debut    = 0;
int fin      = 0;
int NbSalons = 0;

pthread_mutex_t mutexJoueurs = PTHREAD_MUTEX_INITIALIZER;


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

static void retirer_joueur_file(int sockClient) {
    pthread_mutex_lock(&mutexJoueurs);
    int index = -1;
    for (int i = debut; i < fin; i++) {
        if (JoueursEnAttente[i].socketJoueur == sockClient) {
            index = i;
            break;
        }
    }
    if (index != -1) {
        for (int i = index; i < fin - 1; i++)
            JoueursEnAttente[i] = JoueursEnAttente[i + 1];
        fin--;
    }
    pthread_mutex_unlock(&mutexJoueurs);
}

static void build_etat_plateau(Message *m) {
    memset(m, 0, sizeof(Message));
    m->type = MSG_ETAT_PLATEAU;
    int idx = 0;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            m->data[idx++] = (int)(unsigned char)board[r][c];

    int nb_w = 0;
    eliminated_piece *p = eliminated_pieces_white.start;
    while (p) { nb_w++; p = p->next; }
    m->data[64] = nb_w;

    int pos = 66;
    p = eliminated_pieces_white.start;
    while (p && pos < MSG_DATA_MAX) {
        m->data[pos++] = (int)(unsigned char)p->piece_type;
        p = p->next;
    }

    int nb_b = 0;
    p = eliminated_pieces_black.start;
    while (p) { nb_b++; p = p->next; }
    m->data[65] = nb_b;

    p = eliminated_pieces_black.start;
    while (p && pos < MSG_DATA_MAX) {
        m->data[pos++] = (int)(unsigned char)p->piece_type;
        p = p->next;
    }
    m->nb = pos;
}


static void *gere_partie(void *arg) {
    Args_gerepartie *args = (Args_gerepartie *)arg;
    // essai
    int sock_blanc, sock_noir;
    int dmec     = args->sock_1;
    int dmec_bis = args->sock_2;
    free(arg);

    /* Réinitialisation du plateau */
    memset(board, ' ', sizeof(board));
    board[0][0]='r'; board[0][1]='h'; board[0][2]='c'; board[0][3]='q';
    board[0][4]='k'; board[0][5]='c'; board[0][6]='h'; board[0][7]='r';
    for (int c = 0; c < 8; c++) board[1][c] = 'p';
    for (int c = 0; c < 8; c++) board[6][c] = 'P';
    board[7][0]='R'; board[7][1]='H'; board[7][2]='C'; board[7][3]='Q';
    board[7][4]='K'; board[7][5]='C'; board[7][6]='H'; board[7][7]='R';

    white_king_moved = false; black_king_moved = false;
    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;

    joueur j_blanc, j_noir;
    memset(&j_blanc, 0, sizeof(joueur));
    memset(&j_noir,  0, sizeof(joueur));
    strncpy(j_blanc.Couleur, "blanc", 20);
    strncpy(j_noir.Couleur,  "noir",  20);
    
    Message ta_couleur;
    int first_co;

    if (rand() % 2) {
        sock_blanc = dmec; sock_noir = dmec_bis; first_co = 1;
    } else {
        sock_blanc = dmec_bis; sock_noir = dmec; first_co = 0;
    }

    ta_couleur.type = MSG_BLANC; ta_couleur.nb = 1;
    send_msg(sock_blanc, &ta_couleur);
    ta_couleur.type = MSG_NOIR; ta_couleur.nb = 1;
    send_msg(sock_noir, &ta_couleur);

    int tour = 1;
    Message msg_in, msg_out, plateau;
    while (1) {
        int player_id = (tour % 2 == 1) ? 1 : 2;
        int sock_current = (player_id == 1) ? sock_blanc : sock_noir;
        joueur *j = (player_id == 1) ? &j_blanc : &j_noir;
        
        //envoie de l'etat
        build_etat_plateau(&plateau);
        if (send_msg(sock_current, &plateau) < 0) return NULL;
        
        // previent le joueur de son tour
        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.type = MSG_VOTRE_TOUR;
        msg_out.data[0] = player_id;
        msg_out.nb = 1;
        if (send_msg(sock_current, &msg_out) < 0) return NULL;

        int coup_valide = 0;
        while (!coup_valide) {
            if (recv_msg(sock_current, &msg_in) < 0) return NULL;

            if (msg_in.type == MSG_DEMANDE_COUPS) {
                int src = msg_in.data[0];
                int r1, c1;
                parse_pos(src, &r1, &c1);

                (player_id == 1) ? compute_moves_white(r1, c1) : compute_moves_black(r1, c1);
                
                msg_out.type = MSG_REPONSE_COUPS;
                msg_out.nb = possible_moves_index;
                for (int i = 0; i < possible_moves_index; i++) {
                    int ri = possible_moves[i] / 10;
                    int ci = possible_moves[i] % 10;
                    msg_out.data[i] = (7 - ri) * 10 + ci;
                }
                send_msg(sock_current, &msg_out);
            }
            else if (msg_in.type == MSG_JOUER_COUP) {
                int src = msg_in.data[0];
                int dst = msg_in.data[1];
                j->coupDuTour = src * 100 + dst;
                int res = (player_id == 1) ? player_white(j, src, dst) : player_black(j, src, dst);

                if (res < 0) {
                    msg_out.type = MSG_COUP_INVALIDE;
                    send_msg(sock_current, &msg_out);
                } else {
                    msg_out.type = MSG_COUP_OK;
                    send_msg(sock_current, &msg_out);

                    Message plateau_maj;
                    build_etat_plateau(&plateau_maj);
                    send_msg(sock_blanc, &plateau_maj); 
                    send_msg(sock_noir, &plateau_maj);  

                    tour++;
                    coup_valide = 1;
                }
            }
        }
    }
    return NULL;
}

static void *gestion_client(void *arg) {
    int sockClient = *(int *)arg;
    free(arg);
    MessageClient msg;

    while (recv(sockClient, &msg, sizeof(msg), 0) > 0) {
        int rep = 0;
        if (msg.choix == '!') {
            retirer_joueur_file(sockClient);
            close(sockClient);
            return NULL;
        }
        else if (msg.choix == 'i') {
            rep = Inscription(msg.idJoueur, msg.Mdp);
            send(sockClient, &rep, sizeof(rep), 0);
        }
        else if (msg.choix == 'c') {
            if (VerifConnexion(msg.idJoueur, msg.Mdp)) rep = 1;
            send(sockClient, &rep, sizeof(rep), 0);
        }
        else if (msg.choix == 'j') {
            joueur *j = creejoueur(sockClient, msg.idJoueur);
            pthread_mutex_lock(&mutexJoueurs);
            if (fin < MAX_CLIENTS) JoueursEnAttente[fin++] = *j;
            
            if (fin - debut >= 2) {
                joueur j1 = JoueursEnAttente[debut];
                joueur j2 = JoueursEnAttente[debut + 1];
                debut += 2;
                NbSalons++;
                creeSalon(NbSalons, &j1, &j2);
                
                int matchTrouve = 1;
                send(j1.socketJoueur, &matchTrouve, sizeof(int), 0);
                send(j2.socketJoueur, &matchTrouve, sizeof(int), 0);

                Args_gerepartie *args = malloc(sizeof(Args_gerepartie));
                args->sock_1 = j1.socketJoueur;
                args->sock_2 = j2.socketJoueur;

                pthread_t t;
                pthread_create(&t, NULL, gere_partie, args);
                pthread_detach(t);
            }
            pthread_mutex_unlock(&mutexJoueurs);
            free(j);
            // CRITIQUE : On quitte ce thread pour ne plus interférer avec gere_partie
            return NULL; 
        }
    }
    return NULL;
}

/* ─── Main unique ───────────────────────────────────────────────────────── */

int main(void) {
    setlocale(LC_ALL, "");
    srand(time(NULL));

    int dsock = socket(PF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(dsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in ad = {0};
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(PORT);

    bind(dsock, (struct sockaddr *)&ad, sizeof(ad));
    listen(dsock, MAX_CLIENTS);

    printf("Serveur d'échecs en écoute sur le port %d...\n", PORT);

    while (1) {
        struct sockaddr_in adClient;
        socklen_t lgA = sizeof(adClient);
        int *sockClient = malloc(sizeof(int));
        *sockClient = accept(dsock, (struct sockaddr *)&adClient, &lgA);

        pthread_t tid;
        pthread_create(&tid, NULL, gestion_client, sockClient);
        pthread_detach(tid);
    }

    close(dsock);
    return 0;
}