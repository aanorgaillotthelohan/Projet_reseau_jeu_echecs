
// application.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Vérifie si le pseudo est disponible
bool VerifIdentifiant(char *idJoueur){
FILE *fp = fopen("HistoJoueur.txt", "r");
char ligne[256];

if (fp == NULL){
return true; // fichier inexistant → pseudo disponible
}

while(fgets(ligne, sizeof(ligne), fp) != NULL){
ligne[strcspn(ligne, "\n")] = '\0';
char pseudo[128];
sscanf(ligne, "%s", pseudo);
if(strcmp(pseudo, idJoueur) == 0){
fclose(fp);
return false;
}
}
fclose(fp);
return true;
}

// Vérifie identifiant + mot de passe
bool VerifConnexion(char *idJoueur, char *Mdp){
FILE *fp = fopen("HistoJoueur.txt", "r");
char ligne[256];

if (fp == NULL) return false;

while(fgets(ligne, sizeof(ligne), fp) != NULL){
ligne[strcspn(ligne, "\n")] = '\0';
char pseudo[128], motdepasse[128];
sscanf(ligne, "%s %s", pseudo, motdepasse);
if(strcmp(pseudo, idJoueur) == 0 && strcmp(motdepasse, Mdp) == 0){
fclose(fp);
return true;
}
}
fclose(fp);
return false;
}

// Inscription
void Inscription(char *idJoueur, char *Mdp){
FILE *fp = fopen("HistoJoueur.txt", "a+");
if (fp == NULL) return;

printf("Saisir son identifiant : ");
scanf(" %s", idJoueur);
while(!VerifIdentifiant(idJoueur)){
printf("Identifiant déjà pris\nSaisir un nouveau : ");
scanf(" %s", idJoueur);
}

printf("Saisir son mot de passe : ");
scanf(" %s", Mdp);

fprintf(fp, "%s %s\n", idJoueur, Mdp);
fclose(fp);
}

// Choix après identifiant non reconnu
char ChoixConnexionNonReconnu(){
char choix = '\0';
while(choix != 'r' && choix != 'i'){
printf("Identifiant ou mot de passe non reconnu.\n");
printf("Réessayer (r) ou s'inscrire (i) ? ");
scanf(" %c", &choix);
choix = tolower(choix);
}
return choix;
}

