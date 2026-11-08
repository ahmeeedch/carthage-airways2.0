/*
 * passager_espace.c — espace passager : carte de fidélité animée, progression, profil
 *
 * La carte (pas_cartefid_dessin, un GtkDrawingArea de Glade) superpose :
 *   1. le PNG du niveau : carte_standard / silver / gold / platinum.png
 *   2. un reflet lumineux qui la traverse à l'ouverture (animation cairo)
 *   3. les textes du passager (niveau, points, nom, numéro)
 */
#include <string.h>
#include "passager_espace.h"
#include "interface.h"
#include "support.h"
#include "ui_theme.h"
#include "ui_anim.h"
#include "ui_shell.h"
#include "connexion.h"
#include "session.h"

static GtkWidget *fen = NULL;
static Shell *shell = NULL;
static Passager moi;
static GdkPixbuf *cartes[4];
static double reflet_t = -1;
static guint reflet_id = 0;

static const char *NATIONALITES[] = {"Tunisienne", "Algérienne", "Libyenne", "Marocaine", "Française", "Autre"};
static const int SEUILS[] = {0, 500, 1500, 3000};

static GtkWidget *W(const char *nom) { return ui_w(fen, nom); }

/* ---------------- carte de fidélité ---------------- */

static gboolean carte_expose(GtkWidget *w, GdkEventExpose *e, gpointer d)
{
    static const double TEXTE[4][3] = {{0.16, 0.13, 0.14}, {0.12, 0.13, 0.16}, {0.17, 0.11, 0.04}, {0.96, 0.91, 0.82}};
    const double W_ = 390, H = 236;
    Niveau nv = passager_niveau(moi.points);
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(w));
    cairo_pattern_t *pat;
    char buf[160], pts[24];
    gchar *maj, *esc;

    ui_source(cr, C_FOND, 1);
    cairo_paint(cr);
    if (cartes[nv] != NULL) {                              /* 1. fond PNG */
        gdk_cairo_set_source_pixbuf(cr, cartes[nv], 0, 0);
        cairo_paint(cr);
    }
    if (reflet_t >= 0) {                                   /* 2. reflet qui traverse la carte */
        double x = -W_ * 0.5 + reflet_t * W_ * 2.0;
        cairo_save(cr);
        ui_arrondi(cr, 5, 4, W_, H, 18);
        cairo_clip(cr);
        pat = cairo_pattern_create_linear(x, 0, x + W_ * 0.35, H * 0.6);
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 1, 1, 1, 0.0);
        cairo_pattern_add_color_stop_rgba(pat, 0.5, 1, 1, 1, 0.35);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 1, 1, 1, 0.0);
        cairo_set_source(cr, pat);
        cairo_paint(cr);
        cairo_pattern_destroy(pat);
        cairo_restore(cr);
    }
    cairo_set_source_rgb(cr, TEXTE[nv][0], TEXTE[nv][1], TEXTE[nv][2]);   /* 3. textes */
    maj = g_utf8_strup(niveau_nom(nv), -1);
    g_snprintf(buf, sizeof buf, "<span letter_spacing='2500'>%s</span>", maj);
    ui_texte(cr, "DejaVu Sans Bold 10", buf, W_ - 18, 28, 1);
    g_free(maj);
    ui_milliers(moi.points, pts, sizeof pts);
    g_snprintf(buf, sizeof buf, "%s <span size='small'>points</span>", pts);
    ui_texte(cr, "DejaVu Serif 24", buf, 28, H * 0.50, 0);
    g_snprintf(buf, sizeof buf, "%s %s", moi.prenom, moi.nom);
    maj = g_utf8_strup(buf, -1);
    esc = g_markup_escape_text(maj, -1);                   /* le nom peut contenir une apostrophe */
    g_snprintf(buf, sizeof buf, "<span letter_spacing='1500'>%s</span>", esc);
    g_free(esc);
    g_free(maj);
    ui_texte(cr, "DejaVu Sans Bold 10", buf, 28, H - 44, 0);
    g_snprintf(buf, sizeof buf, "<span letter_spacing='1500'>N\xC2\xB0 \xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2\xE2\x80\xA2 %s</span>",
               moi.cin + 4);
    ui_texte(cr, "DejaVu Sans Mono 9", buf, W_ - 18, H - 43, 1);
    cairo_destroy(cr);
    return TRUE;
}

static gboolean reflet_tick(gpointer d)
{
    reflet_t += 0.024;
    if (reflet_t > 1.0) { reflet_t = -1; reflet_id = 0; }
    gtk_widget_queue_draw(W("pas_cartefid_dessin"));
    return reflet_id != 0;
}

static void fidelite_remplir(void)
{
    Niveau nv = passager_niveau(moi.points);
    char nom[40];
    gchar *maj = g_utf8_strup(niveau_nom(nv), -1);
    int i;
    gtk_label_set_text(GTK_LABEL(W("pas_chiptxt_niv_fid")), maj);
    g_free(maj);
    gtk_label_set_text(GTK_LABEL(W("pas_t3_borne_bas")), niveau_nom(nv));
    gtk_label_set_text(GTK_LABEL(W("pas_t3_borne_haut")), nv < PLATINUM ? niveau_nom(nv + 1) : "");
    for (i = 0; i < 4; i++) {                              /* palier actuel mis en avant */
        g_snprintf(nom, sizeof nom, "pas_palier_%d", i);
        ui_renommer(W(nom), i == (int) nv ? "ca-palier-actif" : "ca-palier");
        g_snprintf(nom, sizeof nom, "pas_t2_palremise_%d", i);
        ui_renommer(W(nom), i == (int) nv ? "ca-accent-txt" : "ca-t2");
    }
}

static void fidelite_animer(void)
{
    Niveau nv = passager_niveau(moi.points);
    char txt[128], reste[16];
    anim_compteur(W("pas_chiffre_points"), moi.points, "");
    if (nv < PLATINUM) {
        double f = (double) (moi.points - SEUILS[nv]) / (SEUILS[nv + 1] - SEUILS[nv]);
        anim_barre(W("pas_jaugeor_progression"), f, 150);
        ui_milliers(SEUILS[nv + 1] - moi.points, reste, sizeof reste);
        g_snprintf(txt, sizeof txt, "Plus que %s points pour atteindre le niveau %s.", reste, niveau_nom(nv + 1));
        ui_surtitre(W("pas_sur_progression"), "PROGRESSION");
    } else {
        anim_barre(W("pas_jaugeor_progression"), 1.0, 150);
        g_snprintf(txt, sizeof txt, "Vous êtes au niveau le plus élevé du programme.");
        ui_surtitre(W("pas_sur_progression"), "NIVEAU MAXIMAL");
    }
    gtk_label_set_text(GTK_LABEL(W("pas_t2_reste")), txt);
    if (reflet_id == 0) { reflet_t = 0; reflet_id = g_timeout_add(16, reflet_tick, NULL); }
}

static void page_montree(Shell *s, int i) { if (i == 0) fidelite_animer(); }

/* ---------------- profil ---------------- */

static void profil_remplir(void)
{
    char date[16];
    size_t i;
    gtk_label_set_text(GTK_LABEL(W("pas_val_i_cin")), moi.cin);
    gtk_label_set_text(GTK_LABEL(W("pas_val_i_sexe")), moi.sexe == 'F' ? "Femme" : "Homme");
    gtk_label_set_text(GTK_LABEL(W("pas_val_i_nom")), moi.nom);
    gtk_label_set_text(GTK_LABEL(W("pas_val_i_prenom")), moi.prenom);
    g_snprintf(date, sizeof date, "%02d/%02d/%04d", moi.naissance.jour, moi.naissance.mois, moi.naissance.annee);
    gtk_label_set_text(GTK_LABEL(W("pas_val_i_naiss")), date);
    gtk_combo_box_set_active(GTK_COMBO_BOX(W("pas_combo_nat_pr")), 5);
    for (i = 0; i < G_N_ELEMENTS(NATIONALITES); i++)
        if (strcmp(NATIONALITES[i], moi.nationalite) == 0) gtk_combo_box_set_active(GTK_COMBO_BOX(W("pas_combo_nat_pr")), (int) i);
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_tel_pr")), moi.tel);
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_email_pr")), moi.email);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(W("pas_coche_news_pr")), moi.newsletter);
}

void espace_enregistrer_profil(void)
{
    Passager p;
    char tel[16], email[64];
    gchar *nat;
    g_strlcpy(tel, gtk_entry_get_text(GTK_ENTRY(W("pas_entry_tel_pr"))), sizeof tel);
    g_strlcpy(email, gtk_entry_get_text(GTK_ENTRY(W("pas_entry_email_pr"))), sizeof email);
    nettoyer(tel);
    nettoyer(email);
    if (!est_tel_valide(tel)) {
        ui_marquer_erreur(W("pas_entry_tel_pr"));
        anim_toast(&shell->toast, FALSE, "Téléphone : 8 chiffres commençant par 2, 4, 5, 7 ou 9");
        return;
    }
    if (!est_email_valide(email) || contient_separateur(email)) {
        ui_marquer_erreur(W("pas_entry_email_pr"));
        anim_toast(&shell->toast, FALSE, "Adresse e-mail invalide");
        return;
    }
    if (!passager_chercher(moi.cin, &p)) { anim_toast(&shell->toast, FALSE, "Fiche introuvable"); return; }
    nat = gtk_combo_box_get_active_text(GTK_COMBO_BOX(W("pas_combo_nat_pr")));
    g_strlcpy(p.nationalite, nat ? nat : "Autre", sizeof p.nationalite);
    g_free(nat);
    g_strlcpy(p.tel, tel, sizeof p.tel);
    g_strlcpy(p.email, email, sizeof p.email);
    p.newsletter = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(W("pas_coche_news_pr")));
    if (passager_modifier(&p) == 1) {
        moi = p;
        anim_toast(&shell->toast, TRUE, "Coordonnées enregistrées");
    } else {
        anim_toast(&shell->toast, FALSE, "Erreur d'écriture dans data/passagers.txt");
    }
}

/* ---------------- ouverture / fermeture ---------------- */

void espace_ouvrir(const Passager *p)
{
    static const char *codes[] = {"fi", "pr", "vo", "re"};
    static const char *sous[] = {
        "Votre solde, votre niveau et vos avantages",
        "Identité et coordonnées",
        "Recherche par ville et par date — module 2",
        "Consulter et modifier vos réservations — module 3",
    };
    static const char *fichiers[] = {"carte_standard.png", "carte_silver.png", "carte_gold.png", "carte_platinum.png"};
    char nom[128], ini[8];
    int i;

    moi = *p;
    for (i = 0; i < 4; i++) if (cartes[i] == NULL) cartes[i] = create_pixbuf(fichiers[i]);
    fen = create_window_passager();
    ui_habiller(fen);
    g_snprintf(nom, sizeof nom, "%s %s", moi.prenom, moi.nom);
    g_snprintf(ini, sizeof ini, "%c%c", moi.prenom[0], moi.nom[0]);
    shell = shell_attacher(fen, "esp", codes, sous, 4, nom, ini);
    g_snprintf(shell->titres[0], sizeof shell->titres[0], "Bonjour, %s", moi.prenom);   /* « Bonjour, Rania » */
    shell->montree = page_montree;

    g_signal_connect(W("pas_cartefid_dessin"), "expose-event", G_CALLBACK(carte_expose), NULL);
    fidelite_remplir();
    profil_remplir();
    shell_montrer(shell);
}

void espace_fermer(void)
{
    if (fen == NULL) return;
    if (reflet_id) g_source_remove(reflet_id);
    reflet_id = 0;
    reflet_t = -1;
    shell_liberer(shell);
    shell = NULL;
    fen = NULL;
    session_fermer();
    connexion_revenir();
}

gboolean espace_touche(GdkEventKey *e) { return shell_touche(shell, e); }
