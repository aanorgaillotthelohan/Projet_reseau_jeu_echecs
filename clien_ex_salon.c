
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
#include <sys/select.h>
#include <fcntl.h>     
#include <termios.h>

pid_t pid_serv;

void gestion_erreur(int res, char* chaine){
    if(res == -1 ){
        printf("Erreur lors de %s \n", chaine);
        exit(1);
    }
}

void mortpere(){
    printf("Non, tapez 'Quit' pour quitter \n");
}
void mortfils(){
}


void vraimort(){
    exit(1);
}


int main(int argc, char **argv){
    //côté LAUTRE LA JAIME PAS LE MOT CLIENT__________________________________________________
    int dsock_mec = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(dsock_mec, "socket de mec");
    int dsock_udp = socket(PF_INET, SOCK_DGRAM, 0);
    gestion_erreur(dsock_udp, "sock udp");
    int port = atoi(argv[1]);
    printf("%d \n",port);
    //le message qu'on vas envoyer
    //EXEMPLE DE MESSAGE, NE VEUT RIEN DIRE
    int message_mec[10];
    for (int i = 0; i <10; i++){
        message_mec[i] = i;
    }
    
    //on crée l'adresse serveur à laquelle on veut envoyer:
    struct sockaddr_in adServ ;
    adServ.sin_family = AF_INET ;
    adServ.sin_port = htons(port) ;
    int res = inet_pton(AF_INET, "10.111.3.108", &(adServ.sin_addr)) ;
    gestion_erreur(res,"inet");
    socklen_t lgA = sizeof(struct sockaddr_in) ;
    //on demande une connection au serveur
    res = connect(dsock_mec, (struct sockaddr *) &adServ, lgA);
    gestion_erreur(res, "connect");
    ssize_t res_ss_mec = send(dsock_mec, message_mec, sizeof(message_mec), 0) ;
    gestion_erreur(res_ss_mec,"send");
    
    char message_udp[128];
    char stockmessage[10][128];
    fd_set set;
    struct timeval timeout;
    memset(stockmessage, 0, sizeof(stockmessage)); //pour initialiser à 0 apparemment

    int pid_pere = getpid();
    int f = fork();
    int timer = 0;
    ssize_t res_udp = sendto(dsock_udp,&message_udp,sizeof(message_udp),0,&adServ,lgA);
    
    if (f == 0){
        while(1){
            signal(SIGINT,mortfils);

            /*
            // 1. Initialiser le jeu de descripteurs à surveiller
            FD_ZERO(&set);
            FD_SET(STDIN_FILENO, &set); // On surveille le clavier (entrée standard)
            FD_SET(dsock_udp, &set);    // On surveille la socket UDP

            // 2. Définir un timeout (optionnel, ici 1 seconde pour éviter un blocage total si besoin)
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            // 3. Attendre qu'un des descripteurs soit prêt
            res = select(dsock_udp + 1, &set, NULL, NULL, &timeout);
            */
            //gestion_erreur(res,"select");
            //if (FD_ISSET(STDIN_FILENO, &set)){
            fgets(message_udp,sizeof(message_udp),stdin); //....
            if(strcmp(message_udp,"Quit\n")==0){
                kill(pid_pere,SIGUSR1);
                memset(message_udp, 0, sizeof(message_udp));
                message_udp[0] = 'f';
                ssize_t res_udp = sendto(dsock_udp,&message_udp,sizeof(message_udp),0,&adServ,lgA);
                gestion_erreur(res_udp, "sendto f"); 
                close(dsock_udp);
                close(dsock_mec); 
                exit(0);
            }
            ssize_t res_udp = sendto(dsock_udp,&message_udp,sizeof(message_udp),0,&adServ,lgA);
            gestion_erreur(res_udp, "sendto chat");    
            
            //}

            
            }
    }
    else{
        while(1){
            signal(SIGINT,mortpere);
            signal(SIGUSR1,vraimort);
            timer +=1;
            int reschat = recvfrom(dsock_udp, stockmessage, sizeof(stockmessage), 0, &adServ,&lgA);
            gestion_erreur(reschat,"recfrom chat");
            //printf("\e[1;1H\e[2J");
            //printf("\033[2J");
            printf("\033c" );
            for (int i = 0; i< 10; i++){
                if(strlen(stockmessage[i]) > 0){
                    printf("message : %s \n",stockmessage[i]);
                    fflush(stdout);
                }
            }
            printf("-----------------\n");
        }
    } 
    /*
    res_ss_mec = recv(dsock_mec, message_mec, sizeof(message_mec), 0);
    //on recoit le message dans message.
    gestion_erreur(res_ss_mec,"recv");
    */
    close(dsock_udp);
    close(dsock_mec);
}
