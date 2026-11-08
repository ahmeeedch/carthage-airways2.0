#include <gtk/gtk.h>


gboolean
on_window_connexion_delete_event       (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_bascule_theme_button_press_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_cnx_entry_login_activate            (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_cnx_entry_mdp_activate              (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_cnx_btnp_connecter_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_cnx_btnf_quitter_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_window_admin_destroy                (GtkObject       *object,
                                        gpointer         user_data);

gboolean
on_window_admin_key_press_event        (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_icone_burger_button_press_event     (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_btnf_deconnexion_clicked            (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_nav_button_press_event              (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_pas_onglet_button_press_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_pas_entry_recherche_changed         (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_pas_btnp_nouveau_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_pas_btn_tri_desc_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_pas_btn_tri_asc_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_pas_table_liste_row_activated       (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data);

void
on_pas_btnp_enregistrer_clicked        (GtkButton       *button,
                                        gpointer         user_data);

void
on_pas_btnf_vider_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_pas_btnd_supprimer_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_pas_btn_reactiver_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_pas_table_bloques_cursor_changed    (GtkTreeView     *treeview,
                                        gpointer         user_data);

void
on_pas_btnd_bloquer_clicked            (GtkButton       *button,
                                        gpointer         user_data);

void
on_window_passager_destroy             (GtkObject       *object,
                                        gpointer         user_data);

gboolean
on_window_passager_key_press_event     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_pas_btnp_enregistrer_pr_clicked     (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_window_compte_bloque_delete_event   (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_blq_btnp_retour_clicked             (GtkButton       *button,
                                        gpointer         user_data);
