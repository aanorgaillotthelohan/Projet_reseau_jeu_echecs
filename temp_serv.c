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
#include "salon.h"

#define MAX_SALONS 25
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
 pthread_mutex_t mutexJoueurs = PTHREAD_MUTEX_INITIALIZER;
 

joueur JoueursEnAttente[MAX_CLIENTS];
int    debut    = 0;
int    fin      = 0;
int    NbSalons = 0;   

Salon* Salons[MAX_SALONS];


//pour chat

struct chat_arg{
    int port;
    int nb;
    int ds;
};

#define NB_MAX 20

int connait_client(struct sockaddr_in * cli, struct sockaddr_in * liste_clients, int nb_cli){
    for (int i = 0; i < nb_cli; i++){
        if (liste_clients[i].sin_addr.s_addr == cli->sin_addr.s_addr &&
            liste_clients[i].sin_port == cli->sin_port)
            return i;
    }
    return -1;
}

static void gestion_erreur(int res, const char *chaine) {
    if (res == -1) { perror(chaine); exit(1); }
}


// Fonction pour le test de chat en thread
void * chat(void * arg){
    struct sockaddr_in liste_clients[NB_MAX];
    int nb_cli = 0;
    int dschat = ((struct chat_arg*)arg)->ds;

    socklen_t lgA = sizeof(struct sockaddr_in);
    char messagechat[128]; 
    struct sockaddr_in adClient;
    int chat_actif = 1;
    char stockmessage[10][128];
    memset(stockmessage, 0, sizeof(stockmessage));
    int position_message = 0;

    while(chat_actif){
        // Reçoit seulement du mec qui est concerné par dschat, il faudrait une liste pour avoir tout le monde...
        ssize_t reschat = recvfrom(dschat, messagechat, sizeof(messagechat), 0, (struct sockaddr*)&adClient, &lgA);
        gestion_erreur((int)reschat, "chat res");

        if (strcmp(messagechat, "f") == 0){
            printf("Un chatteur est parti \n");
            char tempo[128];
            strcpy(tempo, stockmessage[0]);
            memset(stockmessage[0], 0, sizeof(stockmessage[0]));
            stockmessage[0][0]='f';
            ssize_t res_udp = sendto(dschat, stockmessage, sizeof(stockmessage), 0, (struct sockaddr*)&adClient, lgA);
            int p = connait_client(&adClient, liste_clients, nb_cli);
            if (p != -1) {
                nb_cli--;
                for(int i = p; i < nb_cli; i++){
                    liste_clients[i] = liste_clients[i+1];
                }
            }
            strcpy(stockmessage[0], tempo);
            
        }
        else{
            // printf("message : %s\n",messagechat);
            // Met le message dans la liste
            strcpy(stockmessage[position_message], messagechat);
            if (position_message < 10){
                position_message += 1;
            }
            else{ 
                for(int i = 0; i < 9; i++){
                    strcpy(stockmessage[i], stockmessage[i+1]);
                }
                position_message = 10;
                strcpy(stockmessage[9], messagechat);
            }
            if(connait_client(&adClient, liste_clients, nb_cli) == -1){
                if (nb_cli < NB_MAX){
                    liste_clients[nb_cli] = adClient;
                    nb_cli++;
                    printf("Client ajouté dans chat à la place %d \n", nb_cli-1);
                }
                else{
                    printf("Client rejeté par manque de place.\n");
                }
            }

            for(int i = 0; i < nb_cli; i++){
                ssize_t res_udp = sendto(dschat, stockmessage, sizeof(stockmessage), 0, (struct sockaddr*)&liste_clients[i], lgA);
                gestion_erreur((int)res_udp, "sendto udp chat");
            }
        }
    }
}

//fin pour chat

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
    int  id_salon; 
} ArgPartie;

 void *gere_partie(void *arg)
{
    ArgPartie *a = (ArgPartie *)arg;
    int idsalon = a->id_salon;
    // MODIFICATION : On extrait l'id_salon de la structure 'a' pour l'utiliser localement
    //a->id_salon = 1;
    
    // MODIFICATION : Utilisation de l'id_salon récupéré pour accéder au tableau Salons
    Salon *s = Salons[a->id_salon - 1];

     //chat lancé:
    pthread_t chat_t;
    struct sockaddr_in ad; // lieu de stock de la socket
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr =  INADDR_ANY ;
    ad.sin_port = htons(PORT + idsalon);

    int dsockUdp = socket(PF_INET, SOCK_DGRAM, 0);
    int res = bind(dsockUdp, (struct sockaddr*)&ad, sizeof(ad));
    gestion_erreur(res, "bind Udp");

    struct chat_arg arg_chat;
    arg_chat.port = PORT;
    arg_chat.ds = dsockUdp;
    pthread_create(&chat_t, NULL, chat, &arg_chat); 

    int dmec     = a->sock1;
    int dmec_bis = a->sock2;
    free(a);
    char nom_joueur1[128], nom_joueur2[128];
    strncpy(nom_joueur1, a->nom1, 128);  
    strncpy(nom_joueur2, a->nom2, 128);   

    Message msg_in, msg_out, plateau;

    memset(&msg_out, 0, sizeof(msg_out));

    msg_out.type = MSG_PORT;
    msg_out.data[0] = idsalon;
    msg_out.nb = 1;
    send_msg(dmec,&msg_out);
    send_msg(dmec_bis,&msg_out );
    

    char b[8][8];
    // vérifier l'utilité des variables
    //eliminated_pieces_list ep_w = { NULL };
    //eliminated_pieces_list ep_b = { NULL };


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
                printf("[partie] Abandon détecté\n");

            /* Identifier gagnant/perdant par socket */
            int winner_sock = (sock_current == dmec) ? dmec_bis : dmec;
            const char *nom_perdant = (sock_current == dmec) ? nom_joueur1 : nom_joueur2;
            const char *nom_gagnant = (sock_current == dmec) ? nom_joueur2 : nom_joueur1;

            /* Notifier le gagnant */
            Message win_msg, loss_msg;
            memset(&win_msg,  0, sizeof(win_msg));
            memset(&loss_msg, 0, sizeof(loss_msg));
            win_msg.type    = MSG_FIN_PARTIE; win_msg.data[0]  = 1; win_msg.nb = 1;
            loss_msg.type   = MSG_FIN_PARTIE; loss_msg.data[0] = 2; loss_msg.nb = 1;
            send_msg(winner_sock,  &win_msg);
            /* Le perdant est déconnecté, pas la peine d'envoyer */

            /* Mettre à jour l'historique */
            UpdateStats(nom_gagnant, RESULT_WIN);
            UpdateStats(nom_perdant, RESULT_LOSS);

            printf("[histo] %s: victoire | %s: defaite (forfait)\n",
                   nom_gagnant, nom_perdant);

            close(dmec);
            close(dmec_bis);
            break;

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
                for(int i = 0; i < s->nbClients; i++){
                    send_msg(s->Clients[i].socketJoueur, &msg_out);
                }

                /* --- Après chaque coup joué --- */
                if (is_checkmate(1)) {
                    /* Blanc est mat → Noir gagne */
                    const char *nom_gagnant_n = (sock_noir == dmec) ? nom_joueur1 : nom_joueur2;
                    const char *nom_perdant_b = (sock_blanc == dmec) ? nom_joueur1 : nom_joueur2;

                    memset(&msg_out, 0, sizeof(msg_out));
                    msg_out.type = MSG_ECHEC_ET_MAT_BLANC;
                    for (int i = 0; i < s->nbClients; i++)
                        send_msg(s->Clients[i].socketJoueur, &msg_out);

                    /* MSG_FIN_PARTIE : 1=victoire, 2=défaite */
                    Message fin_g, fin_p;
                    memset(&fin_g, 0, sizeof(fin_g)); memset(&fin_p, 0, sizeof(fin_p));
                    fin_g.type = MSG_FIN_PARTIE; fin_g.data[0] = 1; fin_g.nb = 1;
                    fin_p.type = MSG_FIN_PARTIE; fin_p.data[0] = 2; fin_p.nb = 1;
                    send_msg(sock_noir,  &fin_g);
                    send_msg(sock_blanc, &fin_p);

                    UpdateStats(nom_gagnant_n, RESULT_WIN);
                    UpdateStats(nom_perdant_b, RESULT_LOSS);
                    printf("[histo] %s: victoire | %s: defaite (mat blanc)\n",
                           nom_gagnant_n, nom_perdant_b);

                    build_etat_plateau(&plateau, &eliminated_pieces_white,
                                       &eliminated_pieces_black, board);
                    for (int i = 0; i < s->nbClients; i++)
                        send_msg(s->Clients[i].socketJoueur, &plateau);
                    goto fin_partie;

                } else if (is_in_check(1)) {
                    memset(&msg_out, 0, sizeof(msg_out));
                    msg_out.type = MSG_ECHEC_BLANC;
                    for (int i = 0; i < s->nbClients; i++)
                        send_msg(s->Clients[i].socketJoueur, &msg_out);
                }

                if (is_checkmate(2)) {
                    /* Noir est mat → Blanc gagne */
                    const char *nom_gagnant_b = (sock_blanc == dmec) ? nom_joueur1 : nom_joueur2;
                    const char *nom_perdant_n = (sock_noir == dmec) ? nom_joueur1 : nom_joueur2;

                    memset(&msg_out, 0, sizeof(msg_out));
                    msg_out.type = MSG_ECHEC_ET_MAT_NOIR;
                    for (int i = 0; i < s->nbClients; i++)
                        send_msg(s->Clients[i].socketJoueur, &msg_out);

                    Message fin_g, fin_p;
                    memset(&fin_g, 0, sizeof(fin_g)); memset(&fin_p, 0, sizeof(fin_p));
                    fin_g.type = MSG_FIN_PARTIE; fin_g.data[0] = 1; fin_g.nb = 1;
                    fin_p.type = MSG_FIN_PARTIE; fin_p.data[0] = 2; fin_p.nb = 1;
                    send_msg(sock_blanc, &fin_g);
                    send_msg(sock_noir,  &fin_p);

                    UpdateStats(nom_gagnant_b, RESULT_WIN);
                    UpdateStats(nom_perdant_n, RESULT_LOSS);
                    printf("[histo] %s: victoire | %s: defaite (mat noir)\n",
                           nom_gagnant_b, nom_perdant_n);

                    build_etat_plateau(&plateau, &eliminated_pieces_white,
                                       &eliminated_pieces_black, board);
                    for (int i = 0; i < s->nbClients; i++)
                        send_msg(s->Clients[i].socketJoueur, &plateau);
                    goto fin_partie;
                }   else if (is_in_check(2)) {
                    memset(&msg_out, 0, sizeof(msg_out));
                    msg_out.type = MSG_ECHEC_NOIR;
                    for (int i = 0; i < s->nbClients; i++)
                        send_msg(s->Clients[i].socketJoueur, &msg_out);
                }

                build_etat_plateau(&plateau,
                                   &eliminated_pieces_white,
                                   &eliminated_pieces_black,
                                   board);
                for(int i = 0; i < s->nbClients; i++){
                    send_msg(s->Clients[i].socketJoueur, &msg_out);
                }
                // envoyer aux spectateurs
                for(int i = 0; i < s->nbClients; i++){
                    send_msg(s->Clients[i].socketJoueur, &plateau);
                }
                printf("[partie] Tour %d joué : %d -> %d\n", tour, src, dst);
                tour++;
                break;   
            }
        } 
    } 

fin_partie:
    printf("Partie terminée. En attente d'une nouvelle connexion...\n");
    for(int i = 0; i < s->nbClients; i++){
        close(s->Clients[i].socketJoueur);
    }
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

                NbSalons ++;

                /* Créer l'objet Salon [SV] */
                Salon *s = creeSalon(NbSalons, &j1, &j2);
                printf("[salon %d] Créé entre %s et %s\n",s->idSalon, j1.nom, j2.nom);
                Salons[NbSalons - 1] = s; 


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
                ap->id_salon = NbSalons;
                strncpy(ap->nom1, j1.nom, 128);
                strncpy(ap->nom2, j2.nom, 128);

                pthread_t tid;
                pthread_create(&tid, NULL, gere_partie, ap);
                pthread_detach(tid);
                return NULL;
            }

            pthread_mutex_unlock(&mutex_file);

            return NULL;   
            
            /*
            printf("[file] %s en attente d'un adversaire…\n", msg.idJoueur);
            pthread_mutex_unlock(&mutexJoueurs);
            */
            /* Le thread reste vivant pour surveiller les déconnexions */
        }else if(msg.choix == 's'){
            pthread_mutex_lock(&mutexJoueurs);

            // 1. Envoi de la liste des salons (inchangé)
            send(sockClient, &NbSalons, sizeof(NbSalons), 0);
            for(int i = 0; i < NbSalons; i++){
                SalonNet sn;
                sn.idSalon = Salons[i]->idSalon;
                sn.nbSpectateurs = Salons[i]->nbSpectateurs;
                strncpy(sn.joueur1, Salons[i]->Joueur1.NomJoueur, 31);
                strncpy(sn.joueur2, Salons[i]->Joueur2.NomJoueur, 31);
                send(sockClient, &sn, sizeof(sn), 0);
            }

            // 2. Réception du choix
            int idChoisi;
            if(recv(sockClient, &idChoisi, sizeof(idChoisi), 0) <= 0){
                printf("Erreur réception choix salon\n");
                pthread_mutex_unlock(&mutexJoueurs);
                return NULL;
            }

            int rep = 0;
            Salon* s = NULL;

            // 3. Recherche du salon
            for(int i = 0; i < NbSalons; i++){
                if(Salons[i] != NULL && Salons[i]->idSalon == idChoisi){
                    s = Salons[i];
                    break;
                }
            }

            // 4. Vérification et ajout
            if(s != NULL && s->nbClients < 22){
                joueur* spec = creejoueur(sockClient, msg.idJoueur);
                s->Clients[s->nbClients] = *spec;
                s->nbClients++;
                s->nbSpectateurs++;
                printf("Spectateur %s ajouté au salon %d\n", spec->NomJoueur, s->idSalon);
                rep = 1; // Succès
            } else {
                rep = 0; // Échec
            }

            // --- ETAPE CRUCIALE : ON ENVOIE D'ABORD LA REPONSE ---
            send(sockClient, &rep, sizeof(rep), 0);

            // --- ETAPE SUIVANTE : SI SUCCÈS, ON ENVOIE LE PLATEAU ---
            if (rep == 1) {
                printf("Envoi du plateau initial au spectateur...\n");
                Message initial_plateau;
                // On construit le message avec le board actuel
                build_etat_plateau(&initial_plateau, &eliminated_pieces_white, &eliminated_pieces_black, board);
                
                if (send_msg(sockClient, &initial_plateau) == -1) {
                    perror("Erreur envoi plateau initial");
                }
            }
        /* ── Historique (non implémenté) ── [SV] */
            
        }else if (msg.choix == 'h') {
            /* Envoyer les stats encodées dans un Message */
            int p = 0, v = 0, d = 0, dr = 0;
            GetStats(idConnecte, &p, &v, &d, &dr);
            Message m_histo;
            memset(&m_histo, 0, sizeof(m_histo));
            m_histo.type    = MSG_HISTO_REPONSE;
            m_histo.data[0] = p;
            m_histo.data[1] = v;
            m_histo.data[2] = d;
            m_histo.data[3] = dr;
            m_histo.nb      = 4;
            send_msg(sockClient, &m_histo);
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