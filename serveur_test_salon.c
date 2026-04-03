
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <dirent.h>
#include <stddef.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "chess.h"
#define PORT 8080

/*
void gestion_erreur(int res, char* chaine){
    if(res == -1 ){
        printf("Erreur lors de %s \n", chaine);
        exit(1);
    }
}



int main(){
    joueur j;
    int socklist[100] ;
    int nbSalon = 0; 
    int creerSalon = 0;
    int portInit = 1071;
    while (1){
        printf("Est ce que je cree un salon ? (1-0)");
        scanf("%d",&creerSalon);
        if(creerSalon){
            int f = fork();
            nbSalon += 1;
            if (f == 0){
                //pour chaque salon 
                //côté SERVEUR_______________________________________________________
                int dsock = socket(PF_INET, SOCK_STREAM, 0); 
                printf("port : %d \n",portInit + nbSalon);
                //SOCK_STREAM nous fait fonctionner en tcp
                gestion_erreur(dsock, "socket"); //fct personnalisé
                struct sockaddr_in ad; //lieu de stock de la socket
                ad.sin_family = AF_INET;
                ad.sin_addr.s_addr =  INADDR_ANY ;
                ad.sin_port = htons((short)portInit + nbSalon) ; 
                //le port qu'on lui donne doit être unique des autres socket en tcp
                //le port doit être supérieur à 1024
                int res = bind(dsock, (struct sockaddr*)&ad, sizeof(ad)) ;
                gestion_erreur(res,"bind");
                res = listen(dsock, 5);
                //5 est le nombre maximum de connexion qui peuvent être misent en attente
                gestion_erreur(res,"listen");
                struct sockaddr_in adClient ;
                socklen_t lgA = sizeof(struct sockaddr_in);
                //la taille de l'adresse
                
                while(1){
                    int dmec_serv = accept(dsock, (struct sockaddr *) &adClient, &lgA);
                    gestion_erreur(dmec_serv, "accept");   // ← correction : tester dmec_serv, pas res

                    char message[256];

                    // Boucle recv : lit tous les messages envoyés par ce client
                    while(1){
                        memset(message, 0, sizeof(message));
                        ssize_t res_ss = recv(dmec_serv, message, sizeof(message), 0);

                        if(res_ss <= 0) {
                            printf("Client déconnecté\n");
                            break;
                        }
                        
                        printf("Message reçu : %s\n", message);
                        printf("ça continue ?\n");

                        // Copie sécurisée dans coupDuTour (taille 10 max)
                        strncpy(j.coupDuTour, message, sizeof(j.coupDuTour) - 1);
                        
                        printf("Coup du tour : %d\n", j.coupDuTour);
                    }
                    close(dmec_serv);  // ← fermer la socket client après la session
                }
                close(dsock);
            }
        }
    }

}
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <locale.h>
#include "chess.h"

void gestion_erreur(int res, char* chaine){
    if(res == -1){
        printf("Erreur lors de %s \n", chaine);
        exit(1);
    }
}

int main(){
    setlocale(LC_ALL, "");  /* Unicode pour wprintf */

    /* ── Initialisation du plateau ── */
    eliminated_pieces_white.start = NULL;
    eliminated_pieces_black.start = NULL;
    piece = (char *)malloc(sizeof(char));
    board[7][7] = 'R';

    /* ── Initialisation des deux joueurs ── */
    joueur j_blanc, j_noir;
    memset(&j_blanc, 0, sizeof(joueur));
    memset(&j_noir,  0, sizeof(joueur));
    strncpy(j_blanc.Couleur, "blanc", 20);
    strncpy(j_noir.Couleur,  "noir",  20);

    int nbSalon   = 0;
    int creerSalon = 0;
    int portInit  = 1070;

    while(1){
        printf("Est ce que je cree un salon ? (1-0) : ");
        scanf("%d", &creerSalon);
        if(creerSalon){
            int f = fork();
            nbSalon += 1;
            if(f == 0){
                /* ══ Processus fils : gestion d'un salon ══ */
                int dsock = socket(PF_INET, SOCK_STREAM, 0);
                gestion_erreur(dsock, "socket");

                /* Permet de réutiliser le port immédiatement */
                int opt = 1;
                setsockopt(dsock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

                struct sockaddr_in ad;
                ad.sin_family      = AF_INET;
                ad.sin_addr.s_addr = INADDR_ANY;
                ad.sin_port        = htons((short)(portInit + nbSalon));
                printf("Salon ouvert sur le port : %d\n", portInit + nbSalon);

                int res = bind(dsock, (struct sockaddr*)&ad, sizeof(ad));
                gestion_erreur(res, "bind");
                res = listen(dsock, 5);
                gestion_erreur(res, "listen");

                struct sockaddr_in adClient;
                socklen_t lgA = sizeof(struct sockaddr_in);

                int tour = 1;  /* 1 = blanc, 2 = noir */

                while(1){
                    /* ── Attente d'un coup du client ── */
                    int dmec_serv = accept(dsock, (struct sockaddr*)&adClient, &lgA);
                    gestion_erreur(dmec_serv, "accept");

                    char message[256];
                    memset(message, 0, sizeof(message));
                    ssize_t res_ss = recv(dmec_serv, message, sizeof(message) - 1, 0);

                    if(res_ss <= 0){
                        printf("Client déconnecté\n");
                        close(dmec_serv);
                        break;
                    }

                    /* Supprimer le '\n' éventuel envoyé par fgets côté client */
                    message[strcspn(message, "\n")] = '\0';

                    printf("Coup reçu : \"%s\"\n", message);

                    /* ── Copier le coup dans la structure du joueur concerné ── */
                    if(tour % 2 != 0){
                        strncpy(j_blanc.coupDuTour, message, sizeof(j_blanc.coupDuTour) - 1);
                        /* ── Jouer le coup sur le plateau ── */
                        system("clear");
                        display();
                        player_white(&j_blanc);
                    } else {
                        strncpy(j_noir.coupDuTour, message, sizeof(j_noir.coupDuTour) - 1);
                        /* ── Jouer le coup sur le plateau ── */
                        system("clear");
                        display();
                        player_black(&j_noir);
                    }

                    tour++;
                    close(dmec_serv);
                }

                close(dsock);
                free(piece);
                exit(0);  /* fin du processus fils */
            }
            /* Le père reprend sa boucle pour créer d'autres salons */
        }
    }
}