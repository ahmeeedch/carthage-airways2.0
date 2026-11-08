/*
 * passager_admin.c — module 1 côté administrateur : liste, fiche, statistiques, comptes bloqués
 *
 * Tous les widgets viennent de Glade (fenêtre window_admin) ; on les récupère
 * par leur nom avec W("pas_...") = lookup_widget().
 * Toute la lecture / écriture de data/passagers.txt passe par passager.c.
 */
#include <math.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#include "passager_admin.h"
#include "interface.h"
#include "support.h"
#include "ui_theme.h"
#include "ui_anim.h"
#include "ui_shell.h"
#include "connexion.h"
#include "passager.h"
#include "session.h"
#include "auth.h"

static GtkWidget *fen = NULL;
static Shell *shell = NULL;
static Passager tous[MAX_PASSAGERS];
static gboolean tri_croissant = TRUE;

static GtkWidget *W(const char *nom) { return ui_w(fen, nom); }
static void toast(gboolean ok, const char *txt) { anim_toast(&shell->toast, ok, txt); }

static const char *NATIONALITES[] = {"Tunisienne", "Algérienne", "Libyenne", "Marocaine", "Française", "Autre"};

/* =====================================================================
 * Onglets « segmentés » : une pastille glisse sous l'onglet actif
 * ===================================================================== */

static const char *ONGLETS[] = {"pas_onglet_liste", "pas_onglet_fiche", "pas_onglet_stats", "pas_onglet_bloques"};

static struct {
    int actif, x0, w0, x1, w1, px, pw, etape;
    guint anim;
} ong;

static void stats_animer(void);

static void onglet_position(int i, int *x, int *w)
{
    GtkAllocation a, b;
    gtk_widget_get_allocation(W(ONGLETS[i]), &a);
    gtk_widget_get_allocation(W("pas_piste_onglets"), &b);
    *x = a.x - b.x;
    *w = a.width;
}

static gboolean piste_expose(GtkWidget *w, GdkEventExpose *e, gpointer d)
{
    GtkAllocation a;
    cairo_t *cr = gdk_cairo_create(e->window);
    gtk_widget_get_allocation(w, &a);
    ui_arrondi(cr, a.x, a.y, a.width, a.height, a.height / 2.0);
    ui_source(cr, C_SURFACE2, 1);
    cairo_fill(cr);
    if (ong.pw > 0) {
        double x = a.x + ong.px + 1, y = a.y + 4, ww = ong.pw - 2, h = a.height - 8;
        cairo_set_source_rgba(cr, 0.15, 0.05, 0.08, theme_est_nuit() ? 0.3 : 0.08);
        ui_arrondi(cr, x, y + 1.5, ww, h, h / 2);
        cairo_fill(cr);
        ui_arrondi(cr, x, y, ww, h, h / 2);
        ui_source(cr, theme_est_nuit() ? C_LIGNE : C_SURFACE, 1);
        cairo_fill(cr);
    }
    cairo_destroy(cr);
    return FALSE;
}

static gboolean pastille_tick(gpointer data)
{
    double t = 1 - pow(1 - ++ong.etape / 15.0, 3);
    ong.px = ong.x0 + (int) ((ong.x1 - ong.x0) * t);
    ong.pw = ong.w0 + (int) ((ong.w1 - ong.w0) * t);
    gtk_widget_queue_draw(W("pas_piste_onglets"));
    if (ong.etape >= 15) { ong.anim = 0; return FALSE; }
    return TRUE;
}

static void onglets_activer(int i)
{
    if (i < 0 || i > 3) return;
    gtk_widget_set_state(W(ONGLETS[ong.actif]), GTK_STATE_NORMAL);
    ong.actif = i;
    gtk_widget_set_state(W(ONGLETS[i]), GTK_STATE_SELECTED);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(W("pas_notebook_onglets")), i);
    if (ong.anim) g_source_remove(ong.anim);
    ong.x0 = ong.px;
    ong.w0 = ong.pw;
    onglet_position(i, &ong.x1, &ong.w1);
    ong.etape = 0;
    ong.anim = g_timeout_add(16, pastille_tick, NULL);
    if (i == 2) stats_animer();
}

static void piste_alloc(GtkWidget *w, GtkAllocation *a, gpointer d)
{
    if (ong.anim == 0) onglet_position(ong.actif, &ong.px, &ong.pw);
}

void admin_onglet(GtkWidget *onglet)
{
    onglets_activer(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(onglet), "index")));
}

/* =====================================================================
 * Liste
 * ===================================================================== */

enum { L_CIN, L_NOM, L_PRENOM, L_NAISS, L_NAT, L_POINTS, L_NIVEAU, L_NIV_COUL, L_STATUT, L_STAT_COUL, L_N };

static void liste_rafraichir(void)
{
    static Passager vus[MAX_PASSAGERS];
    GtkListStore *st;
    GtkTreeIter it;
    char date[16], pts[16], compte[64], niv[40];
    int i, n, k;
    gchar *mot = g_strdup(gtk_entry_get_text(GTK_ENTRY(W("pas_entry_recherche"))));

    nettoyer(mot);
    n = passager_charger_tous(tous, MAX_PASSAGERS);
    k = mot[0] ? passager_filtrer(tous, n, mot, vus) : n;
    if (!mot[0]) memcpy(vus, tous, sizeof(Passager) * n);
    g_free(mot);
    passager_trier_nom(vus, k, tri_croissant);

    st = gtk_list_store_new(L_N, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    for (i = 0; i < k; i++) {
        const Passager *p = &vus[i];
        Niveau nv = passager_niveau(p->points);
        g_snprintf(date, sizeof date, "%02d/%02d/%04d", p->naissance.jour, p->naissance.mois, p->naissance.annee);
        ui_milliers(p->points, pts, sizeof pts);
        g_snprintf(niv, sizeof niv, "\xE2\x97\x86  %s", niveau_nom(nv));
        gtk_list_store_append(st, &it);
        gtk_list_store_set(st, &it, L_CIN, p->cin, L_NOM, p->nom, L_PRENOM, p->prenom, L_NAISS, date,
                           L_NAT, p->nationalite, L_POINTS, pts, L_NIVEAU, niv, L_NIV_COUL, theme_couleur_niveau(nv),
                           L_STATUT, p->bloque ? "\xE2\x97\x8F  Bloqué" : "\xE2\x97\x8F  Actif",
                           L_STAT_COUL, theme_couleur(p->bloque ? C_ERREUR : C_SUCCES), -1);
    }
    gtk_tree_view_set_model(GTK_TREE_VIEW(W("pas_table_liste")), GTK_TREE_MODEL(st));
    g_object_unref(st);
    g_snprintf(compte, sizeof compte, k > 1 ? "%d passagers" : "%d passager", k);
    gtk_label_set_text(GTK_LABEL(W("pas_t2_compte")), compte);
    ui_renommer(W("pas_btn_tri_asc"), tri_croissant ? "ca-btn-actif" : "ca-btn");
    ui_renommer(W("pas_btn_tri_desc"), tri_croissant ? "ca-btn" : "ca-btn-actif");
}

void admin_recherche(void) { liste_rafraichir(); }
void admin_trier(gboolean croissant) { tri_croissant = croissant; liste_rafraichir(); }

/* =====================================================================
 * Fiche (ajouter / modifier)
 * ===================================================================== */

static gboolean mode_modification(void) { return !gtk_widget_get_sensitive(W("pas_entry_cin")); }

void admin_vider(void)
{
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_cin")), "");
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_nom")), "");
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_prenom")), "");
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_tel")), "");
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_email")), "");
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(W("pas_spin_jour")), 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(W("pas_spin_mois")), 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(W("pas_spin_annee")), 2000);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(W("pas_coche_homme")), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(W("pas_coche_newsletter")), FALSE);
    gtk_combo_box_set_active(GTK_COMBO_BOX(W("pas_combo_nationalite")), 0);
    gtk_widget_set_sensitive(W("pas_entry_cin"), TRUE);
    gtk_widget_hide(W("pas_align_infos"));
    ui_bouton_actif(W("pas_btnd_supprimer"), FALSE);
    gtk_label_set_text(GTK_LABEL(W("pas_stitre_fiche")), "Nouveau passager");
    gtk_label_set_text(GTK_LABEL(W("pas_t2_fiche")), "Tous les champs sont obligatoires, sauf la newsletter.");
}

static void fiche_remplir(const Passager *p)
{
    char t[96];
    gchar *maj;
    Niveau nv = passager_niveau(p->points);
    size_t i;
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_cin")), p->cin);
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_nom")), p->nom);
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_prenom")), p->prenom);
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_tel")), p->tel);
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_email")), p->email);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(W("pas_spin_jour")), p->naissance.jour);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(W("pas_spin_mois")), p->naissance.mois);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(W("pas_spin_annee")), p->naissance.annee);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(W(p->sexe == 'F' ? "pas_coche_femme" : "pas_coche_homme")), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(W("pas_coche_newsletter")), p->newsletter);
    gtk_combo_box_set_active(GTK_COMBO_BOX(W("pas_combo_nationalite")), 5);
    for (i = 0; i < G_N_ELEMENTS(NATIONALITES); i++)
        if (strcmp(NATIONALITES[i], p->nationalite) == 0) gtk_combo_box_set_active(GTK_COMBO_BOX(W("pas_combo_nationalite")), (int) i);
    gtk_widget_set_sensitive(W("pas_entry_cin"), FALSE);         /* la clé ne se modifie pas */
    ui_bouton_actif(W("pas_btnd_supprimer"), TRUE);

    maj = g_utf8_strup(niveau_nom(nv), -1);
    gtk_label_set_text(GTK_LABEL(W("pas_chiptxt_niveau")), maj);
    g_free(maj);
    ui_milliers(p->points, t, sizeof t);
    g_strlcat(t, " PTS", sizeof t);
    gtk_label_set_text(GTK_LABEL(W("pas_chiptxt_points")), t);
    gtk_label_set_text(GTK_LABEL(W("pas_chiptxt_statut")), p->bloque ? "BLOQUÉ" : "ACTIF");
    ui_renommer(W("pas_chipok_statut"), p->bloque ? "ca-chip-err" : "ca-chip-ok");
    gtk_widget_show(W("pas_align_infos"));
    g_snprintf(t, sizeof t, "%s %s", p->prenom, p->nom);
    gtk_label_set_text(GTK_LABEL(W("pas_stitre_fiche")), t);
    g_snprintf(t, sizeof t, "CIN %s · le CIN ne se modifie pas.", p->cin);
    gtk_label_set_text(GTK_LABEL(W("pas_t2_fiche")), t);
}

void admin_nouveau(void) { admin_vider(); onglets_activer(1); gtk_widget_grab_focus(W("pas_entry_cin")); }

void admin_ligne_activee(GtkTreePath *chemin)
{
    GtkTreeModel *m = gtk_tree_view_get_model(GTK_TREE_VIEW(W("pas_table_liste")));
    GtkTreeIter it;
    gchar *cin;
    Passager p;
    if (!gtk_tree_model_get_iter(m, &it, chemin)) return;
    gtk_tree_model_get(m, &it, L_CIN, &cin, -1);
    if (passager_chercher(cin, &p)) {
        fiche_remplir(&p);
        onglets_activer(1);
        toast(TRUE, "Fiche chargée depuis la liste");
    }
    g_free(cin);
}

/* lit un champ texte, retire les espaces autour ; renvoie FALSE (et marque le champ) si invalide */
static gboolean lire(const char *champ, char *dest, size_t taille, int (*valide)(const char *), const char *message)
{
    g_strlcpy(dest, gtk_entry_get_text(GTK_ENTRY(W(champ))), taille);
    nettoyer(dest);
    if (!valide(dest)) { ui_marquer_erreur(W(champ)); toast(FALSE, message); return FALSE; }
    return TRUE;
}

static int email_ou_vide_refuse(const char *s) { return est_email_valide(s) && !contient_separateur(s); }

void admin_enregistrer(void)
{
    Passager p, ancien;
    gchar *nat;
    char msg[128];
    int r;

    passager_init(&p);
    if (!lire("pas_entry_cin", p.cin, sizeof p.cin, est_cin_valide, "CIN : exactement 8 chiffres")) return;
    if (!lire("pas_entry_nom", p.nom, sizeof p.nom, est_nom_valide, "Nom : lettres, espaces, - et ' uniquement")) return;
    if (!lire("pas_entry_prenom", p.prenom, sizeof p.prenom, est_nom_valide, "Prénom : lettres, espaces, - et ' uniquement")) return;
    p.naissance.jour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(W("pas_spin_jour")));
    p.naissance.mois = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(W("pas_spin_mois")));
    p.naissance.annee = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(W("pas_spin_annee")));
    if (!est_date_valide(p.naissance)) {
        ui_marquer_erreur(W("pas_spin_jour"));
        toast(FALSE, "Date de naissance impossible (jour inexistant ou date future)");
        return;
    }
    p.sexe = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(W("pas_coche_femme"))) ? 'F' : 'H';
    nat = gtk_combo_box_get_active_text(GTK_COMBO_BOX(W("pas_combo_nationalite")));
    g_strlcpy(p.nationalite, nat ? nat : "Autre", sizeof p.nationalite);
    g_free(nat);
    if (!lire("pas_entry_tel", p.tel, sizeof p.tel, est_tel_valide, "Téléphone : 8 chiffres commençant par 2, 4, 5, 7 ou 9")) return;
    if (!lire("pas_entry_email", p.email, sizeof p.email, email_ou_vide_refuse, "Adresse e-mail invalide")) return;
    p.newsletter = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(W("pas_coche_newsletter")));

    if (mode_modification()) {
        if (!passager_chercher(p.cin, &ancien)) { toast(FALSE, "Ce passager n'existe plus dans le fichier"); return; }
        p.points = ancien.points;                       /* la fiche ne touche ni aux points ni au blocage */
        p.bloque = ancien.bloque;
        g_strlcpy(p.motif, ancien.motif, sizeof p.motif);
        p.date_blocage = ancien.date_blocage;
        r = passager_modifier(&p);
        toast(r == 1, r == 1 ? "Modifications enregistrées" : "Erreur d'écriture dans data/passagers.txt");
    } else {
        r = passager_ajouter(&p);
        if (r == 0) { ui_marquer_erreur(W("pas_entry_cin")); toast(FALSE, "Ce CIN existe déjà"); return; }
        if (r < 0) { toast(FALSE, "Erreur d'écriture dans data/passagers.txt"); return; }
        auth_ajouter(p.cin, p.cin, "PASSAGER");          /* compte de connexion : mot de passe initial = CIN */
        g_snprintf(msg, sizeof msg, "Passager ajouté · identifiant et mot de passe : %s", p.cin);
        toast(TRUE, msg);
    }
    if (r == 1) { fiche_remplir(&p); liste_rafraichir(); }
}

void admin_supprimer(void)
{
    GtkWidget *dlg;
    int r;
    char cin[9];
    if (!mode_modification()) { toast(FALSE, "Chargez d'abord une fiche depuis la liste (double-clic)"); return; }
    g_strlcpy(cin, gtk_entry_get_text(GTK_ENTRY(W("pas_entry_cin"))), sizeof cin);
    dlg = gtk_message_dialog_new(GTK_WINDOW(fen), GTK_DIALOG_MODAL, GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE,
                                 "Supprimer définitivement %s %s ?", gtk_entry_get_text(GTK_ENTRY(W("pas_entry_prenom"))),
                                 gtk_entry_get_text(GTK_ENTRY(W("pas_entry_nom"))));
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dlg), "Sa fiche et son compte de connexion seront effacés.");
    gtk_window_set_title(GTK_WINDOW(dlg), "Confirmation");
    gtk_container_set_border_width(GTK_CONTAINER(dlg), 12);
    ui_bouton_style(gtk_dialog_add_button(GTK_DIALOG(dlg), "Annuler", GTK_RESPONSE_NO), "ca-btn-fantome");
    ui_bouton_style(gtk_dialog_add_button(GTK_DIALOG(dlg), "Supprimer", GTK_RESPONSE_YES), "ca-btn-danger");
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_NO);
    r = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
    if (r != GTK_RESPONSE_YES) return;
    if (passager_supprimer(cin) == 1) {
        auth_supprimer(cin);
        admin_vider();
        liste_rafraichir();
        toast(TRUE, "Passager supprimé");
    } else {
        toast(FALSE, "Suppression impossible");
    }
}

/* =====================================================================
 * Statistiques
 * ===================================================================== */

static void stats_animer(void)
{
    int age_n[4] = {0}, niv_n[4] = {0}, bloq = 0, somme = 0, i, a, n;
    char txt[32], nom[32];
    n = passager_charger_tous(tous, MAX_PASSAGERS);
    for (i = 0; i < n; i++) {
        a = calculer_age(tous[i].naissance);
        age_n[a < 18 ? 0 : a <= 30 ? 1 : a <= 50 ? 2 : 3]++;
        niv_n[passager_niveau(tous[i].points)]++;
        bloq += tous[i].bloque;
        somme += tous[i].points;
    }
    anim_compteur(W("pas_chiffre_kpi_total"), n, "");
    anim_compteur(W("pas_chiffre_kpi_bloques"), bloq, "");
    anim_compteur(W("pas_chiffre_kpi_platinum"), niv_n[3], "");
    anim_compteur(W("pas_chiffre_kpi_moyenne"), n ? somme / n : 0, "");
    for (i = 0; i < 4; i++) {
        g_snprintf(txt, sizeof txt, "%d · %d %%", age_n[i], n ? age_n[i] * 100 / n : 0);
        g_snprintf(nom, sizeof nom, "pas_val_age%d", i + 1);
        gtk_label_set_text(GTK_LABEL(W(nom)), txt);
        g_snprintf(nom, sizeof nom, "pas_jauge_age%d", i + 1);
        anim_barre(W(nom), n ? (double) age_n[i] / n : 0, i * 90);
        g_snprintf(txt, sizeof txt, "%d · %d %%", niv_n[i], n ? niv_n[i] * 100 / n : 0);
        g_snprintf(nom, sizeof nom, "pas_val_niv%d", i + 1);
        gtk_label_set_text(GTK_LABEL(W(nom)), txt);
        g_snprintf(nom, sizeof nom, "pas_jauge_niv%d", i + 1);
        anim_barre(W(nom), n ? (double) niv_n[i] / n : 0, i * 90);
    }
}

/* =====================================================================
 * Comptes bloqués
 * ===================================================================== */

static void bloques_rafraichir(void)
{
    GtkListStore *st = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkTreeIter it;
    char nom[128], date[16];
    int i, n = passager_charger_tous(tous, MAX_PASSAGERS);
    for (i = 0; i < n; i++) {
        if (!tous[i].bloque) continue;
        g_snprintf(nom, sizeof nom, "%s %s", tous[i].prenom, tous[i].nom);
        g_snprintf(date, sizeof date, "%02d/%02d/%04d", tous[i].date_blocage.jour, tous[i].date_blocage.mois,
                   tous[i].date_blocage.annee);
        gtk_list_store_append(st, &it);
        gtk_list_store_set(st, &it, 0, tous[i].cin, 1, nom, 2, tous[i].motif, 3, date, -1);
    }
    gtk_tree_view_set_model(GTK_TREE_VIEW(W("pas_table_bloques")), GTK_TREE_MODEL(st));
    g_object_unref(st);
    ui_bouton_actif(W("pas_btn_reactiver"), FALSE);
}

void admin_bloques_selection(void)
{
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(W("pas_table_bloques")));
    ui_bouton_actif(W("pas_btn_reactiver"), gtk_tree_selection_get_selected(sel, NULL, NULL));
}

void admin_reactiver(void)
{
    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(W("pas_table_bloques")));
    GtkTreeModel *m;
    GtkTreeIter it;
    gchar *cin;
    if (ui_bouton_est_off(W("pas_btn_reactiver")) || !gtk_tree_selection_get_selected(sel, &m, &it)) {
        toast(FALSE, "Sélectionnez d'abord un compte dans la liste");
        return;
    }
    gtk_tree_model_get(m, &it, 0, &cin, -1);
    if (passager_reactiver(cin) == 1) toast(TRUE, "Compte réactivé : le passager peut de nouveau se connecter");
    else toast(FALSE, "Réactivation impossible");
    g_free(cin);
    bloques_rafraichir();
    liste_rafraichir();
}

void admin_bloquer(void)
{
    char cin[16], motif[201];
    int r;
    if (!lire("pas_entry_cin_bloc", cin, sizeof cin, est_cin_valide, "CIN : exactement 8 chiffres")) return;
    g_strlcpy(motif, gtk_entry_get_text(GTK_ENTRY(W("pas_entry_motif"))), sizeof motif);
    nettoyer(motif);
    if (!est_texte_valide(motif, 200)) {
        ui_marquer_erreur(W("pas_entry_motif"));
        toast(FALSE, "Le motif est obligatoire (sans point-virgule)");
        return;
    }
    if (!passager_existe(cin)) { ui_marquer_erreur(W("pas_entry_cin_bloc")); toast(FALSE, "Aucun passager avec ce CIN"); return; }
    r = passager_bloquer(cin, motif);
    if (r == 0) { toast(FALSE, "Ce compte est déjà bloqué"); return; }
    if (r < 0) { toast(FALSE, "Erreur d'écriture dans data/passagers.txt"); return; }
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_cin_bloc")), "");
    gtk_entry_set_text(GTK_ENTRY(W("pas_entry_motif")), "");
    toast(TRUE, "Compte bloqué : la connexion de ce passager est suspendue");
    bloques_rafraichir();
    liste_rafraichir();
}

/* =====================================================================
 * Ouverture / fermeture de la fenêtre
 * ===================================================================== */

static void theme_change(void)          /* les couleurs des niveaux sont écrites dans la liste */
{
    if (fen != NULL) liste_rafraichir();
}

static void colonnes(void)
{
    GtkTreeView *tv = GTK_TREE_VIEW(W("pas_table_liste"));
    ui_colonne(tv, "CIN", L_CIN, -1, 0, FALSE);
    ui_colonne(tv, "NOM", L_NOM, -1, 0, TRUE);
    ui_colonne(tv, "PRÉNOM", L_PRENOM, -1, 0, FALSE);
    ui_colonne(tv, "NAISSANCE", L_NAISS, -1, 0, FALSE);
    ui_colonne(tv, "NATIONALITÉ", L_NAT, -1, 0, FALSE);
    ui_colonne(tv, "POINTS", L_POINTS, -1, 1, FALSE);
    ui_colonne(tv, "NIVEAU", L_NIVEAU, L_NIV_COUL, 0, TRUE);
    ui_colonne(tv, "STATUT", L_STATUT, L_STAT_COUL, 0, TRUE);
    tv = GTK_TREE_VIEW(W("pas_table_bloques"));
    ui_colonne(tv, "CIN", 0, -1, 0, FALSE);
    ui_colonne(tv, "PASSAGER", 1, -1, 0, TRUE);
    ui_colonne(tv, "MOTIF", 2, -1, 0, FALSE);
    ui_colonne(tv, "BLOQUÉ LE", 3, -1, 0, FALSE);
}

void admin_ouvrir(void)
{
    static const char *codes[] = {"pa", "vo", "re", "pn", "av", "ae"};
    static const char *sous[] = {
        "Comptes, fidélité, statistiques et blocage — module 1",
        "Programmation et recherche des vols — module 2",
        "Réservations, prix et services — module 3",
        "Équipages, plannings et affectations — module 4",
        "Flotte, disponibilité et maintenance — module 5",
        "Aéroports et charge journalière — module 6",
    };
    int i;

    fen = create_window_admin();
    ui_habiller(fen);
    shell = shell_attacher(fen, "adm", codes, sous, 6, session_login(), "AD");

    memset(&ong, 0, sizeof ong);
    for (i = 0; i < 4; i++) g_object_set_data(G_OBJECT(W(ONGLETS[i])), "index", GINT_TO_POINTER(i));
    gtk_widget_set_state(W(ONGLETS[0]), GTK_STATE_SELECTED);
    g_signal_connect(W("pas_piste_onglets"), "expose-event", G_CALLBACK(piste_expose), NULL);
    g_signal_connect(W("pas_piste_onglets"), "size-allocate", G_CALLBACK(piste_alloc), NULL);

    colonnes();
    ui_indice(W("pas_entry_recherche"), "Rechercher un nom, un prénom ou un CIN");
    gtk_widget_set_tooltip_text(W("pas_entry_recherche"), "La liste se filtre à chaque lettre");
    ui_indice(W("pas_entry_motif"), "Ex. : impayé, comportement à bord...");

    admin_vider();
    liste_rafraichir();
    bloques_rafraichir();
    theme_ecouter(theme_change);
    shell_montrer(shell);
    gtk_widget_grab_focus(W("pas_entry_recherche"));
}

void admin_fermer(void)
{
    if (fen == NULL) return;
    if (ong.anim) g_source_remove(ong.anim);
    ong.anim = 0;
    theme_oublier(theme_change);
    shell_liberer(shell);
    shell = NULL;
    fen = NULL;
    session_fermer();
    connexion_revenir();
}

gboolean admin_touche(GdkEventKey *e)
{
    if (shell_touche(shell, e)) return TRUE;
    if ((e->state & GDK_MOD1_MASK) && e->keyval >= GDK_1 && e->keyval <= GDK_4 && shell->actif == 0) {
        onglets_activer((int) (e->keyval - GDK_1));
        return TRUE;
    }
    return FALSE;
}
