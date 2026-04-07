#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "application.h"

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

int main(int argc, char **argv){

    if(argc < 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int dsock_mec = socket(PF_INET, SOCK_STREAM, 0);
    gestion_erreur(dsock_mec, "socket");

    struct sockaddr_in adServ ;
    adServ.sin_family = AF_INET ;
    adServ.sin_port = htons(atoi(argv[1])) ;
    inet_pton(AF_INET, "127.0.0.1", &(adServ.sin_addr)) ;

    gestion_erreur(connect(dsock_mec, (struct sockaddr *) &adServ, sizeof(adServ)), "connect");

    char idJoueur[128], Mdp[128];
    char choix;
    bool JeuEnCour = true;

    printf("Vous quitter le jeu a tout moment en apuyant sur ! (! = quitter) : "); 
    while(JeuEnCour){
        printf("Voulez vous vous connecter ou vous inscrire (c = connexion / i = inscription) : ");
        scanf(" %c", &choix);
        choix = tolower(choix);

        if(choix == '!'){
            JeuEnCour = false;
            break;
        }
        else if(choix == 'i'){
            Inscription(idJoueur, Mdp);
        } 
        else if(choix == 'c'){
            printf("Saisir identifiant : ");
            scanf("%s", idJoueur);
            printf("Saisir mot de passe : ");
            scanf("%s", Mdp);

            if(!VerifConnexion(idJoueur, Mdp)){
                char ch = ChoixConnexionNonReconnu();
                if(ch == 'i'){
                    Inscription(idJoueur, Mdp);
                }
            }
        } else {
            printf("Choix invalide\n");
            close(dsock_mec);
            exit(1);
        }

        MessageClient msg;
        msg.choix = choix;
        strcpy(msg.idJoueur, idJoueur);
        strcpy(msg.Mdp, Mdp);

        send(dsock_mec, &msg, sizeof(msg), 0);

        char rep;
        recv(dsock_mec, &rep, sizeof(rep), 0);

        if(rep == 1){
            printf("Validation serveur OK\n");
        }else{
            printf("Refus serveur\n");
        }

        char choixValid = 0;

        while(!choixValid){
            printf("Souhaitez-vous Jouer, être spectateur ou regarder votre historique ? (j = Jouer / s = Spectateur / h = historique) : ");
            scanf(" %c", &choix);
            choix = tolower(choix);
            if(choix =='!'){
                JeuEnCour = false;
                break;
            } else if(choix == 'j' || choix == 's' || choix == 'h'){
                choixValid = 1;
            } else {
                printf("Choix invalide. Veuillez saisir uniquement j, s ou h.\n");
            }
        }
        if(choix == '!'){
            JeuEnCour = false;
            break;
        }
        else if(choix == 'j'){
            // lancer le jeu
        }
        else if (choix == 's'){
            // mode spectateur
        }
        else if(choix == 'h'){
            // afficher historique
        }
    }

    close(dsock_mec);
}