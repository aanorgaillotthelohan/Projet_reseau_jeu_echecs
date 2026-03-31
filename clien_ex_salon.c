
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

#define PORT 8080


void gestion_erreur(int res, char* chaine){
    if(res == -1 ){
        printf("Erreur lors de %s \n", chaine);
        exit(1);
    }
}

int main(int argc, char **argv){
    //côté LAUTRE LA JAIME PAS LE MOT CLIENT__________________________________________________
    int dsock_mec = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(dsock_mec, "socket de mec");
    int port = atoi(argv[1]);
    printf("%d \n",port);
    //le message qu'on vas envoyer

    char message_mec[256];
    
    //on crée l'adresse serveur à laquelle on veut envoyer:
    struct sockaddr_in adServ ;
    adServ.sin_family = AF_INET ;
    adServ.sin_port = htons(port) ;
    int res = inet_pton(AF_INET, "127.0.0.1", &(adServ.sin_addr)) ;
    gestion_erreur(res,"inet");
    socklen_t lgA = sizeof(struct sockaddr_in) ;
    //on demande une connection au serveur
    res = connect(dsock_mec, (struct sockaddr *) &adServ, lgA);
    gestion_erreur(res, "connect");

    while(1) {
        printf("Entrez votre message : ");
        fgets(message_mec, sizeof(message_mec), stdin);
        
        if(strncmp(message_mec, "quit", 4) == 0) break;  // taper "quit" pour sortir
        
        ssize_t res_ss_mec = send(dsock_mec, message_mec, strlen(message_mec), 0);
        gestion_erreur(res_ss_mec, "send");
    }

    close(dsock_mec);
    /*
    ssize_t res_ss_mec = send(dsock_mec, message_mec, strlen(message_mec), 0);
    gestion_erreur(res_ss_mec,"send");
    
    res_ss_mec = recv(dsock_mec, message_mec, sizeof(message_mec), 0);
    //on recoit le message dans message.
    gestion_erreur(res_ss_mec,"recv");
    */
    
    close(dsock_mec);
}   