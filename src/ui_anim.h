/*
 * ui_anim.h — petites animations réutilisables (toutes à base de g_timeout_add, 60 images/s)
 */
#ifndef UI_ANIM_H
#define UI_ANIM_H

#include <gtk/gtk.h>

void anim_titre(GtkWidget *label, const char *texte);           /* fondu + lettres qui se resserrent */
void anim_compteur(GtkWidget *label, int cible, const char *suffixe); /* 0 -> cible en 0,7 s */
void anim_barre(GtkWidget *progress, double cible, int delai_ms);    /* jauge qui se remplit */
void anim_secouer(GtkWidget *fenetre);                          /* « non » : la fenêtre tremble */

/* message en pastille qui s'efface seul (boîte = GtkEventBox « toast », label = son texte) */
typedef struct { GtkWidget *boite, *label; guint id; } Toast;
void anim_toast(Toast *t, gboolean ok, const char *texte);
void anim_toast_arreter(Toast *t);

#endif
