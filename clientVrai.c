#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "application.h"

typedef struct {
    char choix;    
    char idJoueur[128];
    char Mdp[128];
} MessageClient;

int dsock_mec;
volatile int enRecherche = 0;

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

void gestion_erreur(int res, char* chaine){
    if(res == -1){
        perror(chaine);
        exit(1);
    }
}

int main(int argc, char **argv){
    if(argc < 2){
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    dsock_mec = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(dsock_mec, "socket");

    struct sockaddr_in adServ;
    adServ.sin_family = AF_INET;
    adServ.sin_port = htons(atoi(argv[1]));
    inet_pton(AF_INET, "127.0.0.1", &adServ.sin_addr);

    gestion_erreur(connect(dsock_mec, (struct sockaddr*)&adServ, sizeof(adServ)), "connect");

    pthread_t tid;
    pthread_create(&tid, NULL, listener, NULL);
    pthread_detach(tid);

    char idJoueur[128], Mdp[128];
    char choix;
    int connecte = 0;
    int jeuEnCours = 1;

    printf("Vous pouvez quitter avec '!'\n");

    while(jeuEnCours){

        // Phase connexion/inscription
        while(!connecte){
            printf("Connexion / Inscription (c/i) : ");
            scanf(" %c", &choix);
            choix = tolower(choix);

            if(choix == '!'){
                jeuEnCours = 0;
                break;
            }

            if(choix == 'i'){
                int rep;
                printf("Identifiant : "); scanf("%s", idJoueur);
                printf("Mot de passe : "); scanf("%s", Mdp);

                MessageClient msg = {'i', "", ""};
                strcpy(msg.idJoueur, idJoueur);
                strcpy(msg.Mdp, Mdp);

                send(dsock_mec, &msg, sizeof(msg), 0);
                recv(dsock_mec, &rep, sizeof(rep), 0);

                if(rep == 1){
                    printf("Inscription réussie ! Connecté.\n");
                    connecte = 1;
                } else {
                    printf("Identifiant déjà pris.\n");
                }
            }
            else if(choix == 'c'){
                printf("Identifiant : "); scanf("%s", idJoueur);
                printf("Mot de passe : "); scanf("%s", Mdp);

                MessageClient msg = {'c', "", ""};
                strcpy(msg.idJoueur, idJoueur);
                strcpy(msg.Mdp, Mdp);

                send(dsock_mec, &msg, sizeof(msg), 0);
                int rep = 0;
                recv(dsock_mec, &rep, sizeof(rep), 0);
                if(rep == 1){
                    printf("Connecté !\n");
                    connecte = 1;
                } else {
                    printf("Erreur de connexion.\n");
                }
            }
            else {
                printf("Choix invalide.\n");
            }
        }

        if(!jeuEnCours) break;

        // Menu principal
        char choixValid = 0;
        
        while(!choixValid){
            printf("Choix (j=jouer / s=spectateur / h=historique / !=quitter) : ");
            scanf(" %c", &choix);
            choix = tolower(choix);

            if(choix == '!'){
                jeuEnCours = 0;
                send(dsock_mec, &choix, sizeof(choix), 0);
                break;
            } 
            else if(choix == 'j' || choix == 's' || choix == 'h'){
                choixValid = 1;
                
            } 
            else {
                printf("Choix invalide\n");
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
        else if(choix == 's'){
            printf("Mode spectateur non implémenté.\n");
        }
        else if(choix == 'h'){
            printf("Historique non implémenté.\n");
        }
    }

    close(dsock_mec);
    return 0;
}