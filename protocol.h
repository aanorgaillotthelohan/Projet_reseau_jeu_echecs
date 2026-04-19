#ifndef PROTOCOL_H
#define PROTOCOL_H


#define MSG_DEMANDE_COUPS   1
#define MSG_JOUER_COUP      2
#define MSG_REPONSE_COUPS   3
#define MSG_ETAT_PLATEAU    4
#define MSG_COUP_OK         5
#define MSG_COUP_INVALIDE   6
#define MSG_ECHEC_BLANC     7
#define MSG_ECHEC_NOIR      8
#define MSG_VOTRE_TOUR      9
#define MSG_BLANC 10
#define MSG_NOIR 11

#define MSG_DATA_MAX  200   /* taille max du tableau data */

typedef struct {
    int type;
    int nb;               /* nombre d'éléments valides dans data */
    int data[MSG_DATA_MAX];

} Message;

#endif 