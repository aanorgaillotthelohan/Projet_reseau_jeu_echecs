#ifndef PLAYERS_H
#define PLAYERS_H

#include <stdbool.h>

typedef struct {
    int socketJoueur;
    char NomJoueur[20];
    char Couleur[20];
    bool rejoinsSalon;
    char message[128];
} joueur;

joueur* creejoueur(int socket, char NomDuJoueur[20]);

#endif
