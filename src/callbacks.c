/*
 * callbacks.c — les fonctions appelées par les signaux déclarés dans Glade.
 *
 * Règle du projet : un callback ne contient (presque) pas de logique, il appelle
 * la fonction du module concerné. Quand tu ajoutes un signal dans Glade puis
 * « Construire », Glade ajoute le squelette vide à la FIN de ce fichier sans
 * toucher au reste : il suffit alors de le remplir.
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"

#include "ui_theme.h"
#include "ui_shell.h"
#include "connexion.h"
#include "passager_admin.h"
#include "passager_espace.h"

/* ===================== communs aux 3 fenêtres ===================== */

gboolean
on_bascule_theme_button_press_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  theme_basculer ();
  return TRUE;
}

gboolean
on_icone_burger_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  Shell *s = shell_de (widget);
  if (s) shell_menu_basculer (s);
  return TRUE;
}

gboolean
on_nav_button_press_event              (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  Shell *s = shell_de (widget);
  int i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "index"));
  if (s && i != s->actif) shell_selectionner (s, i, TRUE);
  return TRUE;
}

void
on_btnf_deconnexion_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  /* détruire la fenêtre déclenche on_window_xxx_destroy, qui revient à la connexion */
  gtk_widget_destroy (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

/* ===================== window_connexion ===================== */

gboolean
on_window_connexion_delete_event       (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  gtk_main_quit ();
  return FALSE;
}

void
on_cnx_entry_login_activate            (GtkEntry        *entry,
                                        gpointer         user_data)
{
  connexion_focus_mdp ();
}

void
on_cnx_entry_mdp_activate              (GtkEntry        *entry,
                                        gpointer         user_data)
{
  connexion_valider ();
}

void
on_cnx_btnp_connecter_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
  connexion_valider ();
}

void
on_cnx_btnf_quitter_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  gtk_main_quit ();
}

/* ===================== window_compte_bloque ===================== */

void
on_blq_btnp_retour_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  bloque_fermer ();
}

gboolean
on_window_compte_bloque_delete_event   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  bloque_fermer ();
  return TRUE;                 /* déjà détruite par bloque_fermer() */
}

/* ===================== window_admin ===================== */

void
on_window_admin_destroy                (GtkObject       *object,
                                        gpointer         user_data)
{
  admin_fermer ();
}

gboolean
on_window_admin_key_press_event        (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
  return admin_touche (event);
}

gboolean
on_pas_onglet_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
  admin_onglet (widget);
  return TRUE;
}

void
on_pas_entry_recherche_changed         (GtkEditable     *editable,
                                        gpointer         user_data)
{
  admin_recherche ();
}

void
on_pas_btn_tri_asc_clicked             (GtkButton       *button,
                                        gpointer         user_data)
{
  admin_trier (TRUE);
}

void
on_pas_btn_tri_desc_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  admin_trier (FALSE);
}

void
on_pas_btnp_nouveau_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  admin_nouveau ();
}

void
on_pas_table_liste_row_activated       (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data)
{
  admin_ligne_activee (path);
}

void
on_pas_btnp_enregistrer_clicked        (GtkButton       *button,
                                        gpointer         user_data)
{
  admin_enregistrer ();
}

void
on_pas_btnf_vider_clicked              (GtkButton       *button,
                                        gpointer         user_data)
{
  admin_vider ();
}

void
on_pas_btnd_supprimer_clicked          (GtkButton       *button,
                                        gpointer         user_data)
{
  admin_supprimer ();
}

void
on_pas_table_bloques_cursor_changed    (GtkTreeView     *treeview,
                                        gpointer         user_data)
{
  admin_bloques_selection ();
}

void
on_pas_btn_reactiver_clicked           (GtkButton       *button,
                                        gpointer         user_data)
{
  admin_reactiver ();
}

void
on_pas_btnd_bloquer_clicked            (GtkButton       *button,
                                        gpointer         user_data)
{
  admin_bloquer ();
}

/* ===================== window_passager ===================== */

void
on_window_passager_destroy             (GtkObject       *object,
                                        gpointer         user_data)
{
  espace_fermer ();
}

gboolean
on_window_passager_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data)
{
  return espace_touche (event);
}

void
on_pas_btnp_enregistrer_pr_clicked     (GtkButton       *button,
                                        gpointer         user_data)
{
  espace_enregistrer_profil ();
}
