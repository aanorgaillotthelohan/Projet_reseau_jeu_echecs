#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <locale.h>

#include "chess.h"
#include "players.h"
#include "moves.h"
#include "pieces.h"
#include "display.h"
#include "protocol.h"
#include "application.h"

#define PORT        1061
#define MAX_CLIENTS 50

typedef struct {
    char choix;          
    char idJoueur[128];
    char Mdp[128];
} MessageClient;

typedef struct {
    int  sock;
    char nom[128];
} JoueurEnAttente;

 JoueurEnAttente file_attente[MAX_CLIENTS];
 int file_debut = 0;
 int file_fin   = 0;

 pthread_mutex_t mutex_file   = PTHREAD_MUTEX_INITIALIZER;
 pthread_cond_t  cond_partie  = PTHREAD_COND_INITIALIZER;

 int send_msg(int sock, const Message *m)
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

 int recv_msg(int sock, Message *m)
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

 void build_etat_plateau(Message *m,
                                eliminated_pieces_list *ep_w,
                                eliminated_pieces_list *ep_b,
                                char b[8][8])
{
    memset(m, 0, sizeof(Message));
    m->type = MSG_ETAT_PLATEAU;

    int idx = 0;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            m->data[idx++] = (int)(unsigned char)b[r][c];

    int nb_w = 0;
    eliminated_piece *p = ep_w->start;
    while (p) { nb_w++; p = p->next; }
    m->data[64] = nb_w;

    int pos = 66;
    p = ep_w->start;
    while (p && pos < MSG_DATA_MAX) {
        m->data[pos++] = (int)(unsigned char)p->piece_type;
        p = p->next;
    }

    int nb_b = 0;
    p = ep_b->start;
    while (p) { nb_b++; p = p->next; }
    m->data[65] = nb_b;

    p = ep_b->start;
    while (p && pos < MSG_DATA_MAX) {
        m->data[pos++] = (int)(unsigned char)p->piece_type;
        p = p->next;
    }

    m->nb = pos;
}

typedef struct {
    int sock1;  
    int sock2;   
    char nom1[128];
    char nom2[128];
} ArgPartie;


 void *thread_partie(void *arg)
{
    ArgPartie *a = (ArgPartie *)arg;
    int dmec     = a->sock1;
    int dmec_bis = a->sock2;
    free(a);

    char b[8][8];
    eliminated_pieces_list ep_w = { NULL };
    eliminated_pieces_list ep_b = { NULL };


    memset(b, ' ', sizeof(b));
    b[0][0]='r'; b[0][1]='h'; b[0][2]='c'; b[0][3]='q';
    b[0][4]='k'; b[0][5]='c'; b[0][6]='h'; b[0][7]='r';
    for (int c=0;c<8;c++) b[1][c]='p';
    for (int c=0;c<8;c++) b[6][c]='P';
    b[7][0]='R'; b[7][1]='H'; b[7][2]='C'; b[7][3]='Q';
    b[7][4]='K'; b[7][5]='C'; b[7][6]='H'; b[7][7]='R';


    memcpy(board, b, sizeof(board));
    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;

    white_king_moved=false; white_rook_left_moved=false; white_rook_right_moved=false;
    black_king_moved=false; black_rook_left_moved=false; black_rook_right_moved=false;
    en_passant_col=-1; en_passant_row=-1;
    en_passant_capture_row=-1; en_passant_capture_col=-1;

    srand((unsigned)time(NULL) ^ (unsigned)pthread_self());
    bool premier = rand() % 2;

    int sock_blanc, sock_noir;
    Message ta_couleur;

    if (premier) {
        sock_blanc = dmec;
        sock_noir  = dmec_bis;
    } else {
        sock_blanc = dmec_bis;
        sock_noir  = dmec;
    }

    memset(&ta_couleur, 0, sizeof(ta_couleur));
    ta_couleur.type = MSG_BLANC; ta_couleur.nb = 1;
    if (send_msg(sock_blanc, &ta_couleur) < 0) goto fin_partie;

    memset(&ta_couleur, 0, sizeof(ta_couleur));
    ta_couleur.type = MSG_NOIR; ta_couleur.nb = 1;
    if (send_msg(sock_noir, &ta_couleur) < 0) goto fin_partie;

    printf("Début de la partie\n");

    joueur j_blanc, j_noir;
    memset(&j_blanc, 0, sizeof(joueur)); strncpy(j_blanc.Couleur, "blanc", 20);
    memset(&j_noir,  0, sizeof(joueur)); strncpy(j_noir.Couleur,  "noir",  20);

    int tour = 1;

    while (1) {
        int player_id      = (tour % 2 == 1) ? 1 : 2;
        int sock_current   = (player_id == 1) ? sock_blanc : sock_noir;
        int sock_other     = (player_id == 1) ? sock_noir  : sock_blanc;
        joueur *j          = (player_id == 1) ? &j_blanc   : &j_noir;

        Message msg_in, msg_out, plateau;

        build_etat_plateau(&plateau,
                           &eliminated_pieces_white,
                           &eliminated_pieces_black,
                           board);
        if (send_msg(sock_current, &plateau) < 0) goto fin_partie;

        if (send_msg(sock_other, &plateau) < 0) goto fin_partie;

        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.type    = MSG_VOTRE_TOUR;
        msg_out.data[0] = player_id;
        msg_out.nb      = 1;
        if (send_msg(sock_current, &msg_out) < 0) goto fin_partie;

        while (1) {
            if (recv_msg(sock_current, &msg_in) < 0) {
                printf("Client a déco\n");
                goto fin_partie;
            }

            if (msg_in.type == MSG_DEMANDE_COUPS) {
                int src = msg_in.data[0];
                int r1, c1;
                memset(&msg_out, 0, sizeof(msg_out));

                if (parse_pos(src, &r1, &c1) < 0) {
                    msg_out.type = MSG_COUP_INVALIDE;
                    send_msg(sock_current, &msg_out);
                    continue;
                }

                int ok = (player_id == 1) ? compute_moves_white(r1, c1)
                                          : compute_moves_black(r1, c1);
                if (ok < 0) {
                    msg_out.type = MSG_COUP_INVALIDE;
                    send_msg(sock_current, &msg_out);
                    continue;
                }

                msg_out.type = MSG_REPONSE_COUPS;
                msg_out.nb   = possible_moves_index;
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
                int r1, c1;

                if (parse_pos(src, &r1, &c1) < 0) {
                    memset(&msg_out, 0, sizeof(msg_out));
                    msg_out.type = MSG_COUP_INVALIDE;
                    send_msg(sock_current, &msg_out);
                    continue;
                }

                int ok = (player_id == 1) ? compute_moves_white(r1, c1)
                                          : compute_moves_black(r1, c1);
                if (ok < 0) {
                    memset(&msg_out, 0, sizeof(msg_out));
                    msg_out.type = MSG_COUP_INVALIDE;
                    send_msg(sock_current, &msg_out);
                    continue;
                }

                int res = (player_id == 1) ? player_white(j, src, dst)
                                           : player_black(j, src, dst);
                if (res < 0) {
                    memset(&msg_out, 0, sizeof(msg_out));
                    msg_out.type = MSG_COUP_INVALIDE;
                    send_msg(sock_current, &msg_out);
                    continue;
                }

                memset(&msg_out, 0, sizeof(msg_out));
                msg_out.type = MSG_COUP_OK;
                send_msg(sock_current, &msg_out);

                if (is_in_check(1)) {
                    memset(&msg_out, 0, sizeof(msg_out));
                    msg_out.type = MSG_ECHEC_BLANC;
                    send_msg(sock_current, &msg_out);
                    send_msg(sock_other,   &msg_out);
                }
                if (is_in_check(2)) {
                    memset(&msg_out, 0, sizeof(msg_out));
                    msg_out.type = MSG_ECHEC_NOIR;
                    send_msg(sock_current, &msg_out);
                    send_msg(sock_other,   &msg_out);
                }

                build_etat_plateau(&plateau,
                                   &eliminated_pieces_white,
                                   &eliminated_pieces_black,
                                   board);
                send_msg(sock_current, &plateau);
                send_msg(sock_other,   &plateau);

                printf("Tour %d joué : %d -> %d\n", tour, src, dst);
                tour++;
                break;   
            }
        } 
    } 

fin_partie:
    printf("Partie terminée. En attente d'une nouvelle connexion...\n");
    close(dmec);
    close(dmec_bis);
    return NULL;
}


 void *thread_client(void *arg)
{
    int sockClient = *(int *)arg;
    free(arg);

    MessageClient msg;
    char idConnecte[128] = {0};
    int connecte = 0;


    while (!connecte) {
        ssize_t r = recv(sockClient, &msg, sizeof(msg), 0);
        if (r <= 0) {
            printf("Client déconnecté avant authentification\n");
            close(sockClient);
            return NULL;
        }

        if (msg.choix == '!') {
            close(sockClient);
            return NULL;
        }

        int rep = 0;

        if (msg.choix == 'i') {                        
            rep = Inscription(msg.idJoueur, msg.Mdp);
            send(sockClient, &rep, sizeof(rep), 0);
            if (rep == 1) {
                strncpy(idConnecte, msg.idJoueur, 128);
                connecte = 1;
                printf("Nouveau joueur inscrit et connecté : %s\n", idConnecte);
            }
        } else if (msg.choix == 'c') {                
            if (VerifConnexion(msg.idJoueur, msg.Mdp)) rep = 1;
            send(sockClient, &rep, sizeof(rep), 0);
            if (rep == 1) {
                strncpy(idConnecte, msg.idJoueur, 128);
                connecte = 1;
                printf("Joueur connecté : %s\n", idConnecte);
            }
        } else {
            
            send(sockClient, &rep, sizeof(rep), 0);
        }
    }

    while (1) {
        ssize_t r = recv(sockClient, &msg, sizeof(msg), 0);
        if (r <= 0) {
            printf("Client %s déconnecté\n", idConnecte);
            break;
        }

        if (msg.choix == '!') {
            printf("Client %s quitte\n", idConnecte);
            break;
        }

        if (msg.choix == 'j') {
          
            pthread_mutex_lock(&mutex_file);

            if (file_fin < MAX_CLIENTS) {
                file_attente[file_fin].sock = sockClient;
                strncpy(file_attente[file_fin].nom, idConnecte, 128);
                file_fin++;
                printf("Joueur %s en file d'attente (%d en attente)\n",
                       idConnecte, file_fin - file_debut);
            } else {
                printf("File pleine !\n");
                pthread_mutex_unlock(&mutex_file);
                continue;
            }


            if (file_fin - file_debut >= 2) {
                JoueurEnAttente j1 = file_attente[file_debut];
                JoueurEnAttente j2 = file_attente[file_debut + 1];
                file_debut += 2;


                if (file_debut > 0 && file_fin >= MAX_CLIENTS / 2) {
                    memmove(file_attente, &file_attente[file_debut],
                            (size_t)(file_fin - file_debut) * sizeof(JoueurEnAttente));
                    file_fin  -= file_debut;
                    file_debut = 0;
                }

                pthread_mutex_unlock(&mutex_file);

           
                int matchTrouve = 1;
                send(j1.sock, &matchTrouve, sizeof(matchTrouve), 0);
                send(j2.sock, &matchTrouve, sizeof(matchTrouve), 0);
                printf("Salon créé entre %s et %s\n", j1.nom, j2.nom);

                ArgPartie *ap = malloc(sizeof(ArgPartie));
                ap->sock1 = j1.sock;
                ap->sock2 = j2.sock;
                strncpy(ap->nom1, j1.nom, 128);
                strncpy(ap->nom2, j2.nom, 128);

                pthread_t tid;
                pthread_create(&tid, NULL, thread_partie, ap);
                pthread_detach(tid);
                return NULL;
            }

            pthread_mutex_unlock(&mutex_file);

            return NULL;   
        }
        
    }

    close(sockClient);
    return NULL;
}


int main(void)
{
    setlocale(LC_ALL, "");

    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;
    piece = (char *)malloc(sizeof(char));
    board[7][7] = 'R';

    int dsock = socket(PF_INET, SOCK_STREAM, 0);
    if (dsock == -1) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(dsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in ad;
    ad.sin_family      = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port        = htons(PORT);

    if (bind(dsock, (struct sockaddr *)&ad, sizeof(ad)) == -1) { perror("bind"); exit(1); }
    if (listen(dsock, MAX_CLIENTS) == -1) { perror("listen"); exit(1); }

    printf("Serveur d'echecs fonctionnel, attente de connexion sur le port %d...\n", PORT);

    while (1) {
        struct sockaddr_in adClient;
        socklen_t lgA = sizeof(adClient);

        int *sockClient = malloc(sizeof(int));
        *sockClient = accept(dsock, (struct sockaddr *)&adClient, &lgA);
        if (*sockClient == -1) { perror("accept"); free(sockClient); continue; }

        printf("Client connecté !\n");

        pthread_t tid;
        pthread_create(&tid, NULL, thread_client, sockClient);
        pthread_detach(tid);
    }

    close(dsock);
    free(piece);
    return 0;
}