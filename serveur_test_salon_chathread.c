
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

struct chat_arg{
    int port;
    int nb;
    int ds;
};


void chat(void * arg){
    int dschat = ((struct chat_arg*)arg)->ds;
    printf("port du chat : %d \n",((struct chat_arg*)arg)->port + ((struct chat_arg*)arg)->nb);
    //SOCK_STREAM nous fait fonctionner en tcp
    int res = listen(dschat, 5);
    //5 est le nombre maximum de connexion qui peuvent être misent en attente
    gestion_erreur(res,"listen");
    socklen_t lgA = sizeof(struct sockaddr_in);
    int * messagechat; 
    struct sockaddr_in adClient ;
    int chat_actif = 0;
    while(chat_actif){
        int reschat = recvfrom(dschat, messagechat, sizeof(messagechat), 0, &adClient,&lgA);
    }
}


int main(){
    int socklist[100] ;
    int nbSalon = 0; 
    int creerSalon = 0;
    int chat_deja_creer = 0;
    int portInit = 1090;
    int portInitChat = 1275;
    while (1){
        printf("Est ce que je cree un salon ? (1-0)");
        scanf("%d",&creerSalon);
        if(creerSalon){
            int f = fork();
            nbSalon += 1;
            if (f == 0){
                //pour chaque salon __________________________________________
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

                pthread_t chat_t;
                struct chat_arg arg;
                arg.port = portInit;
                arg.nb = nbSalon;
                arg.ds = dsock;
                pthread_create(&chat_t, 0, chat, &arg);      

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
                    
                    printf("je suis %d\n", portInit + nbSalon);
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
                pthread_join(chat_t, 0);
            }
        }
    }

}