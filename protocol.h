#ifndef PROTOCOL_H
#define PROTOCOL_H

/*
  protocol.h — Structure de message partagée entre client et serveur.
 
  Tous les échanges utilisent une seule struct Message.
  Le champ `type` indique la nature du message, `data` et `nb` portent
  le contenu variable.

    CLIENT → SERVEUR                                               
    MSG_DEMANDE_COUPS  data[0]=src (label affiché XY)              
    MSG_JOUER_COUP     data[0]=src, data[1]=dst (labels affichés)  
                                                                   
    SERVEUR → CLIENT                                               
    MSG_REPONSE_COUPS  data[0..nb-1] = destinations (labels)       
                       nb = nombre de coups disponibles            
    MSG_ETAT_PLATEAU   data[0..63]   = board aplati ligne par ligne
                       data[64]      = pièces élim. blanches (nb)  
                       data[65]      = pièces élim. noires  (nb)   
                       data[66..nb-1]= chars des pièces éliminées 
MSG_COUP_OK        (aucune donnée)                              
MSG_COUP_INVALIDE  (aucune donnée)                              
MSG_ECHEC_BLANC    roi blanc en échec                          
MSG_ECHEC_NOIR     roi noir en échec                           
MSG_VOTRE_TOUR     data[0] = player_id (1=blanc, 2=noir)       
 
*/

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

typedef struct {
  int sock_1;
  int sock_2;
} Args_gerepartie; // permet de stocker les arguments nécessaires pour utiliser gerepartie


#endif 