#include "chess.h"
#include "players.h"
#include <stdlib.h>
#include <string.h>

#ifndef SALON_H
#define SALON_H


typedef struct {
    int idSalon;

    joueur Joueur1;
    joueur Joueur2;

    int nbSpectateurs;
    int nbCoups;

    int ListeCoups[51];
    joueur ListeSpectateurs[20];

} Salon;

Salon* creeSalon(int id, joueur* j1, joueur* j2);
void afficherSalon(Salon* s);
void ajouterCoup(Salon* s, int coup);
void detruireSalon(Salon* s);

#endif
