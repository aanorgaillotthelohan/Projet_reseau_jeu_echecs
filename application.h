#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdbool.h>

bool VerifIdentifiant(char *idJoueur);
bool VerifConnexion(char *idJoueur, char *Mdp);
int Inscription(char *idJoueur, char *Mdp);
char ChoixConnexionNonReconnu();
typedef enum { RESULT_WIN, RESULT_LOSS, RESULT_DRAW } GameResult;

bool GetStats(const char *idJoueur, int *parties, int *victoires,int *defaites, int *draws);
void UpdateStats(const char *idJoueur, GameResult result);
void PrintStats(const char *idJoueur);
#endif
