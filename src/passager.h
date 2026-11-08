/*
 * passager.h - module 1 : gestion des passagers (couche metier)
 *
 * Conventions de retour :
 *   1  succes
 *   0  refus metier (doublon, introuvable, deja bloque...)
 *  -1  erreur d'acces au fichier
 *
 * Aucun printf ni GtkWidget dans passager.c : les messages sont
 * construits dans passager_ui.c.
 *
 * Format d'une ligne de data/passagers.txt (13 champs separes par ';') :
 * cin;nom;prenom;JJ/MM/AAAA;sexe;nationalite;tel;email;newsletter;points;bloque;motif;JJ/MM/AAAA
 */
#ifndef PASSAGER_H
#define PASSAGER_H

#include <stdio.h>
#include "utils.h"

#define MAX_PASSAGERS 1000

typedef struct {
    char cin[9];          /* 8 chiffres + '\0' */
    char nom[61];         /* 30 caracteres UTF-8 = jusqu'a 60 octets */
    char prenom[61];
    Date naissance;
    char sexe;            /* 'H' ou 'F' */
    char nationalite[41];
    char tel[9];
    char email[51];
    int  newsletter;      /* 0 / 1 */
    int  points;          /* cumul fidelite */
    int  bloque;          /* 0 actif, 1 bloque */
    char motif[201];      /* "-" si aucun */
    Date date_blocage;    /* 00/00/0000 si aucune */
} Passager;

typedef enum { STANDARD, SILVER, GOLD, PLATINUM } Niveau;

/* ---- Format du fichier (les 2 seules fonctions qui connaissent le format) ---- */
void passager_ecrire_ligne(FILE *f, const Passager *p);
int  passager_lire_ligne(FILE *f, Passager *p);       /* 1 si une ligne complete a ete lue */
void passager_init(Passager *p);                       /* struct a zero + motif "-" */

/* ---- CRUD (usage interne au module 1) ---- */
int  passager_ajouter(const Passager *p);             /* 1 ok, 0 doublon, -1 fichier */
int  passager_modifier(const Passager *p);            /* 1 ok, 0 introuvable, -1 fichier */
int  passager_supprimer(const char *cin);              /* 1 ok, 0 introuvable, -1 fichier */
int  passager_chercher(const char *cin, Passager *out);/* 1 trouve, 0 non */
int  passager_charger_tous(Passager t[], int max);     /* renvoie le nombre lu */
void passager_trier_nom(Passager t[], int n, int croissant);
int  passager_filtrer(const Passager t[], int n, const char *mot, Passager r[]);

/* ---- Fidelite ---- */
Niveau      passager_niveau(int points);
const char *niveau_nom(Niveau n);
float       niveau_remise(Niveau n);                   /* 0.00 .. 0.15 */
int         niveau_seuil_suivant(int points);          /* points du niveau suivant, -1 si Platinum */
float       passager_taux_remise(const char *cin);
int         passager_crediter_points(const char *cin, float prix_dt);
int         passager_debiter_points(const char *cin, float prix_dt);

/* ---- Blocage (option 2) ---- */
int  passager_est_bloque(const char *cin);
int  passager_bloquer(const char *cin, const char *motif);  /* 0 si deja bloque ou introuvable */
int  passager_reactiver(const char *cin);                   /* 0 si deja actif ou introuvable */

/* ---- Pour les autres modules ---- */
int  passager_existe(const char *cin);

#endif
