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
#include "application.h" 
#include "players.h"
#include "salon.h"

#define PORT 1061
#define MAX_CLIENTS 5

typedef struct {
    char choix;    
    char idJoueur[128];
    char Mdp[128];
} MessageClient;

void gestion_erreur(int res, char* chaine){
    if(res == -1 ){
        printf("Erreur lors de %s \n", chaine);
        exit(1);
    }
}

int main(){
    int dsock = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(dsock, "socket");

    struct sockaddr_in ad;
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr = INADDR_ANY;
    ad.sin_port = htons(PORT);

    int res = bind(dsock, (struct sockaddr*)&ad, sizeof(ad));
    gestion_erreur(res, "bind");

    res = listen(dsock, MAX_CLIENTS);
    gestion_erreur(res, "listen");

    int NbSalons =0;

    printf("Serveur en écoute sur le port %d...\n", PORT);

    joueur JoueursEnAttente[50];
    memset(JoueursEnAttente, 0, sizeof(JoueursEnAttente));


    while(1){
        struct sockaddr_in adClient;
        socklen_t lgA = sizeof(adClient);

        int sockClient = accept(dsock, (struct sockaddr*)&adClient, &lgA);
        gestion_erreur(sockClient, "accept");

        printf("Client connecté !\n");

        MessageClient msg;
        ssize_t r = recv(sockClient, &msg, sizeof(msg), 0);
        gestion_erreur(r, "receive");

        printf("Reçu :\n");
        printf("Choix: %c\n", msg.choix);
        printf("ID: %s\n", msg.idJoueur);
        printf("MDP: %s\n", msg.Mdp);

        char rep = 0;        

        if(msg.choix == 'c'){
            if(VerifConnexion(msg.idJoueur, msg.Mdp)){
                rep = 1;
            }
        }
        else if(msg.choix == 'i'){
            Inscription(msg.idJoueur, msg.Mdp);
            rep = 1;
        }
        else if(msg.choix == 'j'){
            joueur* j = creejoueur(sockClient, msg.idJoueur);

            // Ajouter un joueur dans la file
            for(int i = 0; i < 50; i++) {
                if (JoueursEnAttente[i].socketJoueur == 0) {
                    JoueursEnAttente[i] = *j;
                    break;
                }
            }

            // Chercher 2 joueurs

            int i1 = -1, i2 = -1;
            bool joueur1_trouve = false;
            bool joueur2_trouve = false;

            for(int i = 0; i < 50; i++) {
                if (JoueursEnAttente[i].socketJoueur != 0) {

                    if (!joueur1_trouve) {
                        i1 = i;
                        joueur1_trouve = true;
                    }
                    else if (!joueur2_trouve) {
                        i2 = i;
                        joueur2_trouve = true;
                        break;
                    }
                }
            }

            // Si 2 joueurs → créer salon
            if (i1 != -1 && i2 != -1){
                NbSalons++;

                joueur j1 = JoueursEnAttente[i1];
                joueur j2 = JoueursEnAttente[i2];

                Salon* s = creeSalon(NbSalons, &j1, &j2);

                printf("Salon %d créé entre %s et %s\n",
                    s->idSalon,
                    j1.NomJoueur,
                    j2.NomJoueur);

                send(j1.socketJoueur, "Match trouvé !\n", 16, 0);
                send(j2.socketJoueur, "Match trouvé !\n", 16, 0);

                // retirer de la file
                JoueursEnAttente[i1].socketJoueur = 0;
                JoueursEnAttente[i2].socketJoueur = 0;
            }
            else {
                printf("Joueur en attente d'un adversaire...\n");
            }
        }
    close(dsock);
    return 0;
    }
}