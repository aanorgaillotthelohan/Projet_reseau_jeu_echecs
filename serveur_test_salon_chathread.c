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


#define NB_MAX 20
struct sockaddr_in liste_clients[NB_MAX];
int nb_cli = 0;

int connait_client(struct sockaddr_in * cli){
    for (int i = 0; i < nb_cli; i++){
        if (liste_clients[i].sin_addr.s_addr == cli->sin_addr.s_addr &&
            liste_clients[i].sin_port == cli->sin_port)
            return i;
    }
    return -1;
}

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


// Fonction pour le test de chat en thread
void * chat(void * arg){
    int dschat = ((struct chat_arg*)arg)->ds;
    // On est en UDP donc on peut utiliser le même port que le TCP du processus principal
    printf("port du chat : %d \n", ((struct chat_arg*)arg)->port + ((struct chat_arg*)arg)->nb);
    // SOCK_STREAM nous fait fonctionner en TCP
    // 5 est le nombre maximum de connexion qui peuvent être mises en attente

    socklen_t lgA = sizeof(struct sockaddr_in);
    char messagechat[128]; 
    struct sockaddr_in adClient;
    int chat_actif = 1;
    char stockmessage[10][128];
    int position_message = 0;

    while(chat_actif){
        // Reçoit seulement du mec qui est concerné par dschat, il faudrait une liste pour avoir tout le monde...
        ssize_t reschat = recvfrom(dschat, messagechat, sizeof(messagechat), 0, (struct sockaddr*)&adClient, &lgA);
        gestion_erreur((int)reschat, "chat res");

        if (strcmp(messagechat, "f") == 0){
            printf("Un chatteur est parti \n");
            int p = connait_client(&adClient);
            if (p != -1) {
                nb_cli--;
                for(int i = p; i < nb_cli; i++){
                    liste_clients[i] = liste_clients[i+1];
                }
            }
        }
        else{
            // printf("message : %s\n",messagechat);
            // Met le message dans la liste
            strcpy(stockmessage[position_message], messagechat);
            if (position_message < 10){
                position_message += 1;
            }
            else{ 
                for(int i = 0; i < 9; i++){
                    strcpy(stockmessage[i], stockmessage[i+1]);
                }
                position_message = 10;
                strcpy(stockmessage[9], messagechat);
            }
            if(connait_client(&adClient) == -1){
                if (nb_cli < NB_MAX){
                    liste_clients[nb_cli] = adClient;
                    nb_cli++;
                    printf("Client ajouté dans chat à la place %d \n", nb_cli-1);
                }
                else{
                    printf("Client rejeté par manque de place.\n");
                }
            }

            for(int i = 0; i < nb_cli; i++){
                ssize_t res_udp = sendto(dschat, stockmessage, sizeof(stockmessage), 0, (struct sockaddr*)&liste_clients[i], lgA);
                gestion_erreur((int)res_udp, "sendto udp chat");
            }
        }
    }
}


int main(){
    int nbSalon = 0; 
    int creerSalon = 0;
    int chat_deja_creer = 0;
    int portInit = 1082;
    pthread_t chat_t;
    
    while (1){
        printf("Est ce que je cree un salon ? (1-0)\n");
        scanf("%d", &creerSalon);
        
        if(creerSalon){
            int f = fork();
            nbSalon += 1;
            
            if (f == 0){
                // Pour chaque salon __________________________________________
                int dsock = socket(PF_INET, SOCK_STREAM, 0); 
                printf("port : %d \n", portInit + nbSalon);
                // SOCK_STREAM nous fait fonctionner en TCP
                gestion_erreur(dsock, "socket"); // fct personnalisé
                struct sockaddr_in ad; // lieu de stock de la socket
                ad.sin_family = AF_INET;
                ad.sin_addr.s_addr =  INADDR_ANY ;
                ad.sin_port = htons((short)(portInit + nbSalon)); 
                // le port qu'on lui donne doit être unique des autres socket en tcp
                // le port doit être supérieur à 1024
                int res = bind(dsock, (struct sockaddr*)&ad, sizeof(ad));
                gestion_erreur(res, "bind");

                res = listen(dsock, 5);
                // 5 est le nombre maximum de connexion qui peuvent être mises en attente, listen, n'est utilisé que par le tcp
                gestion_erreur(res, "listen");

                // Crée la socket pour le UDP du chat
                int dsockUdp = socket(PF_INET, SOCK_DGRAM, 0);
                res = bind(dsockUdp, (struct sockaddr*)&ad, sizeof(ad));
                gestion_erreur(res, "bind Udp");

                struct sockaddr_in adClient;
                // la taille de l'adresse
                socklen_t lgA = sizeof(struct sockaddr_in);
                struct chat_arg arg;
                arg.port = portInit;
                arg.nb = nbSalon;
                arg.ds = dsockUdp;
                pthread_create(&chat_t, NULL, chat, &arg);      

                while(1){
                    int dmec_serv = accept(dsock, (struct sockaddr *)&adClient, &lgA);
                    // on accepte la connexion du premier venu
                    gestion_erreur(dmec_serv, "accept"); // Correction : utiliser dmec_serv au lieu de res ici
                    
                    // on répond à sa requête. avec recv et send:
                    int message[10];
                    
                    // ATTENTION au descripteur mis, ce n'est pas celui du serveur!
                    ssize_t res_ss = recv(dmec_serv, message, sizeof(message), 0);
                    // on recoit le message dans message.
                    gestion_erreur((int)res_ss, "recv");
                    
                    printf("je suis %d\n", portInit + nbSalon);
                    printf("On recoit ");
                    for (int i = 0; i < 10; i++){
                        printf(",%d ", message[i]);
                    }
                    printf("\n");
                    /*
                    res_ss = send(dsock, message, sizeof(message), 0) ;
                    gestion_erreur(res_ss, "send");
                    */
                }
                close(dsock);
                pthread_join(chat_t, NULL);
            }
        }
    }
}