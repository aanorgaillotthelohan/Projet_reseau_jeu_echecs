#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>


// Si VerifIdentifiant(id) == true le pseudo est disponible, si c'est faux le pseudo est déja pris 
bool VerifIdentifiant(char *idJoueur){
    FILE *fp = fopen("HistoJoueur.txt", "r");
    char ligne[256];

    if (fp == NULL)
    {
        printf("Le fichier n'a pas pu être ouvert\n");
        return false;
    }
    while (fgets(ligne, sizeof(ligne), fp) != NULL) {
        ligne[strcspn(ligne, "\n")] = '\0';
        if(strcmp(ligne,idJoueur) == 0){
            fclose(fp);
            return false;
        }
    }
    fclose(fp);
    return true;
}

bool VerifConnexion(char *idJoueur, char *Mdp){
    FILE *fp = fopen("HistoJoueur.txt", "r");
    char ligne[256];

    if (fp == NULL){
        printf("Le fichier n'a pas pu être ouvert\n");
        return false;
    }

    while(fgets(ligne, sizeof(ligne), fp) != NULL){
        ligne[strcspn(ligne, "\n")] = '\0'; // enlever \n

        char pseudo[128], motdepasse[128];
        sscanf(ligne, "%s %s", pseudo, motdepasse);
        if(strcmp(pseudo, idJoueur) == 0 && strcmp(motdepasse, Mdp) == 0){
            fclose(fp);
            return true; // identifiant et mot de passe corrects
        }
    }

    fclose(fp);
    return false; // identifiant ou mot de passe incorrect
}

void Inscription(char *idJoueur, char *Mdp){
    bool verif = false;
    FILE *fp = fopen("HistoJoueur.txt", "a+");

    if (fp == NULL)
    {
        printf("Le fichier n'a pas pu être ouvert\n");
        return;
    }
    printf("Saisir son identifiant : ");
    scanf(" %s",idJoueur);

    verif = VerifIdentifiant(idJoueur);

    while(verif == false){
        printf("identifiant déja pris \n");
        printf("Saisir un nouveau identifiant : ");
        scanf(" %s",idJoueur);
        verif = VerifIdentifiant(idJoueur);
    }

    printf("Saisir son mot de passe : ");
    scanf(" %s",Mdp);

    fprintf(fp, "%s %s\n",idJoueur,Mdp);
    fclose(fp);
}

char ChoixConnexionNonReconnu(){
    char choix = '\0';
    while(choix != 'r' && choix != 'i'){
        printf("Identifiant ou mot de passe non reconnu.\n");
        printf("Voulez-vous réessayer (r) ou vous inscrire (i) ?\n");
        scanf(" %c", &choix);
        choix = tolower(choix);
    }
    return choix;
}

int main(){
    char CouI = '\0';
    char idJoueur[128];
    char Mdp[128];

    // Choix initial connexion ou inscription
    while(CouI != 'c' && CouI != 'i'){
        printf("Connection (c) / Inscription (i)\n");
        scanf(" %c", &CouI);
        CouI = tolower(CouI);
        if(CouI != 'c' && CouI != 'i'){
            printf("Choix invalide. Veuillez saisir 'c' ou 'i'.\n");
        }
    }

    // Inscription directe
    if(CouI == 'i'){
        Inscription(idJoueur, Mdp);
    }

    // Connexion
    while(CouI == 'c'){
        printf("Saisir son identifiant : ");
        scanf(" %s", idJoueur);
        printf("Saisir son mot de passe : ");
        scanf(" %s", Mdp);

        if(VerifConnexion(idJoueur, Mdp)){
            printf("Connexion réussie !\n");
            CouI = 'f'; // sortir de la boucle
        } else {
            char choix = ChoixConnexionNonReconnu();
            if(choix == 'i'){
                Inscription(idJoueur, Mdp);
                CouI = 'f'; // sortir de la boucle
            }
            // sinon la boucle recommence pour réessayer
        }
    }

    printf("Jouer (j) / Être spectateur (s) / Historique (h)\n");
    return 0;
}