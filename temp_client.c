#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <locale.h>
#include <wchar.h>

#include "chess.h"
#include "display.h"
#include "pieces.h"
#include "protocol.h"
#include "salon.h"


typedef struct {
    char choix;
    char idJoueur[128];
    char Mdp[128];
} MessageClient;

int dsock_mec;
volatile int enRecherche = 0;

void gestion_erreur(int res, const char *chaine)
{
    if (res == -1) { perror(chaine); exit(1); }
}

 int send_msg(int sock, const Message *m)
{
    ssize_t total = 0, size = (ssize_t)sizeof(Message);
    const char *ptr = (const char *)m;
    while (total < size) {
        ssize_t n = send(dsock_mec, ptr + total, (size_t)(size - total), 0);
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}

 int recv_msg(int sock, Message *m)
{
    ssize_t total = 0, size = (ssize_t)sizeof(Message);
    char *ptr = (char *)m;
    while (total < size) {
        ssize_t n = recv(dsock_mec, ptr + total, (size_t)(size - total), 0);
        if (n <= 0) return -1;
        total += n;
    }
    return 0;
}


 char lire_choix(const char *prompt)
{
    char buf[256];
    printf("%s", prompt); fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return '\0';
    return (char)tolower((unsigned char)buf[0]);
}


 void lire_chaine(const char *prompt, char *dest, int maxlen)
{
    printf("%s", prompt); fflush(stdout);
    if (!fgets(dest, maxlen, stdin)) { dest[0] = '\0'; return; }
    dest[strcspn(dest, "\n")] = '\0';
}

// Thread pour écouter le serveur
void* listener(void* arg){
    while(1){
        if(enRecherche){
            int matchTrouve = 0;
            ssize_t r = recv(dsock_mec, &matchTrouve, sizeof(matchTrouve), 0);
            if(r <= 0){
                printf("\nDéconnecté du serveur.\n");
                exit(1);
            }
            if(matchTrouve == 1){
                printf("\n=== Match trouvé ! ===\n");
                enRecherche = 0;
            }
        } else {
            usleep(100000);
        }
    }
    return NULL;
}

 void affiche_plateau_recu(const Message *m)
{
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            board[r][c] = (char)m->data[r * 8 + c];

    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;

    int nb_w = m->data[64], nb_b = m->data[65], pos = 66;
    for (int i = 0; i < nb_w && pos < MSG_DATA_MAX; i++, pos++)
        insert_eliminated_list((char)m->data[pos], &eliminated_pieces_white);
    for (int i = 0; i < nb_b && pos < MSG_DATA_MAX; i++, pos++)
        insert_eliminated_list((char)m->data[pos], &eliminated_pieces_black);

    display();
}

 void affiche_coups_recus(const Message *m, int src)
{
    printf("\u25B6 Coup(s) possible(s) avec cette pièce %d :\n  ", src);
    for (int i = 0; i < m->nb; i++) {
        int label = m->data[i] / 10, col = m->data[i] % 10, r = 7 - label;
        printf("[%d%d] ", label, col);
        if (board[r][col] == ' ') board[r][col] = 'x';
    }
    printf("\n");
    display();
    printf("\u25B6 Coup(s) possible(s) avec cette pièce %d :\n  ", src);
    for (int i = 0; i < m->nb; i++)
        printf("[%d%d] ", m->data[i] / 10, m->data[i] % 10);
    printf("\n");
}

 void efface_marqueurs(void)
{
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c] == 'x') board[r][c] = ' ';
}


 int tour_client(int sock, int player_id)
{
    const char *couleur = (player_id == 1) ? "Blanc" : "Noir";
    Message msg_out, msg_in;
    char buf[64];

    while (1) {
        efface_marqueurs();
        display();

        printf("\n\u25CF %s — Selectionnez la pièce à jouer, suivre le format ligne puis colonne (ex: 11): ", couleur);
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) return -1;
        int src = atoi(buf);

        if (src < 0 || src > 77 || src % 10 > 7) {
            printf("\u26A0 Coordonnees invalides. Format : LigneColonne (ex: 11)\n");
            continue;
        }

        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.type = MSG_DEMANDE_COUPS; msg_out.data[0] = src; msg_out.nb = 1;
        if (send_msg(sock, &msg_out) < 0) return -1;
        if (recv_msg(sock, &msg_in)  < 0) return -1;

        if (msg_in.type == MSG_COUP_INVALIDE || msg_in.nb == 0) {
            printf("\u26A0 Pas de piece valide en %d ou aucun coup legal.\n", src);
            continue;
        }

        affiche_coups_recus(&msg_in, src);

        printf("\n\u25CF %s — Entrez la destination (ou 0 pour changer de piece) : ", couleur);
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) return -1;
        int dst = atoi(buf);

        if (dst == 0) continue;

        if (dst < 0 || dst > 77 || dst % 10 > 7) {
            printf("\u26A0 Coordonnees invalides.\n");
            continue;
        }

        memset(&msg_out, 0, sizeof(msg_out));
        msg_out.type = MSG_JOUER_COUP; msg_out.data[0] = src; msg_out.data[1] = dst; msg_out.nb = 2;
        if (send_msg(sock, &msg_out) < 0) return -1;

        while (1) {
            if (recv_msg(sock, &msg_in) < 0) return -1;
            if (msg_in.type == MSG_COUP_INVALIDE) {
                printf("\u26A0 Coup invalide, reessayez.\n"); break;
            }
            if (msg_in.type == MSG_COUP_OK)      { continue; }
            if (msg_in.type == MSG_ECHEC_BLANC)  { printf("\n\u26A0\u26A0  ECHEC AU ROI BLANC !  \u26A0\u26A0\n"); continue; }
            if (msg_in.type == MSG_ECHEC_NOIR)   { printf("\n\u26A0\u26A0  ECHEC AU ROI NOIR !  \u26A0\u26A0\n");  continue; }
            if (msg_in.type == MSG_ETAT_PLATEAU) {
                efface_marqueurs();
                affiche_plateau_recu(&msg_in);
                printf("\u2705 Coup joue : %d -> %d\n", src, dst);
                return 0;
            }
        }
    }
}

int main(int argc, char **argv){
    setlocale(LC_ALL, "");

    if (argc < 2) { printf("Usage : %s <port>\n", argv[0]); return 1; }

    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;
    piece = (char *)malloc(sizeof(char));
    board[7][7] = 'R';

    int port = atoi(argv[1]);
    printf("Connexion au serveur sur le port %d...\n", port);

    dsock_mec = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(dsock_mec, "socket");

    struct sockaddr_in adServ;
    adServ.sin_family = AF_INET;
    adServ.sin_port   = htons(port);
    gestion_erreur(inet_pton(AF_INET, "127.0.0.1", &adServ.sin_addr), "inet_pton");
    gestion_erreur(connect(dsock_mec, (struct sockaddr *)&adServ, sizeof(adServ)), "connect");

    printf("Connecte au serveur.\n");
    printf("Vous pouvez quitter avec '!'\n");

    pthread_t tid;
    pthread_create(&tid, NULL, listener, NULL);
    pthread_detach(tid);

    char idJoueur[128] = {0}, Mdp[128] = {0}, choix;
    int  connecte = 0, jeuEnCours = 1;
    MessageClient msg_auth;

    while (!connecte && jeuEnCours) {
        choix = lire_choix("Connexion / Inscription (c/i) : ");
        if (choix == '!') { jeuEnCours = 0; break; }

        if (choix == 'i' || choix == 'c') {
            lire_chaine("Identifiant : ",  idJoueur, 128);
            lire_chaine("Mot de passe : ", Mdp,      128);

            memset(&msg_auth, 0, sizeof(msg_auth));
            msg_auth.choix = choix;
            strncpy(msg_auth.idJoueur, idJoueur, 128);
            strncpy(msg_auth.Mdp,      Mdp,      128);
            send(dsock_mec, &msg_auth, sizeof(msg_auth), 0);

            int rep = 0;
            recv(dsock_mec, &rep, sizeof(rep), 0);

            if (rep == 1) {
                if (choix == 'i') printf("Inscription réussie ! Connecté.\n");
                else              printf("Connecté !\n");
                connecte = 1;
            } else {
                if (choix == 'i') printf("Identifiant déjà pris.\n");
                else              printf("Identifiant ou mot de passe incorrect.\n");
            }
        } else {
            printf("Choix invalide.\n");
        }
    }

    if (!jeuEnCours) { close(dsock_mec); free(piece); return 0; }


    while (jeuEnCours) {
        // Menu principal
        char choixValid = 0;

        while(!choixValid){
            printf("Choix (j=jouer / !=quitter / s=spectateur / h=historitque) : ");
            scanf(" %c", &choix);
            choix = tolower(choix);

            if(choix == '!'){
                jeuEnCours = 0;
                send(dsock_mec, &choix, sizeof(choix), 0);
                break;
            }
            else if(choix == 'j'){
                choixValid = 1;
            }
            else if (choix == 's'){ // ajout du mode spectateur si problèmes problèmes ici

                int nbSalons = 0;
                int choixSalon;
                // ajout essai
                MessageClient msgReq = {choix, "", ""};
                strncpy(msgReq.idJoueur, idJoueur, 128);
                send(dsock_mec, &msgReq, sizeof(msgReq), 0);

                if(recv(dsock_mec, &nbSalons, sizeof(nbSalons), 0) <= 0){
                    printf("Erreur réception.\n");
                    break;
                }

                printf("\n=== Salons disponibles (%d) ===\n", nbSalons);
                for(int i = 0; i < nbSalons; i++){
                    SalonNet s;
                    if(recv(dsock_mec, &s, sizeof(SalonNet), 0) <= 0){
                        printf("Erreur réception salon.\n");
                        break;
                    }
                printf("Salon %d : %s vs %s (%d spectateurs)\n",s.idSalon, s.joueur1, s.joueur2, s.nbSpectateurs);          
                }

                printf("==============================\n");
                printf("Entrer l'ID du salon à rejoindre (0 pour annuler) : ");
                scanf("%d", &choixSalon);

                // envoyer le choix au serveur
                send(dsock_mec, &choixSalon, sizeof(choixSalon), 0);

                if(choixSalon == 0){
                    printf("Retour menu.\n");
                    break;
                }

                // recevoir la réponse du serveur
                int rep = 0;
                if(recv(dsock_mec, &rep, sizeof(rep), 0) <= 0){
                    printf("Erreur réception.\n");
                    break;
                }

                if(rep == 1){
                    printf("Vous avez rejoint le salon %d en tant que spectateur.\n", choixSalon);

                    Message msg;

                    while(1){
                        /* duplication ?
                        if(recv_msg(dsock_mec, &msg) <= 0){
                            printf("Erreur réception plateau.\n");
                            return 0;
                        }
                        */
                        if(msg.type == MSG_ETAT_PLATEAU){
                            affiche_plateau_recu(&msg);
                        }
                        else if(msg.type == MSG_COUP_OK){
                            printf("Un coup a été joué.\n");
                        }
                        else if(msg.type == MSG_ECHEC_BLANC){
                            printf("Blanc en échec !\n");
                        }
                        else if(msg.type == MSG_ECHEC_NOIR){
                            printf("Noir en échec !\n");
                        }
                        else if(recv_msg(dsock_mec, &msg) <= 0){
                            printf("Déconnexion.\n");
                            break;
                        }
                    }

                } else {
                    printf("Salon invalide ou plein.\n");
                }

            }

        if(!jeuEnCours) break;

        MessageClient msg = {choix, "", ""};
        strcpy(msg.idJoueur, idJoueur);
        send(dsock_mec, &msg, sizeof(msg), 0);

        if(choix == 'j'){
            printf("Recherche de partie... (appuyez sur 'r' + Entrée pour annuler)\n");
            enRecherche = 1;

            char input[16];
            while(enRecherche){
                if(fgets(input, sizeof(input), stdin)){
                    if(tolower(input[0]) == 'r'){
                        printf("Recherche annulée.\n");
                        enRecherche = 0;
                        break;
                    }
                }
            }     
        }

        int my_player_id = 0;
        Message msg_jeu;

        while (1) {
            if (recv_msg(dsock_mec, &msg_jeu) < 0) {
                printf("Connexion perdue.\n");
                jeuEnCours = 0;
                break;
            }

            if (msg_jeu.type == MSG_BLANC) {
                my_player_id = 1;
                printf("Couleur de cette partie : Blanc\n");
                continue;
            }
            
            if (msg_jeu.type == MSG_NOIR) {
                my_player_id = 2;
                printf("Couleur de cette partie : Noir\n");
                continue;
            }

            if (msg_jeu.type == MSG_ETAT_PLATEAU) {
                affiche_plateau_recu(&msg_jeu);
                continue;
            }

            if (msg_jeu.type == MSG_ECHEC_BLANC) {
                printf("\n\u26A0\u26A0  ECHEC AU ROI BLANC !  \u26A0\u26A0\n");
                continue;
            }

            if (msg_jeu.type == MSG_ECHEC_NOIR) {
                printf("\n\u26A0\u26A0  ECHEC AU ROI NOIR !  \u26A0\u26A0\n");
                continue;
            }
            
            if (msg_jeu.type == MSG_VOTRE_TOUR) {
                int player_id = msg_jeu.data[0];
                const char *couleur_tour = (player_id == 1) ? "BLANC" : "NOIR";
                printf("\n══ C'est au tour des %s ══\n", couleur_tour);

                if (player_id == my_player_id) {
                    if (tour_client(dsock_mec, player_id) < 0) {
                        printf("Connexion perdue.\n");
                        jeuEnCours = 0;
                        break;
                    }
                }
                continue;
            }

        }
        break;
    }
}

    close(dsock_mec);
    free(piece);
    return 0;
}