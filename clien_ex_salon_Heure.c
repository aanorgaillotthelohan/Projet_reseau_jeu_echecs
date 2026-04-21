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
#include <termios.h>
#include <time.h>


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
    
    // Vérification basique des arguments
    if (argc < 2) {
        printf("Oublie du port: %s <port>\n", argv[0]);
        exit(1);
    }

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
    // Conversion explicite du résultat de inet_pton pour éviter warning (retour int)
    int res = inet_pton(AF_INET, " 172.30.118.229", &(adServ.sin_addr)) ;
    gestion_erreur(res,"inet");
    
    socklen_t lgA = sizeof(struct sockaddr_in) ;
    
    //on demande une connection au serveur
    // Cast explicite pour compatibilité type sockaddr
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
    
    // Cast explicite (struct sockaddr *) pour chaque appel sendto/recvfrom
    int res_udp = sendto(dsock_udp, &message_udp, sizeof(message_udp), 0, (struct sockaddr *)&adServ, lgA);
    
    if (f == 0){
        time_t rawtime;
        struct tm * timeinfo;
        // Processus fils (client qui écrit)
        while(1){
            signal(SIGINT,mortfils);

            fgets(message_udp, sizeof(message_udp), stdin); 

            if(strcmp(message_udp,"Quit\n")==0){
                kill(pid_pere, SIGUSR1);
                memset(message_udp, 0, sizeof(message_udp));
                message_udp[0] = 'f';
                // Cast explicite
                res_udp = sendto(dsock_udp, &message_udp, sizeof(message_udp), 0, (struct sockaddr *)&adServ, lgA);
                gestion_erreur(res_udp, "sendto f"); 
                close(dsock_udp);
                close(dsock_mec); 
                exit(0);
            }
            //récupére l'heure actuelle
            time ( &rawtime );
            timeinfo = localtime ( &rawtime );

            char heure[32];
            strftime(heure, sizeof(heure), "↑ [%H:%M:%S]", timeinfo);
            //strcat(message_udp,timeinfo);
            strncat(message_udp, heure, sizeof(message_udp) - strlen(message_udp) - 1);
            // Cast explicite
            res_udp = sendto(dsock_udp, &message_udp, sizeof(message_udp), 0, (struct sockaddr *)&adServ, lgA);
            gestion_erreur(res_udp, "sendto chat");    
        }
    }
    else{
        // Processus père (client qui lit)
        while(1){
            signal(SIGINT,mortpere);
            signal(SIGUSR1,vraimort);
            timer +=1;
            
            // Cast explicite pour recvfrom
            ssize_t reschat = recvfrom(dsock_udp, stockmessage, sizeof(stockmessage), 0, (struct sockaddr *)&adServ, &lgA);
            gestion_erreur(reschat, "recvfrom chat");
            
            printf("\033c" );
            for (int i = 0; i < 10; i++){
                if(strlen(stockmessage[i]) > 0){
                    printf("message : %s \n", stockmessage[i]);
                    fflush(stdout);
                }
            }
            printf("-----------------\n");
        }
    } 
    
    close(dsock_udp);
    close(dsock_mec);
}