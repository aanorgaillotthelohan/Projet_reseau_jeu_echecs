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
#include "application.c" 

#define PORT 1061
#define MAX_CLIENTS 5

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

    printf("Serveur en écoute sur le port %d...\n", PORT);

    while(1){
        struct sockaddr_in adClient;
        socklen_t lgA = sizeof(adClient);
        int sockClient = accept(dsock, (struct sockaddr*)&adClient, &lgA);
        gestion_erreur(sockClient, "accept");

        printf("Client connecté !\n");

        char idJoueur[128], Mdp[128];
        char choix;

        printf("Choix du client (c = connexion / i = inscription) : ");
        scanf(" %c", &choix);
        choix = tolower(choix);

        if(choix == 'i'){
            Inscription(idJoueur, Mdp);
            printf("Inscription réussie : %s / %s\n", idJoueur, Mdp);
        } 
        else if(choix == 'c'){
            printf("Saisir identifiant : ");
            scanf(" %s", idJoueur);
            printf("Saisir mot de passe : ");
            scanf(" %s", Mdp);

            if(VerifConnexion(idJoueur, Mdp)){
                printf("Connexion réussie !\n");
            } else {
                char ch = ChoixConnexionNonReconnu();
                if(ch == 'i'){
                    Inscription(idJoueur, Mdp);
                    printf("Inscription après échec : %s / %s\n", idJoueur, Mdp);
                }
            }
        }
        close(sockClient);
    }
    close(dsock);
    return 0;
}