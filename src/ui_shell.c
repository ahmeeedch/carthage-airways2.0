/*
 * ui_shell.c — coquille commune : menu ☰ animé, sélection de page, raccourcis clavier
 */
#include <math.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "ui_shell.h"
#include "ui_theme.h"

static GtkWidget *w_(Shell *s, const char *prefixe, const char *suffixe)
{
    gchar *nom = g_strdup_printf("%s_%s", prefixe, suffixe);
    GtkWidget *w = ui_w(s->fenetre, nom);
    g_free(nom);
    return w;
}

Shell *shell_attacher(GtkWidget *fenetre, const char *P, const char *codes[], const char *soustitres[],
                      int n, const char *nom_affiche, const char *initiales)
{
    Shell *s = g_new0(Shell, 1);
    GtkWidget *avatar;
    int i;

    s->fenetre = fenetre;
    s->n = MIN(n, SHELL_MAX);
    s->largeur = SB_LARGE;
    s->sidebar = w_(s, P, "sidebar_menu");
    s->pied = w_(s, P, "sur_navigation");
    s->titre = w_(s, P, "titre_page");
    s->sous_titre = w_(s, P, "t2_soustitre");
    s->pages = w_(s, P, "notebook_pages");
    s->toast.boite = w_(s, P, "toast_message");
    s->toast.label = w_(s, P, "toasttxt_message");
    for (i = 0; i < s->n; i++) {
        gchar *nav = g_strdup_printf("nav_%s", codes[i]), *txt = g_strdup_printf("navtxt_%s", codes[i]), *tip;
        s->items[i] = w_(s, P, nav);
        s->noms[i] = w_(s, P, txt);
        g_strlcpy(s->titres[i], gtk_label_get_text(GTK_LABEL(s->noms[i])), sizeof s->titres[i]);
        s->soustitres[i] = soustitres[i];
        g_object_set_data(G_OBJECT(s->items[i]), "index", GINT_TO_POINTER(i));
        tip = g_strdup_printf("%s (Ctrl+%d)", s->titres[i], i + 1);
        gtk_widget_set_tooltip_text(s->items[i], tip);
        g_free(tip);
        g_free(nav);
        g_free(txt);
    }
    gtk_widget_set_tooltip_text(w_(s, P, "icone_burger"), "Replier / déplier le menu (Ctrl+B)");

    gtk_label_set_text(GTK_LABEL(w_(s, P, "val_nom")), nom_affiche);
    g_strlcpy(s->initiales, initiales, sizeof s->initiales);
    avatar = w_(s, P, "avatar_initiales");
    g_object_set_data(G_OBJECT(avatar), "initiales", s->initiales);
    g_object_set_data(G_OBJECT(fenetre), "shell", s);
    return s;
}

Shell *shell_de(GtkWidget *w)
{
    return g_object_get_data(G_OBJECT(gtk_widget_get_toplevel(w)), "shell");
}

void shell_selectionner(Shell *s, int i, gboolean anime)
{
    if (i < 0 || i >= s->n) return;
    gtk_widget_set_state(s->items[s->actif], GTK_STATE_NORMAL);
    s->actif = i;
    gtk_widget_set_state(s->items[i], GTK_STATE_SELECTED);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(s->pages), i);
    if (anime) anim_titre(s->titre, s->titres[i]);
    else gtk_label_set_text(GTK_LABEL(s->titre), s->titres[i]);
    gtk_label_set_text(GTK_LABEL(s->sous_titre), s->soustitres[i]);
    if (s->montree) s->montree(s, i);
}

void shell_montrer(Shell *s)
{
    gtk_widget_show(s->fenetre);
    s->actif = 0;
    shell_selectionner(s, 0, TRUE);
    gtk_window_set_focus(GTK_WINDOW(s->fenetre), NULL);     /* pas d'anneau de focus au démarrage */
}

/* --- menu ☰ : la largeur glisse de 248 à 76 px en 14 images --- */

static gboolean menu_tick(gpointer data)
{
    Shell *s = data;
    double t = 1 - pow(1 - ++s->etape / 14.0, 3);
    int i;
    s->largeur = s->a0 + (int) ((s->a1 - s->a0) * t);
    gtk_widget_set_size_request(s->sidebar, s->largeur, -1);
    if (s->etape >= 14) {
        s->anim = 0;
        if (!s->replie) {
            for (i = 0; i < s->n; i++) gtk_widget_show(s->noms[i]);
            gtk_widget_show(s->pied);
        }
        return FALSE;
    }
    return TRUE;
}

void shell_menu_basculer(Shell *s)
{
    int i;
    if (s->anim) g_source_remove(s->anim);
    s->replie = !s->replie;
    if (s->replie) {                      /* on cache les noms AVANT de rétrécir */
        for (i = 0; i < s->n; i++) gtk_widget_hide(s->noms[i]);
        gtk_widget_hide(s->pied);
    }
    s->a0 = s->largeur;
    s->a1 = s->replie ? SB_ETROIT : SB_LARGE;
    s->etape = 0;
    s->anim = g_timeout_add(16, menu_tick, s);
}

gboolean shell_touche(Shell *s, GdkEventKey *e)
{
    if (s == NULL || !(e->state & GDK_CONTROL_MASK)) return FALSE;
    if (e->keyval == GDK_b || e->keyval == GDK_B) { shell_menu_basculer(s); return TRUE; }
    if (e->keyval == GDK_t || e->keyval == GDK_T) { theme_basculer(); return TRUE; }
    if (e->keyval >= GDK_1 && e->keyval < GDK_1 + (guint) s->n) {
        shell_selectionner(s, (int) (e->keyval - GDK_1), TRUE);
        return TRUE;
    }
    return FALSE;
}

void shell_liberer(Shell *s)
{
    if (s == NULL) return;
    if (s->anim) g_source_remove(s->anim);
    anim_toast_arreter(&s->toast);
    g_object_set_data(G_OBJECT(s->fenetre), "shell", NULL);
    g_free(s);
}
