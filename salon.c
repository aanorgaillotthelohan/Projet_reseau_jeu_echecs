#include "salon.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Salon *creeSalon(int id, joueur *j1, joueur *j2)
{
    Salon *s = malloc(sizeof(Salon));
    if (s == NULL) return NULL;

    s->idSalon       = id;
    s->Joueur1       = *j1;
    s->Joueur2       = *j2;
    s->nbSpectateurs = 0;
    s->nbCoups       = 0;

    return s;
}

void afficherSalon(Salon* s){
    if(s == NULL) return;

    printf("Salon ID : %d\n", s->idSalon);
    printf("Joueur 1 : %s\n", s->Joueur1.NomJoueur);
    printf("Joueur 2 : %s\n", s->Joueur2.NomJoueur);
    printf("Nombre de coups : %d\n", s->nbCoups);
    printf("Nombre de spectateurs : %d\n", s->nbSpectateurs);
}

void detruireSalon(Salon* s){
    if(s != NULL){
        free(s);
    }
}