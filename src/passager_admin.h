/*
 * passager_admin.h — module 1 côté administrateur (fenêtre window_admin, page « Passagers »)
 * Chaque fonction est appelée depuis le callback Glade du même nom (callbacks.c).
 */
#ifndef PASSAGER_ADMIN_H
#define PASSAGER_ADMIN_H

#include <gtk/gtk.h>

void     admin_ouvrir(void);
void     admin_fermer(void);                       /* destroy de window_admin */
gboolean admin_touche(GdkEventKey *e);             /* Ctrl+B / Ctrl+T / Ctrl+1..6 / Alt+1..4 */

void admin_onglet(GtkWidget *onglet);              /* clic sur un des 4 onglets */
void admin_recherche(void);
void admin_trier(gboolean croissant);
void admin_nouveau(void);
void admin_ligne_activee(GtkTreePath *chemin);
void admin_enregistrer(void);
void admin_vider(void);
void admin_supprimer(void);
void admin_bloques_selection(void);
void admin_reactiver(void);
void admin_bloquer(void);

#endif
