
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



int main(){
    //côté SERVEUR_______________________________________________________
    int dsock = socket(PF_INET, SOCK_STREAM, 0); 
    //SOCK_STREAM nous fait fonctionner en tcp
    gestion_erreur(dsock, "socket"); //fct personnalisé
    struct sockaddr_in ad; //lieu de stock de la socket
    ad.sin_family = AF_INET;
    ad.sin_addr.s_addr =  INADDR_ANY ;
    ad.sin_port = htons((short)1061) ; 
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
        //on accepte la connexion du premier venu
        gestion_erreur(res,"accept");
        //on répond à sa requête. avec recv et send:
        int message[10];
        
        //ATTENTION au descripteur mit, ce n'est pas celui du serveur!
        ssize_t res_ss = recv(dmec_serv, message, sizeof(message), 0);
        //on recoit le message dans message.
        gestion_erreur(res_ss,"recv");
        
        printf("On recoit ");
        for (int i = 0; i <10; i++){
            printf(",%d ",message[i]);
        }
        printf("\n");
        /*
        res_ss = send(dsock, message, sizeof(message), 0) ;
        gestion_erreur(res_ss,"send");
        */
    }
    close(dsock);
}