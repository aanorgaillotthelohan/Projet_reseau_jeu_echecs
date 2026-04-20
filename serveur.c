/*
 * server_final.c — Fusion de serveur_test_salon.c + serverVrai.c + temp_serv.c
 *
 * Fonctionnement :
 *   - Un thread par client connecté (gestion_client)
 *   - Authentification (connexion 'c' / inscription 'i')
 *   - File d'attente : dès que 2 joueurs demandent à jouer ('j'),
 *     un thread de partie (thread_partie) est lancé pour eux
 *   - La partie échecs tourne dans thread_partie via gere_partie()
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <locale.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "chess.h"
#include "application.h"
#include "salon.h"
#include "protocol.h"
#include "players.h"
#include "pieces.h"
#include "display.h"

/* ── Constantes ─────────────────────────────────────────────────────────── */
#define PORT        1061
#define MAX_CLIENTS 50

/* ════════════════════════════════════════════════════════════════════════════
 *  TYPES
 * ════════════════════════════════════════════════════════════════════════════ */

typedef struct {
    char choix;
    char idJoueur[128];
    char Mdp[128];
} MessageClient;

typedef struct {
    int  sock1;
    int  sock2;
    int  id_salon;
    char nom1[20];
    char nom2[20];
} ContextePartie;

/* ════════════════════════════════════════════════════════════════════════════
 *  VARIABLES GLOBALES — File d'attente
 * ════════════════════════════════════════════════════════════════════════════ */

joueur JoueursEnAttente[MAX_CLIENTS];
int    debut    = 0;
int    fin      = 0;
int    NbSalons = 0;

pthread_mutex_t mutexJoueurs = PTHREAD_MUTEX_INITIALIZER;

/* ════════════════════════════════════════════════════════════════════════════
 *  UTILITAIRES RÉSEAU
 * ════════════════════════════════════════════════════════════════════════════ */

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

/* ════════════════════════════════════════════════════════════════════════════
 *  FILE D'ATTENTE
 * ════════════════════════════════════════════════════════════════════════════ */

static void retirer_joueur_file(int sockClient)
{
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
        printf("[file] Joueur retiré (socket %d)\n", sockClient);
    }
    pthread_mutex_unlock(&mutexJoueurs);
}

/* ════════════════════════════════════════════════════════════════════════════
 *  PLATEAU — État partagé par partie (protégé dans thread_partie)
 * ════════════════════════════════════════════════════════════════════════════ */

/* Chaque partie a son propre état local.
 * On utilise les globaux de chess.h / globals.c car le projet y repose.
 * Les parties sont sérialisées par mutex_plateau si nécessaire.
 * Pour l'instant, une seule partie à la fois est supportée proprement
 * (le mutex global board est partagé) ; le code est structuré pour
 * être facilement étendu à plusieurs parties simultanées. */

static pthread_mutex_t mutex_plateau = PTHREAD_MUTEX_INITIALIZER;

static void init_plateau(void)
{
    memset(board, ' ', sizeof(board));
    board[0][0]='r'; board[0][1]='h'; board[0][2]='c'; board[0][3]='q';
    board[0][4]='k'; board[0][5]='c'; board[0][6]='h'; board[0][7]='r';
    for (int c = 0; c < 8; c++) board[1][c] = 'p';
    for (int c = 0; c < 8; c++) board[6][c] = 'P';
    board[7][0]='R'; board[7][1]='H'; board[7][2]='C'; board[7][3]='Q';
    board[7][4]='K'; board[7][5]='C'; board[7][6]='H'; board[7][7]='R';

    white_king_moved       = false; white_rook_left_moved  = false;
    white_rook_right_moved = false; black_king_moved       = false;
    black_rook_left_moved  = false; black_rook_right_moved = false;

    en_passant_col         = -1; en_passant_row         = -1;
    en_passant_capture_row = -1; en_passant_capture_col = -1;

    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;
}

static void build_etat_plateau(Message *m)
{
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

/* ════════════════════════════════════════════════════════════════════════════
 *  LOGIQUE DE PARTIE
 * ════════════════════════════════════════════════════════════════════════════ */

static void gere_partie(int dmec, int dmec_bis,
                        const char *nom1, const char *nom2)
{
    joueur j_blanc, j_noir;
    memset(&j_blanc, 0, sizeof(joueur));
    memset(&j_noir,  0, sizeof(joueur));
    strncpy(j_blanc.Couleur, "blanc", 20);
    strncpy(j_noir.Couleur,  "noir",  20);
    strncpy(j_blanc.NomJoueur, nom1, 20);
    strncpy(j_noir.NomJoueur,  nom2, 20);

    int tour = 1;
    Message msg_in, msg_out, ta_couleur, plateau;

    /* ── Attribution des couleurs ── */
    int first_co;
    if (rand() % 2) {
        memset(&ta_couleur, 0, sizeof(ta_couleur));
        ta_couleur.type = MSG_BLANC; ta_couleur.nb = 1;
        if (send_msg(dmec,     &ta_couleur) < 0) goto fin_partie;
        ta_couleur.type = MSG_NOIR;
        if (send_msg(dmec_bis, &ta_couleur) < 0) goto fin_partie;
        first_co = 1;
    } else {
        memset(&ta_couleur, 0, sizeof(ta_couleur));
        ta_couleur.type = MSG_NOIR; ta_couleur.nb = 1;
        if (send_msg(dmec,     &ta_couleur) < 0) goto fin_partie;
        ta_couleur.type = MSG_BLANC;
        if (send_msg(dmec_bis, &ta_couleur) < 0) goto fin_partie;
        first_co = 0;
    }

    printf("[partie] Début entre %s (blanc=%s) et %s\n",
           nom1, first_co ? nom1 : nom2, nom2);

    /* ── Boucle des tours ── */
    while (1) {
        int player_id;
        joueur *j;
        int sock_current;

        if (tour % 2 == 1) {
            player_id    = 1;
            sock_current = (first_co == 1) ? dmec : dmec_bis;
            j            = &j_blanc;
        } else {
            player_id    = 2;
            sock_current = (first_co == 1) ? dmec_bis : dmec;
            j            = &j_noir;
        }

        /* 1. Envoyer plateau + MSG_VOTRE_TOUR */
        build_etat_plateau(&plateau);
        if (send_msg(sock_current, &plateau) < 0) goto fin_partie;

        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.type    = MSG_VOTRE_TOUR;
        msg_out.data[0] = player_id;
        msg_out.nb      = 1;
        if (send_msg(sock_current, &msg_out) < 0) goto fin_partie;

        /* 2 & 3. Boucle interne : traitement des messages du client */
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

                j->coupDuTour = src * 100 + dst;
                int res = (player_id == 1) ? player_white(j, src, dst)
                                           : player_black(j, src, dst);
                if (res < 0) {
                    msg_out.type = MSG_COUP_INVALIDE;
                    send_msg(sock_current, &msg_out);
                    continue;
                }

                /* Coup accepté */
                msg_out.type = MSG_COUP_OK;
                msg_out.nb   = 0;
                send_msg(sock_current, &msg_out);

                if (is_in_check(1)) {
                    msg_out.type = MSG_ECHEC_BLANC;
                    send_msg(sock_current, &msg_out);
                }
                if (is_in_check(2)) {
                    msg_out.type = MSG_ECHEC_NOIR;
                    send_msg(sock_current, &msg_out);
                }

                build_etat_plateau(&plateau);
                send_msg(sock_current, &plateau);

                printf("Tour %d joué : %d -> %d\n", tour, src, dst);
                tour++;
                break;
            }
        }
    }

fin_partie:
    printf("[partie] Fin de partie entre %s et %s\n", nom1, nom2);
    close(dmec);
    close(dmec_bis);
}

/* ════════════════════════════════════════════════════════════════════════════
 *  THREAD DE PARTIE
 * ════════════════════════════════════════════════════════════════════════════ */

static void *thread_partie(void *arg)
{
    ContextePartie *ctx = (ContextePartie *)arg;

    pthread_mutex_lock(&mutex_plateau);
    init_plateau();
    pthread_mutex_unlock(&mutex_plateau);

    gere_partie(ctx->sock1, ctx->sock2, ctx->nom1, ctx->nom2);

    free(ctx);
    return NULL;
}

/* ════════════════════════════════════════════════════════════════════════════
 *  GESTION D'UN CLIENT (un thread par connexion)
 * ════════════════════════════════════════════════════════════════════════════ */

static void *gestion_client(void *arg)
{
    int sockClient = *(int *)arg;
    free(arg);

    MessageClient msg;

    while (1) {
        ssize_t r = recv(sockClient, &msg, sizeof(msg), 0);
        if (r <= 0) {
            printf("Client déconnecté\n");
            retirer_joueur_file(sockClient);
            close(sockClient);
            return NULL;
        }

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
            rep = VerifConnexion(msg.idJoueur, msg.Mdp) ? 1 : 0;
            send(sockClient, &rep, sizeof(rep), 0);
        }
        else if (msg.choix == 'j') {
            joueur *j = creejoueur(sockClient, msg.idJoueur);

            pthread_mutex_lock(&mutexJoueurs);

            if (fin < MAX_CLIENTS) {
                JoueursEnAttente[fin++] = *j;
            } else {
                printf("File d'attente pleine !\n");
                pthread_mutex_unlock(&mutexJoueurs);
                free(j);
                continue;
            }
            free(j);

            if (fin - debut >= 2) {
                joueur j1 = JoueursEnAttente[debut];
                joueur j2 = JoueursEnAttente[debut + 1];
                debut += 2;

                NbSalons++;
                Salon *s = creeSalon(NbSalons, &j1, &j2);
                printf("Salon %d créé entre %s et %s\n",
                       s->idSalon, j1.NomJoueur, j2.NomJoueur);
                free(s);

                int matchTrouve = 1;
                send(j1.socketJoueur, &matchTrouve, sizeof(matchTrouve), 0);
                send(j2.socketJoueur, &matchTrouve, sizeof(matchTrouve), 0);

                if (debut > 0 && fin >= MAX_CLIENTS / 2) {
                    memmove(JoueursEnAttente,
                            &JoueursEnAttente[debut],
                            (size_t)(fin - debut) * sizeof(joueur));
                    fin   -= debut;
                    debut  = 0;
                }

                ContextePartie *ctx = malloc(sizeof(ContextePartie));
                ctx->sock1    = j1.socketJoueur;
                ctx->sock2    = j2.socketJoueur;
                ctx->id_salon = NbSalons;
                strncpy(ctx->nom1, j1.NomJoueur, 20);
                strncpy(ctx->nom2, j2.NomJoueur, 20);

                pthread_t tid_partie;
                pthread_create(&tid_partie, NULL, thread_partie, ctx);
                pthread_detach(tid_partie);

                pthread_mutex_unlock(&mutexJoueurs);
                return NULL;
            }

            printf("[file] %s en attente d'un adversaire...\n", msg.idJoueur);
            pthread_mutex_unlock(&mutexJoueurs);
        }
        else if (msg.choix == 's') {
            printf("Mode spectateur non implémenté.\n");
        }
        else if (msg.choix == 'h') {
            printf("Historique non implémenté.\n");
        }
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  MAIN
 * ════════════════════════════════════════════════════════════════════════════ */

int main(void)
{
    setlocale(LC_ALL, "");

    piece = (char *)malloc(sizeof(char));
    init_plateau();

    memset(JoueursEnAttente, 0, sizeof(JoueursEnAttente));

    int dsock = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(dsock, "socket");

    int opt = 1;
    setsockopt(dsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in ad;
    ad.sin_family      = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port        = htons(PORT);

    gestion_erreur(bind  (dsock, (struct sockaddr *)&ad, sizeof(ad)), "bind");
    gestion_erreur(listen(dsock, MAX_CLIENTS), "listen");

    printf("Serveur d'echecs en écoute sur le port %d...\n", PORT);

    while (1) {
        struct sockaddr_in adClient;
        socklen_t lgA = sizeof(adClient);

        int *sockClient = malloc(sizeof(int));
        *sockClient = accept(dsock, (struct sockaddr *)&adClient, &lgA);
        if (*sockClient == -1) {
            perror("accept");
            free(sockClient);
            continue;
        }

        printf("Client connecté !\n");

        pthread_t tid;
        pthread_create(&tid, NULL, gestion_client, sockClient);
        pthread_detach(tid);
    }

    close(dsock);
    free(piece);
    return 0;
}