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

#define PORT 1061

typedef struct {
    char choix;
    char idJoueur[128];
    char Mdp[128];
} MessageClient;

int dsock_mec;
volatile int enRecherche = 0;
int inchat = 0;
int idsalon = 0;


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

void gere_chat(int *inchat, struct sockaddr_in adServ, int idsalon, char * idJoueur, int estSpectateur){

    int dsock_udp = socket(PF_INET, SOCK_DGRAM, 0);
    gestion_erreur(dsock_udp, "sock udp");

    printf("chat\n");
    *inchat = 1;
    char message_udp[128];
    char stockmessage[10][128];
    char message_udp_complete[128];
    memset(stockmessage, 0, sizeof(stockmessage)); //pour initialiser à 0 apparemment
    memset(message_udp, 0, sizeof(message_udp));
    memset(message_udp, 0, sizeof(message_udp_complete));
    int pid_pere = getpid();
    
    int f = fork();
    int timer = 0;
    
    struct sockaddr_in adUdp; // lieu de stock de la socket
    adUdp.sin_family = AF_INET;
    adUdp.sin_addr.s_addr =  adServ.sin_addr.s_addr ;
    adUdp.sin_port = htons(PORT + idsalon);

    size_t lgA = sizeof(adUdp);
    
    if (f == 0){
        //message vide qui initialise la communication
        int res_udp = sendto(dsock_udp, &message_udp, sizeof(message_udp), 0, (struct sockaddr *)&adUdp, lgA);
        // Processus fils (client qui écrit)
        time_t rawtime;
        struct tm * timeinfo;
        while(*inchat){

            fgets(message_udp, sizeof(message_udp), stdin); 
            
            if(strcmp(message_udp,"Quit\n")==0){
                memset(message_udp, 0, sizeof(message_udp));
                message_udp[0] = 'f';
                // Cast explicite
                res_udp = sendto(dsock_udp, &message_udp, sizeof(message_udp), 0, (struct sockaddr *)&adUdp, lgA);
                gestion_erreur(res_udp, "sendto f"); 
                close(dsock_udp);
                exit(0);
            }
            if(strcmp(message_udp,"\n")!=0){
                time ( &rawtime );
                timeinfo = localtime ( &rawtime );
                char heure[32];
                strftime(heure, sizeof(heure), " [%Hh:%Mm:%Ss]", timeinfo);

                memset(message_udp_complete, 0, sizeof(message_udp_complete));

                strcat(message_udp_complete, idJoueur);
                if(estSpectateur){strcat(message_udp_complete, " (spectateur) ");}
                strcat(message_udp_complete, heure);
                strcat(message_udp_complete, ": ");

                strcat(message_udp_complete, message_udp);

                res_udp = sendto(dsock_udp, &message_udp_complete, sizeof(message_udp_complete), 0, (struct sockaddr *)&adUdp, lgA);
                gestion_erreur(res_udp, "sendto chat");    
            }

        }
    }
    else{
        // Processus père (client qui lit)
        while(*inchat){
            timer +=1;
            
            efface_marqueurs();
            if(!estSpectateur){display();}
            // Cast explicite pour recvfrom
            ssize_t reschat = recvfrom(dsock_udp, stockmessage, sizeof(stockmessage), 0, (struct sockaddr *)&adUdp, &lgA);
            gestion_erreur(reschat, "recvfrom chat");
            
            if(strcmp(stockmessage[0],"f") == 0){
                *inchat = 0;
                printf("Quitte chat\n");
                break;
            }
            else{
                if(!estSpectateur){
                    printf("\033c" );
                }
                for (int i = 0; i < 10; i++){
                    if(strlen(stockmessage[i]) > 1){
                        printf("%s \n", stockmessage[i]);
                        fflush(stdout);
                    }
                }
                if(estSpectateur){
                    printf("-----------------\n");
                }else{
                    printf("-----------------'Quit' pour revenir au jeu\n");
                }
                
            }


        }
    } 
    
    close(dsock_udp);
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


 int tour_client(int sock, int player_id, struct sockaddr_in adServ, char * idJoueur)
{
    const char *couleur = (player_id == 1) ? "Blanc" : "Noir";
    Message msg_out, msg_in;
    char buf[64];
    socklen_t lgA = sizeof(struct sockaddr_in) ;

    while (1) {
        efface_marqueurs();
        display();

        printf("\n\u25CF %s — Selectionnez la pièce à jouer, suivre le format ligne puis colonne (ex: 11) ou 666 pour chat: ", couleur);
        fflush(stdout);
        if (!fgets(buf, sizeof(buf), stdin)) return -1;
        int src = atoi(buf);

        if (src == 666){
            gere_chat(&inchat, adServ,idsalon,idJoueur,0);
        }
        else{
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

            // gestion des reception de coup coté client
            while (1) {
                if (recv_msg(sock, &msg_in) < 0) return -1;
                if (msg_in.type == MSG_COUP_INVALIDE) {
                    printf("\u26A0 Coup invalide, reessayez.\n"); break;
                }
                if (msg_in.type == MSG_COUP_OK)      { continue; }
                if (msg_in.type == MSG_ECHEC_BLANC)  { printf("\nECHEC AU ROI BLANC ! \n"); continue; }
                if (msg_in.type == MSG_ECHEC_NOIR)   { printf("\nECHEC AU ROI NOIR !  \n");  continue; }
                if (msg_in.type == MSG_ECHEC_ET_MAT_BLANC)  { printf("\nECHEC ET MAT AU ROI BLANC ! \n"); break; }
                if (msg_in.type == MSG_ECHEC_ET_MAT_NOIR)   { printf("\n\u26A0\u26A0  ECHEC ET MAT AU ROI NOIR !  \u26A0\u26A0\n");  continue; }
                if (msg_in.type == MSG_FIN_PARTIE) {
                    // Remonter le message pour que main() le traite
                    if (msg_in.data[0] == 1)      printf("\n+++ VICTOIRE ! Bien joué. +++\n");
                    else if (msg_in.data[0] == 2) printf("\n--- DÉFAITE. ---\n");
                    else                          printf("\n=== MATCH NUL ===\n");
                    return -1;  // force la sortie de la boucle principale
                }
                            
                if (msg_in.type == MSG_ETAT_PLATEAU) {
                    efface_marqueurs();
                    affiche_plateau_recu(&msg_in);
                    printf("\u2705 Coup joue : %d -> %d\n", src, dst);
                    return 0;
                }
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
                    idsalon = choixSalon;
                    Message msg;
                    int inchat = 1;
                    int f = fork();
                    while(1){
                        if(f == 0){
                            if (recv_msg(dsock_mec, &msg) <= 0) {
                                break;
                            }
                            // On traite le message selon son type
                            if (msg.type == MSG_ETAT_PLATEAU) {
                                // Effacer l'écran pour un affichage propre
                                    printf("\033[H\033[J"); 
                                    printf("--- VUE SPECTATEUR ---\n");
                                    printf("\033c" );
                                    affiche_plateau_recu(&msg);
                            }
                        }
                        else{
                            gere_chat(&inchat, adServ,idsalon,idJoueur,1);
                            affiche_plateau_recu(&msg);
                        }

                    }          
                }else {
                    printf("Salon invalide ou plein.\n");
                }

            }

        if(!jeuEnCours) break;

        MessageClient msg = {choix, "", ""};
        strcpy(msg.idJoueur, idJoueur);
        send(dsock_mec, &msg, sizeof(msg), 0);

        if (choix == 'h') {
                    /* Envoyer la demande au serveur */
                    /* (le send ci-dessus l'a déjà envoyé) */

                    /* Recevoir la réponse */
                    Message m_histo;
                    if (recv_msg(dsock_mec, &m_histo) == 0 &&
                        m_histo.type == MSG_HISTO_REPONSE) {
                        int p  = m_histo.data[0];
                        int v  = m_histo.data[1];
                        int d  = m_histo.data[2];
                        int dr = m_histo.data[3];
                        printf("\n+==============================+\n");
                        printf("|  Historique de %-14s|\n", idJoueur);
                        printf("+==============================+\n");
                        printf("|  Parties jouees : %-10d|\n", p);
                        printf("|  Victoires      : %-10d|\n", v);
                        printf("|  Defaites       : %-10d|\n", d);
                        printf("|  Nuls           : %-10d|\n", dr);
                        if (p > 0)
                            printf("|  Win rate       : %-9.0f%%|\n",
                                (v * 100.0) / p);
                        printf("+==============================+\n");
                        printf("\nAppuyez sur Entree pour continuer...\n");
                        getchar();
                    }
                    continue;  /* retour au menu sans lancer de partie */
        }

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
            
            if (msg_jeu.type == MSG_PORT){
                idsalon = msg_jeu.data[0];
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
                    if (tour_client(dsock_mec, player_id, adServ, idJoueur) < 0) {
                        printf("Connexion perdue.\n");
                        jeuEnCours = 0;
                        break;
                    }
                }
                continue;
            }
            if (msg_jeu.type == MSG_ECHEC_ET_MAT_BLANC) {
                printf("\n*** ECHEC ET MAT - LES NOIRS GAGNENT ! ***\n");
                continue;
            }
            if (msg_jeu.type == MSG_ECHEC_ET_MAT_NOIR) {
                printf("\n*** ECHEC ET MAT - LES BLANCS GAGNENT ! ***\n");
                continue;
            }
            if (msg_jeu.type == MSG_VICTOIRE_FORFAIT) {
                printf("\n*** VICTOIRE PAR FORFAIT ! ***\n");
                jeuEnCours = 0;
                break;
            }
            if (msg_jeu.type == MSG_FIN_PARTIE) {
                if (msg_jeu.data[0] == 1)
                    printf("\n+++ VICTOIRE ! Bien joue. +++\n");
                else if (msg_jeu.data[0] == 2)
                    printf("\n--- DEFAITE. Meilleure chance la prochaine fois. ---\n");
                else
                    printf("\n=== MATCH NUL ===\n");
                jeuEnCours = 0;
                break;
            }

        }
        break;
    }
}

    close(dsock_mec);
    free(piece);
    return 0;
}