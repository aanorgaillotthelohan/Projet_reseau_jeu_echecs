
// application.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "application.h"

// Vérifie si le pseudo est disponible
bool VerifIdentifiant(char *idJoueur){
    FILE *fp = fopen("HistoJoueur.txt", "r");
    char ligne[256];

    if (fp == NULL){
        printf("Erreur problème de fichier des données");
        return false; // fichier inexistant → pseudo disponible
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

int Inscription(char *idJoueur, char *Mdp){
    // Vérifie si identifiant déjà pris
    FILE *fp = fopen("HistoJoueur.txt", "r");
    if (fp != NULL){
        char line[256];
        while(fgets(line, sizeof(line), fp)){
            char existingId[128];
            sscanf(line, "%s", existingId);
            if(strcmp(existingId, idJoueur) == 0){
                fclose(fp);
                return 0; // échec
            }
        }
        fclose(fp);
    }

    // Ajoute le nouveau joueur
    fp = fopen("HistoJoueur.txt", "a");
    if(fp != NULL){
        fprintf(fp, "%s %s\n", idJoueur, Mdp);
        fclose(fp);
        printf("Nouvel utilisateur inscrit : %s\n", idJoueur);
        return 1; // succès
    }

    return 0; // échec si fichier inaccessible
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

#define LINE_MAX_LEN 512

bool GetStats(const char *idJoueur, int *parties, int *victoires,
              int *defaites, int *draws)
{
    FILE *fp = fopen("HistoJoueur.txt", "r");
    if (!fp) return false;

    char ligne[LINE_MAX_LEN];
    while (fgets(ligne, sizeof(ligne), fp)) {
        ligne[strcspn(ligne, "\n")] = '\0';
        if (strlen(ligne) == 0) continue;

        char pseudo[128], mdp[128];
        int p = 0, v = 0, d = 0, dr = 0;
        int n = sscanf(ligne, "%127s %127s %d %d %d %d",
                       pseudo, mdp, &p, &v, &d, &dr);
        if (n < 2) continue;

        if (strcmp(pseudo, idJoueur) == 0) {
            *parties   = (n >= 3) ? p  : 0;
            *victoires = (n >= 4) ? v  : 0;
            *defaites  = (n >= 5) ? d  : 0;
            *draws     = (n >= 6) ? dr : 0;
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}

void UpdateStats(const char *idJoueur, GameResult result)
{
    FILE *fp = fopen("HistoJoueur.txt", "r");
    if (!fp) return;

    char lignes[256][LINE_MAX_LEN];
    int nb_lignes = 0;
    while (nb_lignes < 256 && fgets(lignes[nb_lignes], LINE_MAX_LEN, fp)) {
        lignes[nb_lignes][strcspn(lignes[nb_lignes], "\n")] = '\0';
        if (strlen(lignes[nb_lignes]) > 0)
            nb_lignes++;
    }
    fclose(fp);

    for (int i = 0; i < nb_lignes; i++) {
        char pseudo[128], mdp[128];
        int p = 0, v = 0, d = 0, dr = 0;
        int n = sscanf(lignes[i], "%127s %127s %d %d %d %d",
                       pseudo, mdp, &p, &v, &d, &dr);
        if (n < 2) continue;

        if (strcmp(pseudo, idJoueur) == 0) {
            p++;
            if      (result == RESULT_WIN)  v++;
            else if (result == RESULT_LOSS) d++;
            else                            dr++;
            snprintf(lignes[i], LINE_MAX_LEN,
                     "%s %s %d %d %d %d", pseudo, mdp, p, v, d, dr);
            break;
        }
    }

    fp = fopen("HistoJoueur.txt", "w");
    if (!fp) return;
    for (int i = 0; i < nb_lignes; i++)
        if (strlen(lignes[i]) > 0)
            fprintf(fp, "%s\n", lignes[i]);
    fclose(fp);
}

void PrintStats(const char *idJoueur)
{
    int parties = 0, victoires = 0, defaites = 0, draws = 0;
    if (!GetStats(idJoueur, &parties, &victoires, &defaites, &draws)) {
        printf("Joueur '%s' introuvable.\n", idJoueur);
        return;
    }
    printf("\n+==============================+\n");
    printf("|  Historique de %-14s|\n", idJoueur);
    printf("+==============================+\n");
    printf("|  Parties jouees : %-10d|\n", parties);
    printf("|  Victoires      : %-10d|\n", victoires);
    printf("|  Defaites       : %-10d|\n", defaites);
    printf("|  Nuls           : %-10d|\n", draws);
    if (parties > 0)
        printf("|  Win rate       : %-9.0f%%|\n",
               (victoires * 100.0) / parties);
    printf("+==============================+\n\n");
}

