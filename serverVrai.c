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

    printf("Serveur en écoute sur le port %d...\n", PORT);

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

        send(sockClient, &rep, sizeof(rep), 0);

        close(sockClient);
    }

    close(dsock);
    return 0;
}