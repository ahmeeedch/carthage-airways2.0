/*
 * ui_anim.c — animations : titre, compteur, jauge, secousse, toast
 * Règle commune : on garde une référence (g_object_ref) sur le widget animé
 * pendant l'animation, pour qu'une fenêtre fermée en cours de route ne plante pas.
 */
#include <math.h>
#include <string.h>
#include "ui_anim.h"
#include "ui_theme.h"

static double adoucir(double t) { return 1 - pow(1 - t, 3); }    /* démarre vite, finit en douceur */

/* --- titre qui apparaît --- */

typedef struct { GtkWidget *l; gchar *txt; int etape; guint id; } Titre;

static void titre_liberer(Titre *t)
{
    g_object_set_data(G_OBJECT(t->l), "titre-anim", NULL);
    g_object_unref(t->l);
    g_free(t->txt);
    g_free(t);
}

static gboolean titre_tick(gpointer data)
{
    Titre *t = data;
    double k = adoucir(++t->etape / 22.0), r1, g1, b1, r2, g2, b2;
    gchar *esc, *m;
    if (t->etape >= 22) {
        gtk_label_set_text(GTK_LABEL(t->l), t->txt);   /* texte final : couleur du gtkrc */
        titre_liberer(t);
        return FALSE;
    }
    ui_rgb(C_FOND, &r1, &g1, &b1);
    ui_rgb(C_TITRE, &r2, &g2, &b2);
    esc = g_markup_escape_text(t->txt, -1);
    m = g_strdup_printf("<span foreground='#%02X%02X%02X' letter_spacing='%d'>%s</span>",
                        (int) ((r1 + (r2 - r1) * k) * 255), (int) ((g1 + (g2 - g1) * k) * 255),
                        (int) ((b1 + (b2 - b1) * k) * 255), (int) ((1 - k) * 5000), esc);
    gtk_label_set_markup(GTK_LABEL(t->l), m);
    g_free(m);
    g_free(esc);
    return TRUE;
}

void anim_titre(GtkWidget *l, const char *txt)
{
    Titre *ancien = g_object_get_data(G_OBJECT(l), "titre-anim"), *t;
    if (ancien) { g_source_remove(ancien->id); titre_liberer(ancien); }
    t = g_new0(Titre, 1);
    t->l = g_object_ref(l);
    t->txt = g_strdup(txt);
    t->id = g_timeout_add(16, titre_tick, t);
    g_object_set_data(G_OBJECT(l), "titre-anim", t);
}

/* --- compteur --- */

typedef struct { GtkWidget *label; int cible, etape; char suffixe[16]; } Compteur;

static gboolean compteur_tick(gpointer data)
{
    Compteur *c = data;
    char nb[32], txt[64];
    ui_milliers((int) lround(c->cible * adoucir(++c->etape / 45.0)), nb, sizeof nb);
    g_snprintf(txt, sizeof txt, "%s%s", nb, c->suffixe);
    gtk_label_set_text(GTK_LABEL(c->label), txt);
    if (c->etape >= 45) { g_object_unref(c->label); g_free(c); return FALSE; }
    return TRUE;
}

void anim_compteur(GtkWidget *label, int cible, const char *suffixe)
{
    Compteur *c = g_new0(Compteur, 1);
    c->label = g_object_ref(label);
    c->cible = cible;
    g_strlcpy(c->suffixe, suffixe, sizeof c->suffixe);
    g_timeout_add(16, compteur_tick, c);
}

/* --- jauge qui se remplit (départs décalés : effet « cascade ») --- */

typedef struct { GtkWidget *pb; double cible; int etape, attente; } Barre;

static gboolean barre_tick(gpointer data)
{
    Barre *b = data;
    if (b->attente > 0) { b->attente--; return TRUE; }
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(b->pb), b->cible * adoucir(++b->etape / 45.0));
    gtk_widget_queue_draw(b->pb);
    if (b->etape >= 45) { g_object_unref(b->pb); g_free(b); return FALSE; }
    return TRUE;
}

void anim_barre(GtkWidget *pb, double cible, int delai_ms)
{
    Barre *b = g_new0(Barre, 1);
    b->pb = g_object_ref(pb);
    b->cible = CLAMP(cible, 0.0, 1.0);
    b->attente = delai_ms / 16;
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pb), 0.0);
    g_timeout_add(16, barre_tick, b);
}

/* --- secousse de fenêtre --- */

typedef struct { GtkWidget *w; int x, y, etape; } Secousse;

static gboolean secousse_tick(gpointer data)
{
    static const int dx[] = {12, -12, 9, -9, 5, -5, 2, -2, 0};
    Secousse *s = data;
    gtk_window_move(GTK_WINDOW(s->w), s->x + dx[s->etape], s->y);
    if (++s->etape >= (int) G_N_ELEMENTS(dx)) {
        g_object_set_data(G_OBJECT(s->w), "secousse", NULL);
        g_object_unref(s->w);
        g_free(s);
        return FALSE;
    }
    return TRUE;
}

void anim_secouer(GtkWidget *fenetre)
{
    Secousse *s;
    if (g_object_get_data(G_OBJECT(fenetre), "secousse")) return;
    s = g_new0(Secousse, 1);
    s->w = g_object_ref(fenetre);
    gtk_window_get_position(GTK_WINDOW(fenetre), &s->x, &s->y);
    g_object_set_data(G_OBJECT(fenetre), "secousse", s);
    g_timeout_add(28, secousse_tick, s);
}

/* --- toast --- */

static gboolean toast_cacher(gpointer data)
{
    Toast *t = data;
    t->id = 0;
    gtk_widget_hide(t->boite);
    return FALSE;
}

void anim_toast(Toast *t, gboolean ok, const char *txt)
{
    gchar *s = g_strdup_printf("%s   %s", ok ? "\xE2\x9C\x93" : "!", txt);
    if (t->id) g_source_remove(t->id);
    ui_renommer(t->boite, ok ? "ca-toast-ok" : "ca-toast-err");
    gtk_label_set_text(GTK_LABEL(t->label), s);
    g_free(s);
    gtk_widget_show(t->boite);
    t->id = g_timeout_add(3600, toast_cacher, t);
}

void anim_toast_arreter(Toast *t)
{
    if (t->id) g_source_remove(t->id);
    t->id = 0;
}
