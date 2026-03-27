
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
    int res = inet_pton(AF_INET, "10.111.3.109", &(adServ.sin_addr)) ;
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

    int f = fork();
    int timer = 0;
    if (f == 0){
        while(1){
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
            fgets(); //....
            ssize_t res_udp = sendto(dsock_udp,&message_udp,sizeof(message_udp),0,&adServ,lgA);
            gestion_erreur(res_udp, "sendto chat");    
            //}
            }
    }
    else{
        while(1){
            timer +=1;
            int reschat = recvfrom(dsock_udp, stockmessage, sizeof(stockmessage), 0, &adServ,&lgA);
            gestion_erreur(reschat,"recfrom chat");
            for (int i = 0; i< 10; i++){
                if(strlen(stockmessage[i]) > 0){
                    printf("message : %s \n",stockmessage[i]);
                    fflush(stdout);
                }
            }
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



/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h> // Nécessaire pour select()
#include <fcntl.h>      // Nécessaire pour F_GETFL, F_SETFL
#include <termios.h>    // Pour gérer le mode du terminal (optionnel mais utile)

// ... (votre fonction gestion_erreur) ...

int main(int argc, char **argv){
    // ... (initialisation sockets dsock_mec et dsock_udp, adresse adServ) ...
    // Supposons que dsock_udp, adServ, lgA sont déjà initialisés comme dans votre code

    fd_set set;
    struct timeval timeout;
    int res;
    char message_udp[128];
    char stockmessage[128]; // On reçoit un message à la fois ou un paquet, adaptez selon le serveur

    printf("Connecté. Tapez vos messages (Ctrl+C pour quitter).\n");

    while(1){
        // 1. Initialiser le jeu de descripteurs à surveiller
        FD_ZERO(&set);
        FD_SET(STDIN_FILENO, &set); // On surveille le clavier (entrée standard)
        FD_SET(dsock_udp, &set);    // On surveille la socket UDP

        // 2. Définir un timeout (optionnel, ici 1 seconde pour éviter un blocage total si besoin)
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        // 3. Attendre qu'un des descripteurs soit prêt
        res = select(dsock_udp + 1, &set, NULL, NULL, &timeout);

        if (res == -1) {
            perror("select");
            break;
        }

        // 4. Si des données sont arrivées sur le réseau (Réception)
        if (FD_ISSET(dsock_udp, &set)) {
            ssize_t rcv_len = recvfrom(dsock_udp, stockmessage, sizeof(stockmessage), 0, NULL, NULL);
            if (rcv_len > 0) {
                // Attention: votre serveur envoie un tableau de 10 messages. 
                // Ici on suppose qu'on reçoit un flux. Si c'est le tableau entier :
                // Il faudra parser le tableau reçu. Pour l'exemple, on affiche tel quel.
                printf("\n[Nouveau Message] %.*s\n> ", (int)rcv_len, stockmessage);
                fflush(stdout); // Force l'affichage immédiat
            }
        }

        // 5. Si l'utilisateur a tapé quelque chose (Envoi)
        if (FD_ISSET(STDIN_FILENO, &set)) {
            if (fgets(message_udp, sizeof(message_udp), stdin) != NULL) {
                // Retirer le \n laissé par fgets si nécessaire
                message_udp[strcspn(message_udp, "\n")] = 0;
                
                if (strlen(message_udp) > 0) {
                    ssize_t res_udp = sendto(dsock_udp, message_udp, strlen(message_udp), 0, 
                                             (struct sockaddr*)&adServ, sizeof(adServ));
                    if (res_udp == -1) perror("sendto");
                }
            } else {
                // Fin de flux (ex: Ctrl+D)
                break;
            }
        }
    }

    close(dsock_udp);
    close(dsock_mec);
    return 0;
}
*/

//pour remplacer fgets bloquant