/*
 * utils.h - dates et controles de saisie (aucun GTK ici)
 * Toutes les fonctions est_* renvoient 1 si valide, 0 sinon.
 */
#ifndef UTILS_H
#define UTILS_H

typedef struct {
    int jour, mois, annee;
} Date;

/* ---- Dates ---- */
int  est_bissextile(int annee);
int  jours_dans_mois(int mois, int annee);
int  est_date_valide(Date d);           /* date reelle, pas dans le futur, age <= 120 ans */
Date date_du_jour(void);
int  calculer_age(Date naissance);       /* age en annees revolues a la date du jour */

/* ---- Chaines ---- */
void nettoyer(char *s);                  /* retire les espaces au debut et a la fin */
int  contient_separateur(const char *s); /* 1 si la chaine contient ';' ou un retour ligne */

/* ---- Controles de saisie ---- */
int  est_cin_valide(const char *s);      /* exactement 8 chiffres */
int  est_nom_valide(const char *s);      /* lettres (accents compris), espaces, '-', '\'' */
int  est_tel_valide(const char *s);      /* 8 chiffres, commence par 2, 4, 5, 7 ou 9 */
int  est_email_valide(const char *s);    /* un seul @, un point apres, aucun espace */
int  est_texte_valide(const char *s, int max_octets); /* non vide, pas de ';', <= max */

#endif
