/*
 * ui_theme.h — thème Jour / Nuit et « habillage » des widgets créés par Glade
 *
 * PRINCIPE
 *   Les fenêtres sont dessinées dans Glade avec des widgets ordinaires.
 *   Le RÔLE visuel d'un widget est le 2e mot de son nom Glade :
 *
 *       pas_btnp_enregistrer   →  rôle « btnp »  →  bouton principal (pilule bordeaux)
 *       pas_champ_nom          →  rôle « champ » →  boîte de saisie arrondie
 *       pas_carte_identite     →  rôle « carte » →  carte blanche avec ombre
 *
 *   ui_habiller(fenetre) parcourt la fenêtre une seule fois, juste après
 *   create_window_xxx(), et branche le bon dessin sur chaque widget.
 *   La liste complète des rôles est dans ui_theme.c (tableau ROLES).
 */
#ifndef UI_THEME_H
#define UI_THEME_H

#include <gtk/gtk.h>

enum {
    C_FOND, C_SURFACE, C_SURFACE2, C_CHAMP, C_LIGNE, C_TEXTE, C_TEXTE2, C_TEXTE3, C_TITRE, C_MARQUE,
    C_ACCENT, C_ACCENT_H, C_ACCENT_A, C_SUR_ACCENT, C_SELECTION, C_OR, C_SUCCES, C_SUCCES_F,
    C_ERREUR, C_ERREUR_F, C_SUR_ERREUR, C_ALT, C_N
};

/* ---- thème ---- */
void        theme_init(gboolean nuit);
void        theme_basculer(void);                    /* Jour <-> Nuit, sur toutes les fenêtres */
gboolean    theme_est_nuit(void);
const char *theme_couleur(int c);                    /* "#RRGGBB" de la palette en cours */
const char *theme_couleur_niveau(int niveau);        /* couleur du texte Standard..Platinum */
void        theme_ecouter(void (*rappel)(void));     /* appelé après chaque bascule */
void        theme_oublier(void (*rappel)(void));

/* ---- dessin cairo (utilisé aussi par les modules) ---- */
void ui_rgb(int c, double *r, double *g, double *b);
void ui_source(cairo_t *cr, int c, double alpha);
void ui_arrondi(cairo_t *cr, double x, double y, double w, double h, double r);
void ui_ombre(cairo_t *cr, double x, double y, double w, double h, double r, int force);
void ui_texte(cairo_t *cr, const char *police, const char *markup, double x, double y, int aligne); /* 0 g, 1 d, 2 centre */

/* ---- habillage des fenêtres Glade ---- */
void       ui_habiller(GtkWidget *fenetre);
GtkWidget *ui_w(GtkWidget *fenetre, const char *nom_glade);   /* = lookup_widget */
void       ui_renommer(GtkWidget *w, const char *nom_style);  /* change le style (ex. "ca-chip-ok") */
void       ui_surtitre(GtkWidget *label, const char *texte);  /* texte en petites capitales espacées */
void       ui_bouton_actif(GtkWidget *b, gboolean actif);     /* grise / dégrise un bouton */
gboolean   ui_bouton_est_off(GtkWidget *b);
void       ui_bouton_style(GtkWidget *b, const char *style);  /* bouton créé en C (ex. boîte de dialogue) */
void       ui_indice(GtkWidget *entry, const char *texte);    /* texte gris tant que le champ est vide */
void       ui_marquer_erreur(GtkWidget *entry);               /* bord rouge + focus */
void       ui_colonne(GtkTreeView *tv, const char *titre, int col, int col_couleur, gfloat xalign, gboolean gras);
void       ui_milliers(int v, char *out, size_t taille);      /* 1720 -> "1 720" */

#endif
