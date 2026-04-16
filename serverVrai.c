#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "application.h"
#include "players.h"
#include "salon.h"

#define PORT 1061
#define MAX_CLIENTS 50

typedef struct {
    char choix;
    char idJoueur[128];
    char Mdp[128];
} MessageClient;

joueur JoueursEnAttente[MAX_CLIENTS];
int debut = 0;
int fin = 0;
int NbSalons = 0;

pthread_mutex_t mutexJoueurs = PTHREAD_MUTEX_INITIALIZER;

void retirer_joueur_file(int sockClient){
    pthread_mutex_lock(&mutexJoueurs);

    int index = -1;

    for(int i = debut; i < fin; i++){
        if(JoueursEnAttente[i].socketJoueur == sockClient){
            index = i;
            break;
        }
    }

    if(index != -1){
        for(int i = index; i < fin - 1; i++){
            JoueursEnAttente[i] = JoueursEnAttente[i + 1];
        }
        fin--;

        printf("Joueur retiré de la file\n");
    }

    pthread_mutex_unlock(&mutexJoueurs);
}

// Fonction pour gérer un client
void* gestion_client(void* arg ){
    int sockClient = *(int*)arg;
    free(arg);

    MessageClient msg;
    while(1){
        ssize_t r = recv(sockClient, &msg, sizeof(msg), 0);
        if(r <= 0){
            printf("Client déconnecté\n");
            retirer_joueur_file(sockClient);
            close(sockClient);
            return NULL;
        }

        int rep = 0;
        if(msg.choix == '!'){
            retirer_joueur_file(sockClient);
            close(sockClient);
            return NULL;
        }
        else if(msg.choix == 'i'){ // Inscription
            rep = Inscription(msg.idJoueur, msg.Mdp);
            send(sockClient, &rep, sizeof(rep), 0);
        }
        else if(msg.choix == 'c'){ // Connexion
            if(VerifConnexion(msg.idJoueur, msg.Mdp)){
                rep = 1;
            }
            send(sockClient, &rep, sizeof(rep), 0);
        }
        else if(msg.choix == 'j'){ // Jouer
            joueur* j = creejoueur(sockClient, msg.idJoueur);

            pthread_mutex_lock(&mutexJoueurs);
            if(fin < MAX_CLIENTS){
                JoueursEnAttente[fin++] = *j;
            } else {
                printf("File d'attente pleine !\n");
            }

            // Si au moins 2 joueurs, créer un salon
            if(fin - debut >= 2){
                joueur j1 = JoueursEnAttente[debut];
                joueur j2 = JoueursEnAttente[debut+1];

                NbSalons++;
                Salon* s = creeSalon(NbSalons, &j1, &j2);
                printf("Salon %d créé entre %s et %s\n", s->idSalon, j1.NomJoueur, j2.NomJoueur);

                int matchTrouve = 1; // int pour signaler match
                send(j1.socketJoueur, &matchTrouve, sizeof(matchTrouve), 0);
                send(j2.socketJoueur, &matchTrouve, sizeof(matchTrouve), 0);

                // Avancer la pile
                debut += 2;

                // Glisser le tableau pour réutiliser l’espace
                if(debut > 0 && fin >= MAX_CLIENTS/2){
                    memmove(JoueursEnAttente, &JoueursEnAttente[debut], (fin - debut) * sizeof(joueur));
                    fin -= debut;
                    debut = 0;
                }
            }
            pthread_mutex_unlock(&mutexJoueurs);
        }
    }
}

int main(){
    int dsock = socket(PF_INET, SOCK_STREAM, 0);
    if(dsock == -1){ perror("socket"); exit(1); }

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(PORT);

    if(bind(dsock, (struct sockaddr*)&ad, sizeof(ad)) == -1){ perror("bind"); exit(1); }
    if(listen(dsock, MAX_CLIENTS) == -1){ perror("listen"); exit(1); }

    printf("Serveur en écoute sur le port %d...\n", PORT);


    memset(JoueursEnAttente, 0, sizeof(JoueursEnAttente));


    while(1){
        struct sockaddr_in adClient;
        socklen_t lgA = sizeof(adClient);

        int* sockClient = malloc(sizeof(int));
        *sockClient = accept(dsock, (struct sockaddr*)&adClient, &lgA);
        if(*sockClient == -1){ perror("accept"); free(sockClient); continue; }

        printf("Client connecté !\n");

        pthread_t tid;
        pthread_create(&tid, NULL, gestion_client, sockClient);
        pthread_detach(tid); // pas besoin de join
    }

    close(dsock);
    return 0;

}