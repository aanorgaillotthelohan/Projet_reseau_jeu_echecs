#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>


// Si VerifIdentifiant(id) == true le pseudo est disponible, si c'ets faux le speudo est indisponible 
bool VerifIdentifiant(char *idJoueur){
    FILE *fp = fopen("HistoJoueur.txt", "r");
    char ligne[256];

    if (fp == NULL)
    {
        printf("Le fichier n'a pas pu être ouvert\n");
        return false;
    }
    while (fgets(ligne, sizeof(ligne), fp) != NULL) {
        if(strcmp(ligne,idJoueur) == 0){
            fclose(fp);
            return false;
        }
    }
    fclose(fp);
    return true;
}


void Inscrition(){
    char idJoueur[128];
    char Mdp[128];
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
        scanf(" %s",&idJoueur);
        verif = VerifIdentifiant(idJoueur);
    }

    printf("Saisir son mot de passe : ");
    scanf(" %s",Mdp);

    fprintf(fp, "%s %s\n",idJoueur,Mdp);
    fclose(fp);
}

int main(){
    char idJoueur[128];
    char Mdp[128];
    char CouI;
    bool verif = false;
    printf("Connection (c) / Inscription (i)\n");
    scanf(" %c",&CouI);
    
    while (CouI != 'c' && CouI != 'i') {
        printf("Choix invalide. Recommence :\n");
        printf("Connection (c) / Inscription (i)\n");
        scanf(" %c", &CouI);
    }

    if(CouI == 'i'){
        Inscrition();
    }else if(CouI == 'c'){
        printf("Saisir son identifiant : ");
        scanf(" %s",&idJoueur);
        verif = VerifIdentifiant(idJoueur);    
        while(verif == false){
                printf("identifiant déja pris \n");
                printf("Saisir un nouveau identifiant : ");
                scanf(" %s",&idJoueur);
                verif = VerifIdentifiant(idJoueur);
            }
        printf("Saisir son mot de passe : ");
        scanf(" %s",&Mdp);
    }

    printf("Jouer (j) / Être spéctateur (s) / Historique (h)\n");
    return 0;
}