
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
    int portInit = 1070;
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
                        j.coupDuTour[sizeof(j.coupDuTour) - 1] = '\0';  // garantit le \0 final

                        printf("Coup du tour : %s\n", j.coupDuTour);
                    }
                    close(dmec_serv);  // ← fermer la socket client après la session
                }
                close(dsock);
            }
        }
    }

}