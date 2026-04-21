#ifndef SALON_H
#define SALON_H

#include "chess.h"
#include "players.h"
#include "moves.h"

typedef struct {
    char CoupJoueur[100];
} coup;

typedef struct {
    int    idSalon;
    joueur Joueur1;
    joueur Joueur2;
    joueur ListeSpectateurs[20];
    int    nbSpectateurs;
    coup   ListeCoups[51];
    int    nbCoups;
    int nbClients;
    joueur Clients[22];
} Salon;

Salon *creeSalon(int id, joueur *j1, joueur *j2);

#endif /* SALON_H */