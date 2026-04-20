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
#include "pieces.h"
#include "protocol.h"

#define PORT 8080

static void gestion_erreur(int res, const char *chaine)
{
    if (res == -1) { perror(chaine); exit(1); }
}

// enlevable ?
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

/* ─── Sérialise board + pièces éliminées dans un MSG_ETAT_PLATEAU ───────── */
static void build_etat_plateau(Message *m)
{
    memset(m, 0, sizeof(Message));
    m->type = MSG_ETAT_PLATEAU;

    /* board[0..7][0..7] aplati en data[0..63] */
    int idx = 0;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            m->data[idx++] = (int)(unsigned char)board[r][c];

    /* pièces éliminées blanches : nb puis chars */
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

    /* pièces éliminées noires */
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
 *  gere_partie  —  gère une partie complète avec un client connecté (dmec).
 *  Boucle :
 *    1. Envoie MSG_VOTRE_TOUR au client
 *    2. Attend MSG_DEMANDE_COUPS → calcule coups → renvoie MSG_REPONSE_COUPS
 *    3. Attend MSG_JOUER_COUP    → valide/joue  → renvoie MSG_ETAT_PLATEAU
 *       Si coup invalide → MSG_COUP_INVALIDE (tour non consommé, retour en 2)
 * ════════════════════════════════════════════════════════════════════════════ */

static void gere_partie(int dmec, int dmec_bis)
{
    joueur *j;
    joueur j_blanc, j_noir;
    memset(&j_blanc, 0, sizeof(joueur));
    memset(&j_noir,  0, sizeof(joueur));
    strncpy(j_blanc.Couleur, "blanc", 20);
    strncpy(j_noir.Couleur,  "noir",  20);

    int tour = 1;   /* 1 = blanc, 2 = noir */
    Message msg_in, msg_out,ta_couleur;

    int first_co, second_co;

    bool premier = rand()%2;
    if (premier){
        memset(&ta_couleur, 0, sizeof(ta_couleur));
        ta_couleur.type    = MSG_BLANC;
        ta_couleur.nb      = 1;   
        if (send_msg(dmec, &ta_couleur) < 0) { printf("adieu le client (il s'est deco)\n"); return; }
        
        memset(&ta_couleur, 0, sizeof(ta_couleur));
        ta_couleur.type    = MSG_NOIR;
        ta_couleur.nb      = 1;   
        if (send_msg(dmec_bis, &ta_couleur) < 0) { printf("adieu le client (il s'est deco)\n"); return; }

        first_co = 1 ;
        second_co = 0;
    }else{
        memset(&ta_couleur, 0, sizeof(ta_couleur));
        ta_couleur.type    = MSG_NOIR;
        ta_couleur.nb      = 1;   
        if (send_msg(dmec, &ta_couleur) < 0) { printf("adieu le client (il s'est deco)\n"); return; }

        memset(&ta_couleur, 0, sizeof(ta_couleur));
        ta_couleur.type    = MSG_BLANC;
        ta_couleur.nb      = 1;   
        if (send_msg(dmec_bis, &ta_couleur) < 0) { printf("adieu le client (il s'est deco)\n"); return; }
        first_co = 0 ;
        second_co = 1;
    }
    printf("Début de la partie\n");
    while (1) {
        int player_id;
        joueur *j;
        int sock_current, sock_other;

        if (tour % 2 == 1) { // tour des blancs
            player_id = 1;
            if (first_co == 1) {
                sock_current = dmec;
                sock_other   = dmec_bis;
            } else {
                sock_current = dmec_bis;
                sock_other   = dmec;
            }
            j = &j_blanc;
        } else { // tour des noirs
            player_id = 2;
            if (first_co == 1) {
                sock_current = dmec_bis;
                sock_other   = dmec;
            } else {
                sock_current = dmec;
                sock_other   = dmec_bis;
            }
            j = &j_noir;
        }

        /* ── 1. Informer le client de son tour ── */
        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.type    = MSG_VOTRE_TOUR;
        msg_out.data[0] = player_id;
        msg_out.nb      = 1;

        /* Envoyer aussi le plateau actuel */
        Message plateau;
        build_etat_plateau(&plateau);
        if (send_msg(sock_current, &plateau) < 0) { printf("Client deco\n"); return; }
        if (send_msg(sock_current, &msg_out) < 0) { printf("Client deco\n"); return; }

        /* Boucle interne : gestion des coups */
        while (1) {
            if (recv_msg(sock_current, &msg_in) < 0) {
                printf("Client a déco\n");
                return;
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

                msg_out.type = MSG_COUP_OK;
                msg_out.nb   = 0;
                send_msg(sock_current, &msg_out);

                // Signaler un échec éventuel
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
                break; // passe au tour suivant
            }
        }
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 *  main
 * ════════════════════════════════════════════════════════════════════════════ */
int main(void){
    setlocale(LC_ALL, "");
    int nbr_co=0;
    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;
    piece = (char *)malloc(sizeof(char));
    board[7][7] = 'R';

    int dsock = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(dsock, "socket");

    int opt = 1;
    setsockopt(dsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in ad;
    ad.sin_family      = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port        = htons(PORT);

    gestion_erreur(bind(dsock, (struct sockaddr *)&ad, sizeof(ad)), "bind");
    gestion_erreur(listen(dsock, 5), "listen");

    printf("Serveur d'echecs fonctionnel, attente de connexion sur le port %d...\n", PORT);

    struct sockaddr_in adClient;
    socklen_t lgA = sizeof(struct sockaddr_in);

    while (1) {
        
        int dmec = accept(dsock, (struct sockaddr *)&adClient, &lgA);
        nbr_co++;
        gestion_erreur(dmec, "accept");
        printf("Premier client connecté\n");
        int dmec_bis = accept(dsock, (struct sockaddr *)&adClient, &lgA);
        nbr_co++;
        gestion_erreur(dmec_bis, "accept");
        printf("Second client connecté\n");

        //Réinitialisation du plateau pour la partie:
        memset(board, ' ', sizeof(board));
        board[0][0]='r'; board[0][1]='h'; board[0][2]='c'; board[0][3]='q';
        board[0][4]='k'; board[0][5]='c'; board[0][6]='h'; board[0][7]='r';
        for (int c=0;c<8;c++) board[1][c]='p';
        for (int c=0;c<8;c++) board[6][c]='P';
        board[7][0]='R'; board[7][1]='H'; board[7][2]='C'; board[7][3]='Q';
        board[7][4]='K'; board[7][5]='C'; board[7][6]='H'; board[7][7]='R';

        
        white_king_moved=false; white_rook_left_moved=false; white_rook_right_moved=false;
        black_king_moved=false; black_rook_left_moved=false; black_rook_right_moved=false;
        en_passant_col=-1; en_passant_row=-1;
        en_passant_capture_row=-1; en_passant_capture_col=-1;

        
        eliminated_pieces_white.start = NULL;
        eliminated_pieces_black.start = NULL;
        // fin de la réinitialisation
        if (nbr_co==2){
            gere_partie(dmec,dmec_bis);
        }
        
        close(dmec);
        printf("Partie terminée. En attente d'une nouvelle connexion...\n");
    }

    close(dsock);
    free(piece);
    return 0;
}