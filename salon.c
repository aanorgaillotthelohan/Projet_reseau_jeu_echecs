#include "salon.h"
#include <stdlib.h>

Salon* creeSalon(int id, joueur* j1, joueur* j2){

    Salon* s = malloc(sizeof(Salon));

    if(s == NULL)
        return NULL;

    s->idSalon = id;
    s->Joueur1 = *j1;
    s->Joueur2 = *j2;

    s->nbSpectateurs = 0;
    s->nbCoups = 0;

    s->ListeCoups[51];
    s->ListeSpectateurs[20];

    return s;
}