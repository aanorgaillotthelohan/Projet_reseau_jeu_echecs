#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdbool.h>

bool VerifIdentifiant(char *idJoueur);
bool VerifConnexion(char *idJoueur, char *Mdp);
int Inscription(char *idJoueur, char *Mdp);
char ChoixConnexionNonReconnu();

#endif
