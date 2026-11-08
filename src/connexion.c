/*
 * connexion.c — écran de connexion
 *
 * Le panneau bordeaux est un GtkFixed dessiné dans Glade :
 *   cnx_img_fond   GtkImage  fond_connexion_jour.png / _nuit.png (dégradé, cercles, trajectoire pâle)
 *   cnx_img_avion  GtkImage  un des 22 PNG avion_00..21.png (l'avion tourné de 5° en 5°)
 *   cnx_or_marque, cnx_blanc_bienvenue, cnx_blancp_slogan : GtkLabel posés sur l'image
 *
 * Animation « décollage » : à chaque image (16 ms), l'avion est déplacé le long de la
 * courbe avec gtk_fixed_move() et on choisit le PNG dont l'angle suit la trajectoire.
 * La partie déjà parcourue est repassée en doré vif par-dessus le fond (cairo).
 */
#include <math.h>
#include <string.h>
#include "connexion.h"
#include "interface.h"
#include "support.h"
#include "ui_theme.h"
#include "ui_anim.h"
#include "auth.h"
#include "session.h"
#include "passager.h"
#include "passager_admin.h"
#include "passager_espace.h"

#define NB_AVIONS 22          /* avion_00.png = -70°, avion_21.png = +35° */

static GtkWidget *fen = NULL, *fen_bloque = NULL;
static GdkPixbuf *avions[NB_AVIONS], *fond_jour = NULL, *fond_nuit = NULL;
static double avion_t = 0;
static guint avion_id = 0;
static int avion_image = -1;
static gboolean en_cours = FALSE;

static GtkWidget *W(const char *nom) { return ui_w(fen, nom); }

/* même courbe que outils/generer_png.c (Bézier quadratique sur un panneau W x H) */
static void point_courbe(double t, double *x, double *y, double *angle)
{
    const double Wp = 380, Hp = 540;
    double x0 = 34, y0 = Hp - 150, x1 = Wp * 0.50, y1 = Hp * 0.12, x2 = Wp - 44, y2 = 150, u = 1 - t;
    *x = u * u * x0 + 2 * u * t * x1 + t * t * x2;
    *y = u * u * y0 + 2 * u * t * y1 + t * t * y2;
    *angle = atan2(2 * u * (y1 - y0) + 2 * t * (y2 - y1), 2 * u * (x1 - x0) + 2 * t * (x2 - x1));
}

static double avion_progression(void)    /* accélère puis ralentit (ease-in-out) */
{
    return avion_t < 0.5 ? 4 * avion_t * avion_t * avion_t : 1 - pow(-2 * avion_t + 2, 3) / 2;
}

/* trajectoire déjà parcourue, dessinée APRÈS le PNG de fond (connect_after) */
static gboolean trace_expose(GtkWidget *w, GdkEventExpose *e, gpointer d)
{
    static const double tirets[] = {2.0, 7.0};
    GtkAllocation a;
    cairo_t *cr = gdk_cairo_create(e->window);
    double x, y, ang, t, fin = avion_progression();
    gtk_widget_get_allocation(w, &a);
    gdk_cairo_region(cr, e->region);
    cairo_clip(cr);
    cairo_translate(cr, a.x, a.y);
    cairo_set_line_width(cr, 1.6);
    cairo_set_dash(cr, tirets, 2, 0);
    for (t = 0; t <= fin + 0.0001; t += 0.02) {
        point_courbe(t, &x, &y, &ang);
        if (t == 0) cairo_move_to(cr, x, y); else cairo_line_to(cr, x, y);
    }
    cairo_set_source_rgba(cr, 0.92, 0.80, 0.55, 0.85);
    cairo_stroke(cr);
    cairo_destroy(cr);
    return FALSE;
}

static void avion_placer(void)
{
    double x, y, ang;
    int k;
    point_courbe(avion_progression(), &x, &y, &ang);
    k = (int) lround((ang * 180.0 / M_PI + 70.0) / 5.0);
    k = CLAMP(k, 0, NB_AVIONS - 1);
    if (k != avion_image && avions[k] != NULL) {
        gtk_image_set_from_pixbuf(GTK_IMAGE(W("cnx_img_avion")), avions[k]);
        avion_image = k;
    }
    gtk_fixed_move(GTK_FIXED(W("cnx_fixed_panneau")), W("cnx_img_avion"), (int) x - 22, (int) y - 22);
    gtk_widget_queue_draw(W("cnx_img_fond"));
}

static gboolean avion_tick(gpointer d)
{
    avion_t += 1.0 / 130;                       /* ≈ 2 secondes de vol */
    if (avion_t >= 1) { avion_t = 1; avion_id = 0; avion_placer(); return FALSE; }
    avion_placer();
    return TRUE;
}

static void avion_decoller(void)
{
    avion_t = 0;
    avion_placer();
    if (avion_id == 0) avion_id = g_timeout_add(16, avion_tick, NULL);
}

static void theme_change(void)                  /* le panneau a sa version Nuit */
{
    GdkPixbuf *p = theme_est_nuit() ? fond_nuit : fond_jour;
    if (fen != NULL && p != NULL) gtk_image_set_from_pixbuf(GTK_IMAGE(W("cnx_img_fond")), p);
}

void connexion_ouvrir(void)
{
    int i;
    char nom[32];

    fen = create_window_connexion();
    ui_habiller(fen);

    for (i = 0; i < NB_AVIONS; i++) {
        g_snprintf(nom, sizeof nom, "avion_%02d.png", i);
        avions[i] = create_pixbuf(nom);
    }
    fond_jour = create_pixbuf("fond_connexion_jour.png");
    fond_nuit = create_pixbuf("fond_connexion_nuit.png");
    theme_change();
    theme_ecouter(theme_change);
    g_signal_connect_after(W("cnx_img_fond"), "expose-event", G_CALLBACK(trace_expose), NULL);

    ui_indice(W("cnx_entry_login"), "admin ou votre CIN");
    gtk_widget_show(fen);
    gtk_widget_grab_focus(W("cnx_entry_login"));
    anim_titre(W("cnx_titre_connexion"), "Connexion");
    avion_decoller();
}

void connexion_revenir(void)
{
    if (fen == NULL) return;
    gtk_entry_set_text(GTK_ENTRY(W("cnx_entry_mdp")), "");
    gtk_label_set_text(GTK_LABEL(W("cnx_err_message")), "");
    gtk_widget_show(fen);
    gtk_window_present(GTK_WINDOW(fen));
    gtk_widget_grab_focus(W("cnx_entry_login"));
    anim_titre(W("cnx_titre_connexion"), "Connexion");
    avion_decoller();
}

void connexion_focus_mdp(void) { gtk_widget_grab_focus(W("cnx_entry_mdp")); }

static void erreur(const char *message, const char *champ)
{
    gtk_label_set_text(GTK_LABEL(W("cnx_err_message")), message);
    ui_marquer_erreur(W(champ));
    anim_secouer(fen);
}

/* ouverture différée de 350 ms : on voit le bouton s'enfoncer avant le changement de fenêtre */
static gboolean ouvrir_espace(gpointer data)
{
    Passager *p = data;
    if (p == NULL) admin_ouvrir();
    else { espace_ouvrir(p); g_free(p); }
    gtk_widget_hide(fen);
    en_cours = FALSE;
    return FALSE;
}

/* ---------- fenêtre Compte suspendu ---------- */

static void bloque_afficher(const Passager *p)
{
    char date[16];
    fen_bloque = create_window_compte_bloque();
    ui_habiller(fen_bloque);
    gtk_window_set_transient_for(GTK_WINDOW(fen_bloque), GTK_WINDOW(fen));
    gtk_label_set_text(GTK_LABEL(ui_w(fen_bloque, "blq_val_motif")), p->motif);
    g_snprintf(date, sizeof date, "%02d/%02d/%04d", p->date_blocage.jour, p->date_blocage.mois, p->date_blocage.annee);
    gtk_label_set_text(GTK_LABEL(ui_w(fen_bloque, "blq_val_date")), date);
    gtk_widget_show(fen_bloque);
    gtk_window_set_focus(GTK_WINDOW(fen_bloque), NULL);
    anim_titre(ui_w(fen_bloque, "blq_titreerr_titre"), "Compte suspendu");
}

void bloque_fermer(void)
{
    if (fen_bloque != NULL) gtk_widget_destroy(fen_bloque);
    fen_bloque = NULL;
    session_fermer();
    connexion_revenir();
}

/* ---------- validation ---------- */

void connexion_valider(void)
{
    char login[32], mdp[32], role[12] = "";
    Passager p;
    int r;

    if (en_cours) return;
    g_strlcpy(login, gtk_entry_get_text(GTK_ENTRY(W("cnx_entry_login"))), sizeof login);
    g_strlcpy(mdp, gtk_entry_get_text(GTK_ENTRY(W("cnx_entry_mdp"))), sizeof mdp);
    nettoyer(login);
    gtk_entry_set_text(GTK_ENTRY(W("cnx_entry_mdp")), "");

    if (login[0] == '\0' || mdp[0] == '\0') {
        erreur("Saisissez votre identifiant et votre mot de passe.", login[0] ? "cnx_entry_mdp" : "cnx_entry_login");
        return;
    }
    r = auth_verifier(login, mdp, role);
    if (r < 0) { erreur("Fichier data/users.txt introuvable : lancez ./reset_data.sh", "cnx_entry_login"); return; }
    if (r == 0) { erreur("Identifiant ou mot de passe incorrect.", "cnx_entry_mdp"); return; }

    gtk_label_set_text(GTK_LABEL(W("cnx_err_message")), "");
    if (strcmp(role, "ADMIN") == 0) {
        session_ouvrir(login, role);
        en_cours = TRUE;
        g_timeout_add(350, ouvrir_espace, NULL);
    } else if (strcmp(role, "PASSAGER") == 0) {
        if (!passager_chercher(login, &p)) { erreur("Compte sans fiche passager : contactez l'administrateur.", "cnx_entry_login"); return; }
        session_ouvrir(login, role);
        if (p.bloque) { bloque_afficher(&p); return; }
        en_cours = TRUE;
        {
            Passager *copie = g_new(Passager, 1);
            *copie = p;
            g_timeout_add(350, ouvrir_espace, copie);
        }
    } else {
        erreur("Espace personnel navigant : module 4, pas encore intégré.", "cnx_entry_login");
    }
}
