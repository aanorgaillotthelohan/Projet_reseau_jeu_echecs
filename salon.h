#include "chess.h"
#include "players.c"
#include "moves.c"
#include <stdlib.h>
#include <string.h>

typedef struct{
    int idSalon;
    joueur Joueur1;
    joueur Joueur2;

    joueur ListeSpectateurs[20];
    int nbSpectateurs;
    
    coup ListeCoups[51];
    int nbCoups;

}Salon;

