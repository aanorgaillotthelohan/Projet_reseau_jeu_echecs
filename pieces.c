#include "chess.h"

/* Crée un nœud pour une pièce éliminée */
eliminated_piece *create_piece(char value)
{
    eliminated_piece *p = (eliminated_piece *)malloc(sizeof(eliminated_piece));
    if (p != NULL) {
        p->piece_type = value;
        p->next = NULL;
    }
    return p;
}

/* Insère une pièce éliminée en fin de liste */
void insert_eliminated_list(char value, eliminated_pieces_list *l)
{
    eliminated_piece *piece = create_piece(value);
    if (l->start == NULL) {
        l->start = piece;
    } else {
        eliminated_piece *temp = l->start;
        while (temp->next != NULL) { temp = temp->next; }
        temp->next = piece;
    }
}

/* Affiche toutes les pièces éliminées d'une liste */
void display_eliminated_list(eliminated_pieces_list l)
{
    eliminated_piece *temp = l.start;
    while (temp != NULL) {
        wprintf(L"%lc ", (wchar_t)display_convert(temp->piece_type));
        temp = temp->next;
    }
    wprintf(L"\n");
}
