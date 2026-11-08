/*
 * ui_theme.c — thème Jour / Nuit + habillage cairo des widgets Glade
 *
 * GTK 2 ne sait dessiner ni coins arrondis, ni ombres, ni boutons « pilule ».
 * On garde donc les widgets ordinaires créés par Glade (GtkButton, GtkEntry,
 * GtkProgressBar...) et on remplace seulement leur DESSIN par du cairo,
 * en se basant sur le rôle écrit dans leur nom Glade.
 *
 * Plan : 1. palettes + gtkrc   2. outils cairo   3. formes et transitions
 *        4. widgets spéciaux   5. table des rôles + ui_habiller()
 */
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "ui_theme.h"
#include "support.h"

/* =====================================================================
 * 1. PALETTES ET GTKRC — toutes les couleurs sont dans ces deux tableaux
 * ===================================================================== */

static const char *JETONS[C_N] = {
    "@fond@", "@surface@", "@surface2@", "@champ@", "@ligne@", "@texte@", "@texte2@", "@texte3@", "@titre@", "@marque@",
    "@accent@", "@accent_h@", "@accent_a@", "@sur_accent@", "@selection@", "@or@", "@succes@", "@succes_f@",
    "@erreur@", "@erreur_f@", "@sur_erreur@", "@alt@"
};

/* Jour : blanc cassé, bordeaux Carthage, filets dorés */
static const char *JOUR[C_N] = {
    "#F6F4F1", "#FFFFFF", "#F3F0EC", "#FAF8F6", "#E7E1DA", "#1F1A1C", "#625A5E", "#77707A", "#1F1A1C", "#8C1D2C",
    "#8C1D2C", "#A3263A", "#6F1622", "#FFFFFF", "#F7E9EB", "#B8924A", "#1E7B45", "#E4F2E8",
    "#B3261E", "#FBEAE8", "#FFFFFF", "#FCFBFA"
};

/* Nuit : anthracite chaud, or, bordeaux en teinte de sélection */
static const char *NUIT[C_N] = {
    "#121113", "#1B1A1D", "#252328", "#201E23", "#34313A", "#F3EFEA", "#ABA4A2", "#918A88", "#F3EFEA", "#D9B872",
    "#D4B26A", "#E3C688", "#B9954F", "#1B1408", "#3A2329", "#D4B26A", "#5CC994", "#16291F",
    "#FF8277", "#3B1C1A", "#1B0B09", "#1E1D20"
};

static const char *NIV_JOUR[4] = {"#625A5E", "#56606C", "#8A6420", "#8C1D2C"};
static const char *NIV_NUIT[4] = {"#ABA4A2", "#C9D1DB", "#E3C688", "#E6A0AA"};

static const char **T = JOUR;

static const char RC_MODELE[] =
"style \"ca-base\" {\n"
"  bg[NORMAL] = \"@fond@\" bg[PRELIGHT] = \"@surface2@\" bg[SELECTED] = \"@accent@\" bg[ACTIVE] = \"@surface2@\" bg[INSENSITIVE] = \"@fond@\"\n"
"  fg[NORMAL] = \"@texte@\" fg[PRELIGHT] = \"@texte@\" fg[SELECTED] = \"@sur_accent@\" fg[ACTIVE] = \"@texte@\" fg[INSENSITIVE] = \"@texte3@\"\n"
"  base[NORMAL] = \"@champ@\" base[SELECTED] = \"@accent@\" base[ACTIVE] = \"@selection@\" base[INSENSITIVE] = \"@champ@\"\n"
"  text[NORMAL] = \"@texte@\" text[SELECTED] = \"@sur_accent@\" text[ACTIVE] = \"@texte@\" text[INSENSITIVE] = \"@texte2@\"\n"
"  font_name = \"DejaVu Sans 10\"\n"
"  GtkWidget::cursor-color = \"@accent@\"\n"
"  GtkWidget::focus-line-width = 0\n"
"  GtkCheckButton::indicator-size = 18\n"
"  GtkCheckButton::indicator-spacing = 4\n"
"}\n"
"class \"GtkWidget\" style \"ca-base\"\n"
"style \"ca-surface\" { bg[NORMAL] = \"@surface@\" bg[PRELIGHT] = \"@surface@\" bg[SELECTED] = \"@surface@\" bg[ACTIVE] = \"@surface@\" }\n"
"widget \"*ca-topbar\" style \"ca-surface\"\n"
"widget \"*ca-sidebar\" style \"ca-surface\"\n"
"widget \"*ca-avatar\" style \"ca-surface\"\n"
"widget \"*ca-sur-carte\" style \"ca-surface\"\n"
"style \"ca-ligne\" { bg[NORMAL] = \"@ligne@\" }\n"
"widget \"*ca-ligne\" style \"ca-ligne\"\n"
/* textes */
"style \"ca-titre\" { fg[NORMAL] = \"@titre@\" font_name = \"DejaVu Serif 23\" }\n"
"widget \"*ca-titre\" style \"ca-titre\"\n"
"style \"ca-titre-moyen\" { fg[NORMAL] = \"@titre@\" font_name = \"DejaVu Serif 16\" }\n"
"widget \"*ca-titre-moyen\" style \"ca-titre-moyen\"\n"
"style \"ca-titre-err\" { fg[NORMAL] = \"@erreur@\" font_name = \"DejaVu Serif 18\" }\n"
"widget \"*ca-titre-err\" style \"ca-titre-err\"\n"
"style \"ca-chiffre\" { fg[NORMAL] = \"@titre@\" font_name = \"DejaVu Serif 25\" }\n"
"widget \"*ca-chiffre\" style \"ca-chiffre\"\n"
"style \"ca-marque\" { fg[NORMAL] = \"@marque@\" font_name = \"DejaVu Serif Bold 13\" }\n"
"widget \"*ca-marque\" style \"ca-marque\"\n"
"style \"ca-t2\" { fg[NORMAL] = \"@texte2@\" }\n"
"widget \"*ca-t2\" style \"ca-t2\"\n"
"style \"ca-t3\" { fg[NORMAL] = \"@texte3@\" font_name = \"DejaVu Sans 9\" }\n"
"widget \"*ca-t3\" style \"ca-t3\"\n"
"style \"ca-surtitre\" { fg[NORMAL] = \"@texte2@\" font_name = \"DejaVu Sans Bold 8\" }\n"
"widget \"*ca-surtitre\" style \"ca-surtitre\"\n"
"style \"ca-valeur\" { fg[NORMAL] = \"@texte@\" font_name = \"DejaVu Sans Bold 10\" }\n"
"widget \"*ca-valeur\" style \"ca-valeur\"\n"
"style \"ca-accent-txt\" { fg[NORMAL] = \"@accent@\" font_name = \"DejaVu Sans Bold 10\" }\n"
"widget \"*ca-accent-txt\" style \"ca-accent-txt\"\n"
"style \"ca-erreur\" { fg[NORMAL] = \"@erreur@\" font_name = \"DejaVu Sans Bold 9\" }\n"
"widget \"*ca-erreur\" style \"ca-erreur\"\n"
"style \"ca-mono\" { fg[NORMAL] = \"@texte3@\" font_name = \"DejaVu Sans Mono 8\" }\n"
"widget \"*ca-mono\" style \"ca-mono\"\n"
/* textes posés sur le panneau bordeaux de la connexion (même rendu de jour comme de nuit) */
"style \"ca-or\" { fg[NORMAL] = \"#EBCC8C\" font_name = \"DejaVu Serif Bold 11\" }\n"
"widget \"*ca-or\" style \"ca-or\"\n"
"style \"ca-blanc\" { fg[NORMAL] = \"#FFFFFF\" font_name = \"DejaVu Serif 24\" }\n"
"widget \"*ca-blanc\" style \"ca-blanc\"\n"
"style \"ca-blancp\" { fg[NORMAL] = \"#E8D2D6\" font_name = \"DejaVu Sans 9\" }\n"
"widget \"*ca-blancp\" style \"ca-blancp\"\n"
/* pastilles et messages : couleur du texte (le fond est dessiné en cairo) */
"style \"ca-chip\" { fg[NORMAL] = \"@texte2@\" font_name = \"DejaVu Sans Bold 8\" }\n"
"widget \"*ca-chip*\" style \"ca-chip\"\n"
"style \"ca-chip-accent\" { fg[NORMAL] = \"@accent@\" font_name = \"DejaVu Sans Bold 8\" }\n"
"widget \"*ca-chip-accent*\" style \"ca-chip-accent\"\n"
"style \"ca-chip-ok\" { fg[NORMAL] = \"@succes@\" font_name = \"DejaVu Sans Bold 8\" }\n"
"widget \"*ca-chip-ok*\" style \"ca-chip-ok\"\n"
"style \"ca-chip-err\" { fg[NORMAL] = \"@erreur@\" font_name = \"DejaVu Sans Bold 8\" }\n"
"widget \"*ca-chip-err*\" style \"ca-chip-err\"\n"
"style \"ca-toast-ok\" { fg[NORMAL] = \"@succes@\" font_name = \"DejaVu Sans Bold 9\" }\n"
"widget \"*ca-toast-ok*\" style \"ca-toast-ok\"\n"
"style \"ca-toast-err\" { fg[NORMAL] = \"@erreur@\" font_name = \"DejaVu Sans Bold 9\" }\n"
"widget \"*ca-toast-err*\" style \"ca-toast-err\"\n"
/* menu, onglets, paliers */
"style \"ca-nav\" { fg[NORMAL] = \"@texte2@\" fg[PRELIGHT] = \"@texte@\" fg[SELECTED] = \"@accent@\" font_name = \"DejaVu Sans 10\" }\n"
"widget \"*ca-nav*\" style \"ca-nav\"\n"
"style \"ca-nav-badge\" { fg[NORMAL] = \"@texte2@\" fg[PRELIGHT] = \"@accent@\" fg[SELECTED] = \"@sur_accent@\" font_name = \"DejaVu Sans Bold 8\" }\n"
"widget \"*ca-nav-badge\" style \"ca-nav-badge\"\n"
"style \"ca-icone\" { fg[NORMAL] = \"@texte@\" fg[PRELIGHT] = \"@accent@\" fg[ACTIVE] = \"@accent@\" font_name = \"DejaVu Sans 16\" }\n"
"widget \"*ca-icone*\" style \"ca-icone\"\n"
"style \"ca-onglet\" { fg[NORMAL] = \"@texte2@\" fg[PRELIGHT] = \"@texte@\" fg[SELECTED] = \"@texte@\" font_name = \"DejaVu Sans Bold 9\" }\n"
"widget \"*ca-onglet*\" style \"ca-onglet\"\n"
"style \"ca-palier\" { fg[NORMAL] = \"@texte2@\" }\n"
"widget \"*ca-palier*\" style \"ca-palier\"\n"
"style \"ca-palier-actif\" { fg[NORMAL] = \"@accent@\" }\n"
"widget \"*ca-palier-actif*\" style \"ca-palier-actif\"\n"
/* boutons : seul le texte est géré ici, la pilule est dessinée en cairo */
"style \"ca-btn\" {\n"
"  fg[NORMAL] = \"@texte@\" fg[PRELIGHT] = \"@texte@\" fg[ACTIVE] = \"@texte@\" fg[INSENSITIVE] = \"@texte3@\"\n"
"  GtkButton::inner-border = { 20, 20, 9, 9 }\n"
"  GtkButton::default-border = { 0, 0, 0, 0 }\n"
"  GtkButton::default-outside-border = { 0, 0, 0, 0 }\n"
"  GtkButton::child-displacement-x = 0\n"
"  GtkButton::child-displacement-y = 0\n"
"  font_name = \"DejaVu Sans Bold 9\"\n"
"}\n"
"widget_class \"*GtkButton*\" style \"ca-btn\"\n"
"style \"ca-btn-primaire\" = \"ca-btn\" { fg[NORMAL] = \"@sur_accent@\" fg[PRELIGHT] = \"@sur_accent@\" fg[ACTIVE] = \"@sur_accent@\" }\n"
"widget \"*ca-btn-primaire*\" style \"ca-btn-primaire\"\n"
"style \"ca-btn-fantome\" = \"ca-btn\" { fg[NORMAL] = \"@texte2@\" fg[PRELIGHT] = \"@texte@\" fg[ACTIVE] = \"@texte@\" }\n"
"widget \"*ca-btn-fantome*\" style \"ca-btn-fantome\"\n"
"style \"ca-btn-danger\" = \"ca-btn\" { fg[NORMAL] = \"@erreur@\" fg[PRELIGHT] = \"@sur_erreur@\" fg[ACTIVE] = \"@sur_erreur@\" }\n"
"widget \"*ca-btn-danger*\" style \"ca-btn-danger\"\n"
"style \"ca-btn-actif\" = \"ca-btn\" { fg[NORMAL] = \"@accent@\" fg[PRELIGHT] = \"@accent@\" fg[ACTIVE] = \"@accent@\" }\n"
"widget \"*ca-btn-actif*\" style \"ca-btn-actif\"\n"
"style \"ca-btn-off\" = \"ca-btn\" { fg[NORMAL] = \"@texte3@\" fg[PRELIGHT] = \"@texte3@\" fg[ACTIVE] = \"@texte3@\" }\n"
"widget \"*ca-btn-off*\" style \"ca-btn-off\"\n"
/* champs : sans cadre GTK, la boîte arrondie est dessinée autour */
"style \"ca-entry\" {\n"
"  base[NORMAL] = \"@champ@\" base[INSENSITIVE] = \"@champ@\" text[NORMAL] = \"@texte@\" text[INSENSITIVE] = \"@texte3@\"\n"
"  base[SELECTED] = \"@accent@\" text[SELECTED] = \"@sur_accent@\" base[ACTIVE] = \"@accent@\" text[ACTIVE] = \"@sur_accent@\"\n"
"  bg[NORMAL] = \"@champ@\" bg[PRELIGHT] = \"@surface2@\" bg[ACTIVE] = \"@ligne@\" bg[INSENSITIVE] = \"@champ@\"\n"
"  fg[NORMAL] = \"@texte2@\" fg[PRELIGHT] = \"@accent@\" fg[ACTIVE] = \"@accent@\"\n"
"  xthickness = 0 ythickness = 0\n"
"  GtkEntry::inner-border = { 4, 4, 7, 7 }\n"
"  font_name = \"DejaVu Sans 10\"\n"
"}\n"
"class \"GtkEntry\" style \"ca-entry\"\n"
"style \"ca-combo\" { fg[NORMAL] = \"@texte@\" fg[PRELIGHT] = \"@texte@\" fg[ACTIVE] = \"@texte@\" text[NORMAL] = \"@texte@\" }\n"
"widget_class \"*GtkComboBox*\" style \"ca-combo\"\n"
"style \"ca-menu\" { bg[NORMAL] = \"@surface@\" bg[PRELIGHT] = \"@selection@\" fg[NORMAL] = \"@texte@\" fg[PRELIGHT] = \"@accent@\" xthickness = 1 ythickness = 3 }\n"
"class \"GtkMenu\" style \"ca-menu\"\n"
"widget_class \"*GtkMenuItem*\" style \"ca-menu\"\n"
"style \"ca-check\" { fg[NORMAL] = \"@texte@\" fg[PRELIGHT] = \"@texte@\" fg[ACTIVE] = \"@texte@\" }\n"
"class \"GtkCheckButton\" style \"ca-check\"\n"
/* tableau */
"style \"ca-table\" {\n"
"  base[NORMAL] = \"@surface@\" text[NORMAL] = \"@texte@\"\n"
"  base[SELECTED] = \"@selection@\" text[SELECTED] = \"@texte@\" base[ACTIVE] = \"@selection@\" text[ACTIVE] = \"@texte@\"\n"
"  bg[NORMAL] = \"@surface@\"\n"
"  GtkTreeView::even-row-color = \"@surface@\"\n"
"  GtkTreeView::odd-row-color = \"@alt@\"\n"
"  GtkTreeView::vertical-separator = 16\n"
"  GtkTreeView::horizontal-separator = 16\n"
"}\n"
"widget \"*ca-table\" style \"ca-table\"\n"
"style \"ca-table-entete\" { fg[NORMAL] = \"@texte3@\" fg[PRELIGHT] = \"@texte@\" fg[ACTIVE] = \"@texte@\" font_name = \"DejaVu Sans Bold 8\" GtkButton::inner-border = { 8, 8, 10, 10 } }\n"
"widget \"*ca-table.*\" style \"ca-table-entete\"\n"
"style \"ca-scroll\" { bg[NORMAL] = \"@ligne@\" bg[PRELIGHT] = \"@texte3@\" bg[ACTIVE] = \"@surface@\" bg[SELECTED] = \"@ligne@\"\n"
"  GtkScrollbar::has-backward-stepper = 0 GtkScrollbar::has-forward-stepper = 0\n"
"  GtkRange::slider-width = 8 GtkRange::trough-border = 2 GtkRange::stepper-size = 0 xthickness = 0 ythickness = 0 }\n"
"class \"GtkScrollbar\" style \"ca-scroll\"\n"
"style \"ca-barre\" { bg[NORMAL] = \"@surface@\" }\n"
"class \"GtkProgressBar\" style \"ca-barre\"\n"
/* boîtes de dialogue (confirmation de suppression) */
"style \"ca-dialogue\" { bg[NORMAL] = \"@surface@\" }\n"
"class \"GtkDialog\" style \"ca-dialogue\"\n";

static GSList *bascules = NULL;      /* labels « ☀ Jour / ☾ Nuit » à mettre à jour */
static GSList *rappels = NULL;       /* fonctions à appeler après une bascule de thème */

static void remplacer_tous(GString *s, const char *jeton, const char *val)
{
    gsize lj = strlen(jeton), lv = strlen(val), pos = 0;
    gchar *p;
    while ((p = strstr(s->str + pos, jeton)) != NULL) {
        pos = (gsize) (p - s->str);
        g_string_erase(s, pos, lj);
        g_string_insert(s, pos, val);
        pos += lv;
    }
}

static void theme_appliquer(void)
{
    GString *s = g_string_new(RC_MODELE);
    int i;
    for (i = 0; i < C_N; i++) remplacer_tous(s, JETONS[i], T[i]);
    gtk_rc_parse_string(s->str);
    gtk_rc_reset_styles(gtk_settings_get_default());
    g_string_free(s, TRUE);
}

void theme_init(gboolean nuit)
{
    /* thème GTK neutre intégré à GTK : même rendu sur toutes les machines */
    g_object_set(gtk_settings_get_default(), "gtk-theme-name", "Raleigh", "gtk-button-images", FALSE, NULL);
    T = nuit ? NUIT : JOUR;
    theme_appliquer();
}

gboolean theme_est_nuit(void) { return T == NUIT; }
const char *theme_couleur(int c) { return T[c]; }
const char *theme_couleur_niveau(int n) { return (T == NUIT ? NIV_NUIT : NIV_JOUR)[CLAMP(n, 0, 3)]; }
void theme_ecouter(void (*rappel)(void)) { rappels = g_slist_append(rappels, (gpointer) rappel); }
void theme_oublier(void (*rappel)(void)) { rappels = g_slist_remove(rappels, (gpointer) rappel); }

static const char *texte_bascule(void)
{
    return T == JOUR ? "\xE2\x98\x80  Jour" : "\xE2\x98\xBE  Nuit";
}

void theme_basculer(void)
{
    GSList *l;
    GList *fen, *f;
    T = (T == JOUR) ? NUIT : JOUR;
    theme_appliquer();
    for (l = bascules; l != NULL; l = l->next) gtk_label_set_text(GTK_LABEL(l->data), texte_bascule());
    for (l = rappels; l != NULL; l = l->next) ((void (*)(void)) l->data)();
    fen = gtk_window_list_toplevels();
    for (f = fen; f != NULL; f = f->next) gtk_widget_queue_draw(GTK_WIDGET(f->data));
    g_list_free(fen);
}

/* =====================================================================
 * 2. OUTILS CAIRO
 * ===================================================================== */

void ui_rgb(int c, double *r, double *g, double *b)
{
    unsigned int v = (unsigned int) strtoul(T[c] + 1, NULL, 16);
    *r = ((v >> 16) & 0xFF) / 255.0;
    *g = ((v >> 8) & 0xFF) / 255.0;
    *b = (v & 0xFF) / 255.0;
}

void ui_source(cairo_t *cr, int c, double alpha)
{
    double r, g, b;
    ui_rgb(c, &r, &g, &b);
    cairo_set_source_rgba(cr, r, g, b, alpha);
}

/* couleur intermédiaire entre deux couleurs du thème (transitions de survol) */
static void source_mix(cairo_t *cr, int c1, int c2, double t)
{
    double r1, g1, b1, r2, g2, b2;
    if (c1 < 0 && c2 < 0) return;
    if (c1 < 0) { ui_source(cr, c2, t); return; }
    if (c2 < 0) { ui_source(cr, c1, 1 - t); return; }
    ui_rgb(c1, &r1, &g1, &b1);
    ui_rgb(c2, &r2, &g2, &b2);
    cairo_set_source_rgb(cr, r1 + (r2 - r1) * t, g1 + (g2 - g1) * t, b1 + (b2 - b1) * t);
}

void ui_arrondi(cairo_t *cr, double x, double y, double w, double h, double r)
{
    if (r > h / 2) r = h / 2;
    if (r > w / 2) r = w / 2;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
    cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI);
    cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
}

/* ombre douce : plusieurs contours transparents empilés */
void ui_ombre(cairo_t *cr, double x, double y, double w, double h, double r, int force)
{
    int i, n = force == 2 ? 5 : 3;
    double a = (T == NUIT ? 0.09 : 0.014) * (force == 2 ? 1.0 : 1.4);
    for (i = n; i >= 1; i--) {
        cairo_set_source_rgba(cr, 0.20, 0.08, 0.10, a);
        ui_arrondi(cr, x - i * 0.5, y + i * 0.9, w + i, h + i * 0.4, r + i * 0.5);
        cairo_fill(cr);
    }
}

void ui_texte(cairo_t *cr, const char *police, const char *markup, double x, double y, int aligne)
{
    PangoLayout *ly = pango_cairo_create_layout(cr);
    PangoFontDescription *fd = pango_font_description_from_string(police);
    int w, h;
    pango_layout_set_font_description(ly, fd);
    pango_layout_set_markup(ly, markup, -1);
    pango_layout_get_pixel_size(ly, &w, &h);
    cairo_move_to(cr, aligne == 1 ? x - w : aligne == 2 ? x - w / 2.0 : x, aligne == 2 ? y - h / 2.0 : y);
    pango_cairo_show_layout(cr, ly);
    pango_font_description_free(fd);
    g_object_unref(ly);
}

void ui_milliers(int v, char *out, size_t taille)
{
    if (v >= 1000) g_snprintf(out, taille, "%d %03d", v / 1000, v % 1000);
    else g_snprintf(out, taille, "%d", v);
}

/* =====================================================================
 * 3. FORMES : comment dessiner un widget selon son état GTK
 *    Indices : NORMAL, ACTIVE (appuyé), PRELIGHT (survol), SELECTED, INSENSITIVE. -1 = rien.
 * ===================================================================== */

typedef struct {
    const char *nom;
    int fond[5], bord[5];
    double rayon;          /* < 0 : pilule (rayon = demi-hauteur) */
    int ombre, marge;
} Forme;

static const Forme FORMES[] = {
    {"ca-carte",        {C_SURFACE, C_SURFACE, C_SURFACE, C_SURFACE, C_SURFACE}, {C_LIGNE, C_LIGNE, C_LIGNE, C_LIGNE, C_LIGNE}, 16, 2, 5},
    {"ca-champ-boite",  {C_CHAMP, C_CHAMP, C_CHAMP, C_CHAMP, C_SURFACE2}, {C_LIGNE, C_LIGNE, C_TEXTE3, C_LIGNE, C_LIGNE}, 10, 0, 0},
    {"ca-btn-primaire", {C_ACCENT, C_ACCENT_A, C_ACCENT_H, C_ACCENT, C_SURFACE2}, {-1, -1, -1, -1, -1}, -1, 1, 2},
    {"ca-btn-fantome",  {-1, C_SURFACE2, C_SURFACE2, -1, -1}, {-1, -1, -1, -1, -1}, -1, 0, 2},
    {"ca-btn-danger",   {C_ERREUR_F, C_ERREUR, C_ERREUR, C_ERREUR_F, C_SURFACE2}, {-1, -1, -1, -1, -1}, -1, 0, 2},
    {"ca-btn-actif",    {C_SELECTION, C_SELECTION, C_SELECTION, C_SELECTION, C_SELECTION}, {C_ACCENT, C_ACCENT, C_ACCENT, C_ACCENT, C_ACCENT}, -1, 0, 2},
    {"ca-btn-off",      {C_SURFACE2, C_SURFACE2, C_SURFACE2, C_SURFACE2, C_SURFACE2}, {-1, -1, -1, -1, -1}, -1, 0, 2},
    {"ca-btn",          {C_SURFACE, C_SURFACE2, C_SURFACE2, C_SURFACE, C_SURFACE2}, {C_LIGNE, C_LIGNE, C_TEXTE3, C_LIGNE, C_LIGNE}, -1, 0, 2},
    {"ca-nav",          {-1, C_SURFACE2, C_SURFACE2, C_SELECTION, -1}, {-1, -1, -1, -1, -1}, 12, 0, 0},
    {"ca-icone",        {-1, C_SURFACE2, C_SURFACE2, -1, -1}, {-1, -1, -1, -1, -1}, 12, 0, 6},
    {"ca-bascule",      {C_SURFACE2, C_LIGNE, C_LIGNE, C_SURFACE2, C_SURFACE2}, {-1, -1, -1, -1, -1}, -1, 0, 0},
    {"ca-chip-accent",  {C_SELECTION, C_SELECTION, C_SELECTION, C_SELECTION, C_SELECTION}, {-1, -1, -1, -1, -1}, -1, 0, 0},
    {"ca-chip-ok",      {C_SUCCES_F, C_SUCCES_F, C_SUCCES_F, C_SUCCES_F, C_SUCCES_F}, {-1, -1, -1, -1, -1}, -1, 0, 0},
    {"ca-chip-err",     {C_ERREUR_F, C_ERREUR_F, C_ERREUR_F, C_ERREUR_F, C_ERREUR_F}, {-1, -1, -1, -1, -1}, -1, 0, 0},
    {"ca-chip",         {C_SURFACE2, C_SURFACE2, C_SURFACE2, C_SURFACE2, C_SURFACE2}, {-1, -1, -1, -1, -1}, -1, 0, 0},
    {"ca-toast-ok",     {C_SUCCES_F, C_SUCCES_F, C_SUCCES_F, C_SUCCES_F, C_SUCCES_F}, {-1, -1, -1, -1, -1}, -1, 0, 0},
    {"ca-toast-err",    {C_ERREUR_F, C_ERREUR_F, C_ERREUR_F, C_ERREUR_F, C_ERREUR_F}, {-1, -1, -1, -1, -1}, -1, 0, 0},
    {"ca-palier-actif", {C_SELECTION, C_SELECTION, C_SELECTION, C_SELECTION, C_SELECTION}, {-1, -1, -1, -1, -1}, 10, 0, 0},
};

static const Forme *forme_pour(GtkWidget *w)
{
    const char *nom = gtk_widget_get_name(w);
    size_t i;
    for (i = 0; i < G_N_ELEMENTS(FORMES); i++)
        if (strcmp(nom, FORMES[i].nom) == 0) return &FORMES[i];
    return NULL;
}

/* --- transition douce entre deux états (≈ 120 ms) --- */

typedef struct { GtkWidget *w; double t; int memo; guint id; } Transition;

static Transition *transition(GtkWidget *w)
{
    Transition *tr = g_object_get_data(G_OBJECT(w), "transition");
    if (tr == NULL) {
        tr = g_new0(Transition, 1);
        tr->w = w;
        tr->memo = GTK_STATE_PRELIGHT;
        g_object_set_data_full(G_OBJECT(w), "transition", tr, g_free);
    }
    return tr;
}

static gboolean transition_tick(gpointer data)
{
    Transition *tr = data;
    int etat = gtk_widget_get_state(tr->w);
    double cible = (etat == GTK_STATE_NORMAL) ? 0.0 : 1.0;
    tr->t += cible > tr->t ? 0.16 : -0.16;
    if (fabs(tr->t - cible) < 0.16) tr->t = cible;
    gtk_widget_queue_draw(tr->w);
    if (tr->t == cible) { tr->id = 0; g_object_unref(tr->w); return FALSE; }
    return TRUE;
}

static void etat_change(GtkWidget *w, GtkStateType ancien, gpointer d)
{
    Transition *tr = transition(w);
    int etat = gtk_widget_get_state(w);
    if (etat == GTK_STATE_INSENSITIVE) return;
    if (etat != GTK_STATE_NORMAL) tr->memo = etat;
    if (etat == GTK_STATE_ACTIVE) { tr->t = 1; gtk_widget_queue_draw(w); return; }   /* clic : immédiat */
    if (tr->id == 0) { g_object_ref(w); tr->id = g_timeout_add(16, transition_tick, tr); }
}

static void champ_dessiner(cairo_t *cr, GtkWidget *w, const Forme *f, double x, double y, double W, double H, double r)
{
    gboolean focus = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "focus"));
    gboolean erreur = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "erreur"));
    ui_arrondi(cr, x + 0.5, y + 0.5, W - 1, H - 1, r);
    ui_source(cr, erreur ? C_ERREUR_F : f->fond[0], 1);
    cairo_fill_preserve(cr);
    ui_source(cr, erreur ? C_ERREUR : focus ? C_ACCENT : C_LIGNE, 1);
    cairo_set_line_width(cr, focus || erreur ? 1.6 : 1.0);
    cairo_stroke(cr);
}

static gboolean forme_expose(GtkWidget *w, GdkEventExpose *e, gpointer d)
{
    const Forme *f = forme_pour(w);
    GtkAllocation a;
    cairo_t *cr;
    double x, y, W, H, r, t;
    int etat = gtk_widget_get_state(w), memo;
    Transition *tr;

    if (f == NULL) return FALSE;
    gtk_widget_get_allocation(w, &a);
    cr = gdk_cairo_create(e->window);
    gdk_cairo_region(cr, e->region);
    cairo_clip(cr);
    x = a.x + f->marge;
    y = a.y + f->marge;
    W = a.width - 2 * f->marge;
    H = a.height - 2 * f->marge;
    r = f->rayon < 0 ? H / 2 : f->rayon;

    if (f->ombre) ui_ombre(cr, x, y, W, H, r, f->ombre);

    if (strcmp(f->nom, "ca-champ-boite") == 0) {
        champ_dessiner(cr, w, f, x, y, W, H, r);
    } else {
        tr = transition(w);
        t = (etat == GTK_STATE_NORMAL) ? tr->t : (tr->id ? tr->t : 1.0);
        memo = (etat == GTK_STATE_NORMAL) ? tr->memo : etat;
        if (etat == GTK_STATE_INSENSITIVE) { memo = etat; t = 1; }
        ui_arrondi(cr, x, y, W, H, r);
        if (f->fond[0] >= 0 || f->fond[memo] >= 0) {
            source_mix(cr, f->fond[0], f->fond[memo], t);
            cairo_fill_preserve(cr);
        }
        if (f->bord[0] >= 0) {
            source_mix(cr, f->bord[0], f->bord[memo], t);
            cairo_set_line_width(cr, 1.0);
            ui_arrondi(cr, x + 0.5, y + 0.5, W - 1, H - 1, r);
            cairo_stroke(cr);
        }
        cairo_new_path(cr);
        /* repère de focus clavier sur les boutons (remplace le pointillé GTK) */
        if (GTK_IS_BUTTON(w) && gtk_widget_has_focus(w)) {
            ui_source(cr, C_OR, 0.9);
            cairo_set_line_width(cr, 1.5);
            ui_arrondi(cr, x - 1.5, y - 1.5, W + 3, H + 3, r + 1.5);
            cairo_stroke(cr);
        }
    }
    cairo_destroy(cr);

    if (GTK_IS_BUTTON(w)) {             /* bouton : on dessine son texte et on bloque le dessin GTK */
        GtkWidget *enfant = gtk_bin_get_child(GTK_BIN(w));
        if (enfant) gtk_container_propagate_expose(GTK_CONTAINER(w), enfant, e);
        return TRUE;
    }
    return FALSE;                        /* conteneur : GTK dessine ensuite les enfants par-dessus */
}

static void habiller(GtkWidget *w)
{
    if (g_object_get_data(G_OBJECT(w), "habille")) return;
    g_object_set_data(G_OBJECT(w), "habille", GINT_TO_POINTER(1));
    g_signal_connect(w, "expose-event", G_CALLBACK(forme_expose), NULL);
    g_signal_connect(w, "state-changed", G_CALLBACK(etat_change), NULL);
}

void ui_renommer(GtkWidget *w, const char *nom)
{
    gtk_widget_set_name(w, nom);
    gtk_widget_reset_rc_styles(w);
    gtk_widget_queue_draw(w);
}

void ui_bouton_actif(GtkWidget *b, gboolean actif)
{
    const char *nom_normal = g_object_get_data(G_OBJECT(b), "nom-normal");
    if (nom_normal == NULL) {
        nom_normal = g_strdup(gtk_widget_get_name(b));
        g_object_set_data_full(G_OBJECT(b), "nom-normal", (gpointer) nom_normal, g_free);
    }
    g_object_set_data(G_OBJECT(b), "off", GINT_TO_POINTER(!actif));
    ui_renommer(b, actif ? nom_normal : "ca-btn-off");
}

gboolean ui_bouton_est_off(GtkWidget *b) { return GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "off")); }

void ui_bouton_style(GtkWidget *b, const char *style)
{
    gtk_widget_set_name(b, style);
    habiller(b);
}

/* =====================================================================
 * 4. WIDGETS SPÉCIAUX
 * ===================================================================== */

/* --- cases à cocher et boutons radio : indicateur dessiné --- */
static gboolean coche_expose(GtkWidget *w, GdkEventExpose *e, gpointer d)
{
    GtkAllocation a;
    cairo_t *cr = gdk_cairo_create(e->window);
    gboolean actif = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
    gboolean survol = gtk_widget_get_state(w) == GTK_STATE_PRELIGHT;
    gboolean radio = GTK_IS_RADIO_BUTTON(w);
    double x, y, s = 18;
    GtkWidget *enfant;

    gtk_widget_get_allocation(w, &a);
    x = a.x + 2;
    y = a.y + (a.height - s) / 2;
    if (radio) cairo_arc(cr, x + s / 2, y + s / 2, s / 2 - 0.5, 0, 2 * M_PI);
    else ui_arrondi(cr, x + 0.5, y + 0.5, s - 1, s - 1, 5);
    ui_source(cr, actif && !radio ? C_ACCENT : C_CHAMP, 1);
    cairo_fill_preserve(cr);
    ui_source(cr, actif || survol ? C_ACCENT : C_TEXTE3, 1);
    cairo_set_line_width(cr, 1.4);
    cairo_stroke(cr);
    if (actif && radio) {
        ui_source(cr, C_ACCENT, 1);
        cairo_arc(cr, x + s / 2, y + s / 2, 4.5, 0, 2 * M_PI);
        cairo_fill(cr);
    } else if (actif) {
        ui_source(cr, C_SUR_ACCENT, 1);
        cairo_set_line_width(cr, 2.2);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_move_to(cr, x + 4.5, y + 9.5);
        cairo_line_to(cr, x + 7.8, y + 12.8);
        cairo_line_to(cr, x + 13.5, y + 5.8);
        cairo_stroke(cr);
    }
    cairo_destroy(cr);
    enfant = gtk_bin_get_child(GTK_BIN(w));
    if (enfant) gtk_container_propagate_expose(GTK_CONTAINER(w), enfant, e);
    return TRUE;
}

/* --- barre de progression arrondie (on garde le vrai GtkProgressBar de Glade) --- */
static gboolean jauge_expose(GtkWidget *w, GdkEventExpose *e, gpointer d)
{
    GtkAllocation a;
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(w));
    double f = gtk_progress_bar_get_fraction(GTK_PROGRESS_BAR(w)), h, larg;
    gboolean or = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "or"));
    cairo_pattern_t *pat;
    double r1, g1, b1, r2, g2, b2;

    gtk_widget_get_allocation(w, &a);
    ui_source(cr, C_SURFACE, 1);
    cairo_paint(cr);
    h = a.height;
    ui_arrondi(cr, 0, 0, a.width, h, h / 2);
    ui_source(cr, C_SURFACE2, 1);
    cairo_fill(cr);
    if (f > 0.001) {
        larg = MAX(h, a.width * f);
        if (or) {
            ui_rgb(C_OR, &r1, &g1, &b1);
            r2 = MIN(1, r1 + 0.18); g2 = MIN(1, g1 + 0.16); b2 = MIN(1, b1 + 0.12);
        } else {
            ui_rgb(C_ACCENT_A, &r1, &g1, &b1);
            ui_rgb(C_ACCENT_H, &r2, &g2, &b2);
        }
        pat = cairo_pattern_create_linear(0, 0, larg, 0);
        cairo_pattern_add_color_stop_rgb(pat, 0, r1, g1, b1);
        cairo_pattern_add_color_stop_rgb(pat, 1, r2, g2, b2);
        ui_arrondi(cr, 0, 0, larg, h, h / 2);
        cairo_set_source(cr, pat);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
    }
    cairo_destroy(cr);
    return TRUE;
}

/* --- en-têtes du tableau : à plat avec un filet --- */
static gboolean entete_expose(GtkWidget *w, GdkEventExpose *e, gpointer d)
{
    GtkAllocation a;
    cairo_t *cr = gdk_cairo_create(e->window);
    GtkWidget *enfant = gtk_bin_get_child(GTK_BIN(w));
    gtk_widget_get_allocation(w, &a);
    ui_source(cr, C_SURFACE, 1);
    cairo_rectangle(cr, a.x, a.y, a.width, a.height);
    cairo_fill(cr);
    ui_source(cr, C_LIGNE, 1);
    cairo_rectangle(cr, a.x, a.y + a.height - 1, a.width, 1);
    cairo_fill(cr);
    cairo_destroy(cr);
    if (enfant) gtk_container_propagate_expose(GTK_CONTAINER(w), enfant, e);
    return TRUE;
}

void ui_colonne(GtkTreeView *tv, const char *titre, int col, int couleur, gfloat x, gboolean gras)
{
    GtkCellRenderer *r = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *c;
    g_object_set(r, "xalign", x, NULL);
    if (gras) g_object_set(r, "weight", PANGO_WEIGHT_BOLD, NULL);
    c = gtk_tree_view_column_new_with_attributes(titre, r, "text", col, NULL);
    if (couleur >= 0) gtk_tree_view_column_add_attribute(c, r, "foreground", couleur);
    gtk_tree_view_column_set_resizable(c, TRUE);
    gtk_tree_view_append_column(tv, c);
    if (c->button) g_signal_connect(c->button, "expose-event", G_CALLBACK(entete_expose), NULL);
}

/* --- liste déroulante dans une boîte arrondie : on efface son bouton interne --- */
static gboolean transparent_expose(GtkWidget *w, GdkEventExpose *e, gpointer d)
{
    GtkWidget *enfant = gtk_bin_get_child(GTK_BIN(w));
    if (enfant) gtk_container_propagate_expose(GTK_CONTAINER(w), enfant, e);
    return TRUE;
}

static void combo_interne(GtkWidget *w, gpointer d)
{
    if (GTK_IS_SEPARATOR(w)) { gtk_widget_set_no_show_all(w, TRUE); gtk_widget_hide(w); return; }
    if (GTK_IS_TOGGLE_BUTTON(w)) g_signal_connect(w, "expose-event", G_CALLBACK(transparent_expose), NULL);
    if (GTK_IS_CONTAINER(w)) gtk_container_forall(GTK_CONTAINER(w), combo_interne, NULL);
}

/* --- champs de saisie dans une boîte arrondie, avec focus et erreur --- */
static gboolean saisie_focus(GtkWidget *w, GdkEventFocus *e, gpointer boite)
{
    g_object_set_data(G_OBJECT(boite), "focus", GINT_TO_POINTER(e->in));
    gtk_widget_queue_draw(GTK_WIDGET(boite));
    return FALSE;
}

static void saisie_change(GtkEditable *e, gpointer boite)
{
    if (g_object_get_data(G_OBJECT(boite), "erreur")) {
        g_object_set_data(G_OBJECT(boite), "erreur", NULL);
        gtk_widget_queue_draw(GTK_WIDGET(boite));
    }
}

static void champ_preparer(GtkWidget *boite)
{
    GtkWidget *enfant = gtk_bin_get_child(GTK_BIN(boite));
    if (enfant == NULL) return;
    if (GTK_IS_ENTRY(enfant)) {                       /* GtkEntry et GtkSpinButton */
        gtk_entry_set_has_frame(GTK_ENTRY(enfant), FALSE);
        g_signal_connect(enfant, "focus-in-event", G_CALLBACK(saisie_focus), boite);
        g_signal_connect(enfant, "focus-out-event", G_CALLBACK(saisie_focus), boite);
        g_signal_connect(enfant, "changed", G_CALLBACK(saisie_change), boite);
    }
    if (GTK_IS_COMBO_BOX(enfant)) gtk_container_forall(GTK_CONTAINER(enfant), combo_interne, NULL);
    g_object_set_data(G_OBJECT(enfant), "boite", boite);
}

/* texte d'indication affiché tant que le champ est vide (GTK 2 n'a pas de « placeholder ») */
static gboolean indice_expose(GtkWidget *w, GdkEventExpose *e, gpointer txt)
{
    cairo_t *cr;
    if (gtk_entry_get_text(GTK_ENTRY(w))[0] != '\0') return FALSE;
    if (e->window == gtk_widget_get_window(w)) return FALSE;     /* seulement la zone de texte */
    cr = gdk_cairo_create(e->window);
    ui_source(cr, C_TEXTE3, 1);
    ui_texte(cr, "DejaVu Sans 10", txt, 9, 7, 0);
    cairo_destroy(cr);
    return FALSE;
}

void ui_indice(GtkWidget *entry, const char *txt)
{
    g_signal_connect_after(entry, "expose-event", G_CALLBACK(indice_expose), (gpointer) txt);
}

void ui_marquer_erreur(GtkWidget *entry)
{
    GtkWidget *b = g_object_get_data(G_OBJECT(entry), "boite");
    if (b) { g_object_set_data(G_OBJECT(b), "erreur", GINT_TO_POINTER(1)); gtk_widget_queue_draw(b); }
    gtk_widget_grab_focus(entry);
}

/* --- survol des boîtes invisibles (menu, onglets, ☰, bascule) : on change leur ÉTAT --- */
static gboolean survol_entree(GtkWidget *w, GdkEventCrossing *e, gpointer d)
{
    GtkWidget *decaler = g_object_get_data(G_OBJECT(w), "decaler");
    GdkCursor *main_curseur;
    if (e->detail == GDK_NOTIFY_INFERIOR) return FALSE;
    main_curseur = gdk_cursor_new(GDK_HAND2);            /* e->window = fenêtre d'entrée de la boîte */
    gdk_window_set_cursor(e->window, main_curseur);
    gdk_cursor_unref(main_curseur);
    if (gtk_widget_get_state(w) != GTK_STATE_SELECTED) gtk_widget_set_state(w, GTK_STATE_PRELIGHT);
    if (decaler) gtk_misc_set_padding(GTK_MISC(decaler), 5, 0);      /* le nom glisse de 5 px */
    return FALSE;
}

static gboolean survol_sortie(GtkWidget *w, GdkEventCrossing *e, gpointer d)
{
    GtkWidget *decaler = g_object_get_data(G_OBJECT(w), "decaler");
    if (e->detail == GDK_NOTIFY_INFERIOR) return FALSE;
    if (gtk_widget_get_state(w) != GTK_STATE_SELECTED) gtk_widget_set_state(w, GTK_STATE_NORMAL);
    if (decaler) gtk_misc_set_padding(GTK_MISC(decaler), 0, 0);
    return FALSE;
}

static void survolable(GtkWidget *eb)
{
    gtk_widget_add_events(eb, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK);
    g_signal_connect(eb, "enter-notify-event", G_CALLBACK(survol_entree), NULL);
    g_signal_connect(eb, "leave-notify-event", G_CALLBACK(survol_sortie), NULL);
}

/* --- élément du menu : pastille arrondie + badge carré dessinés en cairo --- */
static gboolean nav_expose(GtkWidget *w, GdkEventExpose *e, gpointer d)
{
    GtkWidget *badge = g_object_get_data(G_OBJECT(w), "badge");
    int etat = gtk_widget_get_state(w);
    GtkAllocation a;
    cairo_t *cr;
    double s = 32;

    forme_expose(w, e, NULL);                 /* pastille de fond (avec transition) */
    if (badge == NULL) return FALSE;
    gtk_widget_get_allocation(badge, &a);
    cr = gdk_cairo_create(e->window);
    ui_arrondi(cr, a.x + (a.width - s) / 2, a.y + (a.height - s) / 2, s, s, 9);
    ui_source(cr, etat == GTK_STATE_SELECTED ? C_ACCENT : C_SURFACE2, 1);
    cairo_fill(cr);
    cairo_destroy(cr);
    return FALSE;                             /* GTK dessine ensuite le code (PA, VO...) par-dessus */
}

/* --- avatar rond avec les initiales (stockées dans la donnée "initiales") --- */
static gboolean avatar_expose(GtkWidget *w, GdkEventExpose *e, gpointer d)
{
    GtkAllocation a;
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(w));
    const char *ini = g_object_get_data(G_OBJECT(w), "initiales");
    gtk_widget_get_allocation(w, &a);
    ui_source(cr, C_SURFACE, 1);
    cairo_paint(cr);
    cairo_arc(cr, a.width / 2.0, a.height / 2.0, a.height / 2.0 - 1, 0, 2 * M_PI);
    ui_source(cr, C_ACCENT, 1);
    cairo_fill_preserve(cr);
    ui_source(cr, C_OR, 1);
    cairo_set_line_width(cr, 1.5);
    cairo_stroke(cr);
    if (ini) {
        gchar *m = g_markup_escape_text(ini, -1);
        ui_source(cr, C_SUR_ACCENT, 1);
        ui_texte(cr, "DejaVu Sans Bold 9", m, a.width / 2.0, a.height / 2.0, 2);
        g_free(m);
    }
    cairo_destroy(cr);
    return TRUE;
}

/* --- pastille « ! » de la fenêtre Compte suspendu --- */
static gboolean alerte_expose(GtkWidget *w, GdkEventExpose *e, gpointer d)
{
    GtkAllocation a;
    cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(w));
    gtk_widget_get_allocation(w, &a);
    ui_source(cr, C_SURFACE, 1);
    cairo_paint(cr);
    cairo_arc(cr, a.width / 2.0, a.height / 2.0, a.height / 2.0 - 1, 0, 2 * M_PI);
    ui_source(cr, C_ERREUR_F, 1);
    cairo_fill(cr);
    ui_source(cr, C_ERREUR, 1);
    ui_texte(cr, "DejaVu Serif Bold 20", "!", a.width / 2.0, a.height / 2.0, 2);
    cairo_destroy(cr);
    return TRUE;
}

/* --- petites capitales espacées (surtitres, marque) --- */
static void espacer(GtkWidget *l, const char *texte, int espacement)
{
    gchar *m = g_markup_printf_escaped("<span letter_spacing='%d'>%s</span>", espacement, texte);
    gtk_label_set_markup(GTK_LABEL(l), m);
    g_free(m);
}

void ui_surtitre(GtkWidget *l, const char *texte) { espacer(l, texte, 1300); }

static void bascule_label_detruit(GtkWidget *l, gpointer d) { bascules = g_slist_remove(bascules, l); }

/* =====================================================================
 * 5. TABLE DES RÔLES + ui_habiller()
 * ===================================================================== */

enum { R_NOM, R_FORME, R_CHAMP, R_COCHE, R_JAUGE, R_JAUGE_OR, R_NAV, R_SURVOL, R_ONGLET, R_BASCULE,
       R_AVATAR, R_ALERTE, R_SUR, R_MARQUE, R_OR };

typedef struct { const char *role, *style; int action; } Role;

/*  rôle (2e mot du nom Glade)   style GTK              ce qui est branché */
static const Role ROLES[] = {
    /* textes (GtkLabel) */
    {"titre",    "ca-titre",        R_NOM},      /* grand titre serif            */
    {"stitre",   "ca-titre-moyen",  R_NOM},      /* titre de section serif       */
    {"titreerr", "ca-titre-err",    R_NOM},      /* titre rouge                  */
    {"chiffre",  "ca-chiffre",      R_NOM},      /* grand nombre (compteurs)     */
    {"marque",   "ca-marque",       R_MARQUE},   /* CARTHAGE AIRWAYS espacé      */
    {"t2",       "ca-t2",           R_NOM},      /* texte secondaire             */
    {"t3",       "ca-t3",           R_NOM},      /* petite légende               */
    {"sur",      "ca-surtitre",     R_SUR},      /* SURTITRE EN CAPITALES        */
    {"val",      "ca-valeur",       R_NOM},      /* valeur en gras               */
    {"acc",      "ca-accent-txt",   R_NOM},      /* texte couleur d'accent       */
    {"err",      "ca-erreur",       R_NOM},      /* message d'erreur             */
    {"mono",     "ca-mono",         R_NOM},      /* texte à chasse fixe          */
    {"or",       "ca-or",           R_OR},       /* texte doré (panneau connexion) */
    {"blanc",    "ca-blanc",        R_NOM},
    {"blancp",   "ca-blancp",       R_NOM},
    {"badge",    "ca-nav-badge",    R_NOM},      /* code PA, VO... du menu       */
    {"navtxt",   "ca-nav-nom",      R_NOM},
    {"ongtxt",   "ca-onglet-txt",   R_NOM},
    {"icotxt",   "ca-icone-txt",    R_NOM},
    /* conteneurs dessinés */
    {"carte",    "ca-carte",        R_FORME},    /* GtkAlignment : carte blanche + ombre */
    {"champ",    "ca-champ-boite",  R_CHAMP},    /* GtkAlignment autour d'un GtkEntry/Spin/Combo */
    {"btnp",     "ca-btn-primaire", R_FORME},    /* GtkButton : pilule bordeaux  */
    {"btnf",     "ca-btn-fantome",  R_FORME},    /* GtkButton : sans fond        */
    {"btnd",     "ca-btn-danger",   R_FORME},    /* GtkButton : rouge            */
    {"btn",      "ca-btn",          R_FORME},    /* GtkButton : secondaire       */
    {"chipa",    "ca-chip-accent",  R_FORME},    /* GtkEventBox : pastille       */
    {"chipok",   "ca-chip-ok",      R_FORME},
    {"chiperr",  "ca-chip-err",     R_FORME},
    {"chip",     "ca-chip",         R_FORME},
    {"toast",    "ca-toast-ok",     R_FORME},    /* message temporaire           */
    {"palier",   "ca-palier",       R_FORME},
    {"nav",      "ca-nav",          R_NAV},      /* GtkEventBox : élément du menu ☰ */
    {"icone",    "ca-icone",        R_SURVOL},   /* GtkEventBox : bouton ☰       */
    {"bascule",  "ca-bascule",      R_BASCULE},  /* GtkEventBox : ☀ Jour / ☾ Nuit */
    {"onglet",   "ca-onglet",       R_ONGLET},   /* GtkEventBox : onglet segmenté */
    {"topbar",   "ca-topbar",       R_NOM},      /* GtkEventBox : fond « surface » */
    {"sidebar",  "ca-sidebar",      R_NOM},
    {"surface",  "ca-sidebar",      R_NOM},
    {"ligne",    "ca-ligne",        R_NOM},      /* GtkEventBox de 1 px : filet  */
    {"table",    "ca-table",        R_NOM},      /* GtkTreeView                  */
    {"coche",    "ca-coche",        R_COCHE},    /* GtkCheckButton / GtkRadioButton */
    {"jauge",    "ca-jauge",        R_JAUGE},    /* GtkProgressBar bordeaux      */
    {"jaugeor",  "ca-jauge",        R_JAUGE_OR}, /* GtkProgressBar dorée         */
    {"avatar",   "ca-avatar",       R_AVATAR},   /* GtkDrawingArea : initiales   */
    {"alerte",   "ca-sur-carte",    R_ALERTE},   /* GtkDrawingArea : « ! »       */
    {"cartefid", "ca-sur-fond",     R_NOM},      /* GtkDrawingArea : carte de fidélité (passager_espace.c) */
};

GtkWidget *ui_w(GtkWidget *fenetre, const char *nom) { return lookup_widget(fenetre, nom); }

/* rôle = texte entre le 1er et le 2e '_' du nom Glade */
static const Role *role_de(const char *nom_glade)
{
    const char *d = strchr(nom_glade, '_'), *f;
    size_t i, n;
    if (d == NULL) return NULL;
    d++;
    f = strchr(d, '_');
    n = f ? (size_t) (f - d) : strlen(d);
    for (i = 0; i < G_N_ELEMENTS(ROLES); i++)
        if (strlen(ROLES[i].role) == n && strncmp(ROLES[i].role, d, n) == 0) return &ROLES[i];
    return NULL;
}

/* -- Si l'option « Définir les noms des widgets » était décochée dans Glade, les widgets
 *    n'ont pas de nom : on le retrouve grâce à GLADE_HOOKUP_OBJECT (nom -> widget). -- */
static void lister(GtkWidget *w, gpointer ens)
{
    g_hash_table_insert(ens, w, w);
    if (GTK_IS_CONTAINER(w)) gtk_container_forall(GTK_CONTAINER(w), lister, ens);
}

typedef struct { GHashTable *ens; GSList *a_nommer; } Collecte;

static void collecter(GQuark cle, gpointer valeur, gpointer data)
{
    Collecte *c = data;
    if (valeur != NULL && g_hash_table_lookup(c->ens, valeur) != NULL) {
        c->a_nommer = g_slist_prepend(c->a_nommer, valeur);
        c->a_nommer = g_slist_prepend(c->a_nommer, (gpointer) g_quark_to_string(cle));
    }
}

static void nommer_depuis_glade(GtkWidget *fenetre)
{
    Collecte c;
    GSList *l;
    c.ens = g_hash_table_new(g_direct_hash, g_direct_equal);
    c.a_nommer = NULL;
    lister(fenetre, c.ens);
    g_datalist_foreach(&G_OBJECT(fenetre)->qdata, collecter, &c);
    for (l = c.a_nommer; l != NULL && l->next != NULL; l = l->next->next) {
        const char *nom = l->data;
        GtkWidget *w = l->next->data;
        if (strcmp(gtk_widget_get_name(w), G_OBJECT_TYPE_NAME(w)) == 0) gtk_widget_set_name(w, nom);
    }
    g_slist_free(c.a_nommer);
    g_hash_table_destroy(c.ens);
}

/* 1er passage : mémorise le nom Glade de chaque widget (avant qu'on le remplace par le style) */
static void memoriser(GtkWidget *w, gpointer d)
{
    if (g_object_get_data(G_OBJECT(w), "nom-glade") == NULL)
        g_object_set_data_full(G_OBJECT(w), "nom-glade", g_strdup(gtk_widget_get_name(w)), g_free);
    if (GTK_IS_CONTAINER(w)) gtk_container_forall(GTK_CONTAINER(w), memoriser, NULL);
}

/* cherche sous w le 1er widget dont le rôle Glade est `role` */
static GtkWidget *sous_role(GtkWidget *w, const char *role)
{
    GList *enfants, *l;
    GtkWidget *trouve = NULL;
    const Role *r;
    if (!GTK_IS_CONTAINER(w)) return NULL;
    enfants = gtk_container_get_children(GTK_CONTAINER(w));
    for (l = enfants; l != NULL && trouve == NULL; l = l->next) {
        r = role_de(g_object_get_data(G_OBJECT(l->data), "nom-glade") ? g_object_get_data(G_OBJECT(l->data), "nom-glade") : "");
        if (r != NULL && strcmp(r->role, role) == 0) trouve = l->data;
        else trouve = sous_role(l->data, role);
    }
    g_list_free(enfants);
    return trouve;
}

/* 2e passage : applique le rôle */
static void appliquer(GtkWidget *w, gpointer d)
{
    const char *nom = g_object_get_data(G_OBJECT(w), "nom-glade");
    const Role *r = nom ? role_de(nom) : NULL;
    GtkWidget *x;

    if (r != NULL && !GTK_IS_WINDOW(w)) {
        if (GTK_IS_LABEL(w)) {
            gchar *txt = g_strdup(gtk_label_get_text(GTK_LABEL(w)));
            gtk_widget_set_name(w, r->style);
            if (r->action == R_SUR) espacer(w, txt, 1300);
            if (r->action == R_MARQUE) espacer(w, txt, 2200);
            if (r->action == R_OR) espacer(w, txt, 3000);
            g_free(txt);
        } else {
            gtk_widget_set_name(w, r->style);
            switch (r->action) {
            case R_FORME:
                habiller(w);
                break;
            case R_CHAMP:
                habiller(w);
                champ_preparer(w);
                break;
            case R_COCHE:
                g_signal_connect(w, "expose-event", G_CALLBACK(coche_expose), NULL);
                break;
            case R_JAUGE_OR:
                g_object_set_data(G_OBJECT(w), "or", GINT_TO_POINTER(1));
                /* fall through : même dessin, en doré */
            case R_JAUGE:
                g_signal_connect(w, "expose-event", G_CALLBACK(jauge_expose), NULL);
                break;
            case R_NAV:
                g_object_set_data(G_OBJECT(w), "habille", GINT_TO_POINTER(1));
                g_object_set_data(G_OBJECT(w), "badge", sous_role(w, "badge"));
                g_object_set_data(G_OBJECT(w), "decaler", sous_role(w, "navtxt"));
                g_signal_connect(w, "expose-event", G_CALLBACK(nav_expose), NULL);
                g_signal_connect(w, "state-changed", G_CALLBACK(etat_change), NULL);
                survolable(w);
                break;
            case R_SURVOL:
                habiller(w);
                survolable(w);
                break;
            case R_ONGLET:
                survolable(w);
                break;
            case R_BASCULE:
                habiller(w);
                survolable(w);
                gtk_widget_set_tooltip_text(w, "Thème Jour / Nuit (Ctrl+T)");
                x = sous_role(w, "val");
                if (x != NULL) {
                    gtk_label_set_text(GTK_LABEL(x), texte_bascule());
                    bascules = g_slist_prepend(bascules, x);
                    g_signal_connect(x, "destroy", G_CALLBACK(bascule_label_detruit), NULL);
                }
                break;
            case R_AVATAR:
                g_signal_connect(w, "expose-event", G_CALLBACK(avatar_expose), NULL);
                break;
            case R_ALERTE:
                g_signal_connect(w, "expose-event", G_CALLBACK(alerte_expose), NULL);
                break;
            default:
                break;
            }
        }
    }
    if (GTK_IS_CONTAINER(w)) gtk_container_forall(GTK_CONTAINER(w), appliquer, NULL);
}

void ui_habiller(GtkWidget *fenetre)
{
    nommer_depuis_glade(fenetre);
    memoriser(fenetre, NULL);
    appliquer(fenetre, NULL);
}
