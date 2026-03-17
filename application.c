#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

bool VerifIdentifiant(idJoueur){
    FILE *fp = fopen("HistoJoueur.txt", "r");
    if (fp == NULL)
    {
        printf("Le fichier texte.txt n'a pas pu être ouvert\n");
        return false;
    }
    for ligne in fp {
        if ligne == idJoueur{
            return true;
        }else {
            return false;
        }
    }
}

int main(){
    char idJoueur;
    printf("Saisit son identifiant : ");
    scanf(" %d",&idJoueur);
    VerifIdentifiant(idJoueur);
    return 0;
}