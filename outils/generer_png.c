/*
 * outils/generer_png.c — fabrique les images PNG du design « Clair et premium »
 *
 * Les PNG sont DÉJÀ fournis dans pixmaps/. Ce programme ne sert que si tu veux
 * retoucher une couleur ou une forme puis régénérer les images.
 *
 * Depuis la racine du projet :
 *   gcc -Wall outils/generer_png.c -o /tmp/generer_png $(pkg-config --cflags --libs pangocairo) -lm
 *   /tmp/generer_png pixmaps
 *
 * Images produites :
 *   fond_connexion_jour.png / fond_connexion_nuit.png   panneau bordeaux de la connexion (380 x 540)
 *   avion_00.png ... avion_21.png                        l'avion tourné de -70° à +35° (pas de 5°)
 *   carte_standard/silver/gold/platinum.png              fond de la carte de fidélité (400 x 248)
 *   logo_ca.png (64 px, icône des fenêtres) et logo_ca_32.png (barre du haut)
 *   illus_module.png                                     illustration des modules « en développement »
 */
#include <cairo.h>
#include <pango/pangocairo.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *dossier = "pixmaps";

/* ---------- outils ---------- */

static void enregistrer(cairo_surface_t *s, const char *nom)
{
    char chemin[512];
    snprintf(chemin, sizeof chemin, "%s/%s", dossier, nom);
    if (cairo_surface_write_to_png(s, chemin) != CAIRO_STATUS_SUCCESS)
        fprintf(stderr, "Impossible d'écrire %s\n", chemin);
    else
        printf("  %s\n", chemin);
}

static void arrondi(cairo_t *cr, double x, double y, double w, double h, double r)
{
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
    cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI);
    cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
}

static void texte(cairo_t *cr, const char *police, const char *markup, double x, double y)
{
    PangoLayout *ly = pango_cairo_create_layout(cr);
    PangoFontDescription *fd = pango_font_description_from_string(police);
    pango_layout_set_font_description(ly, fd);
    pango_layout_set_markup(ly, markup, -1);
    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, ly);
    pango_font_description_free(fd);
    g_object_unref(ly);
}

/* silhouette de l'avion (vue de dessus), nez vers la droite */
static void avion(cairo_t *cr, double x, double y, double angle, double echelle)
{
    static const double P[][2] = {
        {15, 0}, {11, -1.6}, {3, -1.6}, {-3, -11}, {-6.5, -11}, {-2.5, -1.6}, {-10, -1.6}, {-13.5, -5.5},
        {-15.5, -5.5}, {-13.5, 0}, {-15.5, 5.5}, {-13.5, 5.5}, {-10, 1.6}, {-2.5, 1.6}, {-6.5, 11}, {-3, 11},
        {3, 1.6}, {11, 1.6},
    };
    size_t i;
    cairo_save(cr);
    cairo_translate(cr, x, y);
    cairo_rotate(cr, angle);
    cairo_scale(cr, echelle, echelle);
    cairo_move_to(cr, P[0][0], P[0][1]);
    for (i = 1; i < sizeof P / sizeof P[0]; i++) cairo_line_to(cr, P[i][0], P[i][1]);
    cairo_close_path(cr);
    cairo_restore(cr);
}

/* même courbe que dans src/connexion.c (Bézier quadratique) */
static void point_courbe(double t, double W, double H, double *x, double *y)
{
    double x0 = 34, y0 = H - 150, x1 = W * 0.50, y1 = H * 0.12, x2 = W - 44, y2 = 150, u = 1 - t;
    *x = u * u * x0 + 2 * u * t * x1 + t * t * x2;
    *y = u * u * y0 + 2 * u * t * y1 + t * t * y2;
}

/* ---------- 1. fonds du panneau de connexion ---------- */

static void fond_connexion(int nuit, const char *nom)
{
    const double W = 380, H = 540;
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_RGB24, (int) W, (int) H);
    cairo_t *cr = cairo_create(s);
    cairo_pattern_t *pat = cairo_pattern_create_linear(0, 0, W * 0.6, H);
    static const double tirets[] = {2.0, 7.0};
    double x, y, t;
    int i;

    if (!nuit) {
        cairo_pattern_add_color_stop_rgb(pat, 0, 0.60, 0.13, 0.19);
        cairo_pattern_add_color_stop_rgb(pat, 1, 0.29, 0.06, 0.10);
    } else {
        cairo_pattern_add_color_stop_rgb(pat, 0, 0.36, 0.08, 0.12);
        cairo_pattern_add_color_stop_rgb(pat, 1, 0.12, 0.03, 0.05);
    }
    cairo_set_source(cr, pat);
    cairo_paint(cr);
    cairo_pattern_destroy(pat);

    cairo_set_line_width(cr, 1.0);                       /* cercles dorés très discrets */
    for (i = 1; i <= 6; i++) {
        cairo_set_source_rgba(cr, 0.85, 0.72, 0.45, 0.07);
        cairo_arc(cr, W * 0.92, H * 0.18, 55.0 * i, 0, 2 * M_PI);
        cairo_stroke(cr);
    }
    cairo_set_line_width(cr, 1.6);                       /* trajectoire complète, pâle */
    cairo_set_dash(cr, tirets, 2, 0);
    for (t = 0; t <= 1.0001; t += 0.02) {
        point_courbe(t, W, H, &x, &y);
        if (t == 0) cairo_move_to(cr, x, y); else cairo_line_to(cr, x, y);
    }
    cairo_set_source_rgba(cr, 0.92, 0.80, 0.55, 0.28);
    cairo_stroke(cr);
    cairo_set_dash(cr, NULL, 0, 0);
    point_courbe(0, W, H, &x, &y);                       /* point de départ (Tunis) */
    cairo_arc(cr, x, y, 4, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, 0.92, 0.80, 0.55, 0.9);
    cairo_fill(cr);
    point_courbe(1, W, H, &x, &y);                       /* point d'arrivée, en creux */
    cairo_arc(cr, x, y, 4, 0, 2 * M_PI);
    cairo_set_line_width(cr, 1.4);
    cairo_set_source_rgba(cr, 0.92, 0.80, 0.55, 0.55);
    cairo_stroke(cr);

    cairo_destroy(cr);
    enregistrer(s, nom);
    cairo_surface_destroy(s);
}

/* ---------- 2. images de l'avion (une par angle) ---------- */

static void images_avion(void)
{
    int i;
    char nom[32];
    for (i = 0; i < 22; i++) {
        cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 44, 44);
        cairo_t *cr = cairo_create(s);
        double angle = (-70 + 5 * i) * M_PI / 180.0;
        avion(cr, 22, 23.2, angle, 1.15);                /* ombre légère sous l'avion */
        cairo_set_source_rgba(cr, 0.10, 0.02, 0.04, 0.25);
        cairo_fill(cr);
        avion(cr, 22, 22, angle, 1.15);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_fill(cr);
        cairo_destroy(cr);
        snprintf(nom, sizeof nom, "avion_%02d.png", i);
        enregistrer(s, nom);
        cairo_surface_destroy(s);
    }
}

/* ---------- 3. cartes de fidélité ---------- */

static void carte(int nv, const char *nom)
{
    /* haut-gauche, bas-droite, couleur du texte (identique à src/passager_espace.c) */
    static const double G[4][9] = {
        {0.95, 0.93, 0.90,  0.84, 0.80, 0.75,  0.16, 0.13, 0.14},   /* Standard : ivoire */
        {0.92, 0.93, 0.95,  0.66, 0.70, 0.75,  0.12, 0.13, 0.16},   /* Silver */
        {0.93, 0.82, 0.56,  0.72, 0.54, 0.23,  0.17, 0.11, 0.04},   /* Gold */
        {0.33, 0.09, 0.13,  0.13, 0.04, 0.06,  0.96, 0.91, 0.82},   /* Platinum : bordeaux profond */
    };
    const double W = 390, H = 236;                       /* carte dans une image de 400 x 248 */
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 400, 248);
    cairo_t *cr = cairo_create(s);
    cairo_pattern_t *pat;
    int i;

    for (i = 5; i >= 1; i--) {                           /* ombre douce (transparente) */
        cairo_set_source_rgba(cr, 0.20, 0.08, 0.10, 0.035);
        arrondi(cr, 5 - i * 0.5, 4 + i * 0.9, W + i, H + i * 0.4, 18 + i * 0.5);
        cairo_fill(cr);
    }
    arrondi(cr, 5, 4, W, H, 18);
    pat = cairo_pattern_create_linear(0, 0, W, H);
    cairo_pattern_add_color_stop_rgb(pat, 0, G[nv][0], G[nv][1], G[nv][2]);
    cairo_pattern_add_color_stop_rgb(pat, 1, G[nv][3], G[nv][4], G[nv][5]);
    cairo_set_source(cr, pat);
    cairo_fill_preserve(cr);
    cairo_pattern_destroy(pat);
    cairo_save(cr);
    cairo_clip(cr);
    cairo_set_line_width(cr, 1.0);
    for (i = 1; i <= 7; i++) {                           /* arcs fins façon guillochage */
        cairo_set_source_rgba(cr, G[nv][6], G[nv][7], G[nv][8], 0.06);
        cairo_arc(cr, W * 1.02, H * 1.1, 46.0 * i, 0, 2 * M_PI);
        cairo_stroke(cr);
    }
    cairo_restore(cr);
    cairo_set_source_rgba(cr, 0.84, 0.70, 0.42, 0.85);   /* filet doré */
    cairo_set_line_width(cr, 1.2);
    arrondi(cr, 12, 11, W - 14, H - 14, 13);
    cairo_stroke(cr);

    /* puce dorée (décor) */
    arrondi(cr, 28, 84, 34, 26, 5);
    pat = cairo_pattern_create_linear(28, 84, 62, 110);
    cairo_pattern_add_color_stop_rgb(pat, 0, 0.93, 0.82, 0.55);
    cairo_pattern_add_color_stop_rgb(pat, 1, 0.71, 0.55, 0.26);
    cairo_set_source(cr, pat);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
    cairo_set_source_rgba(cr, 0.45, 0.32, 0.10, 0.5);
    cairo_set_line_width(cr, 0.8);
    cairo_move_to(cr, 28, 97); cairo_line_to(cr, 62, 97);
    cairo_move_to(cr, 45, 84); cairo_line_to(cr, 45, 110);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, G[nv][6], G[nv][7], G[nv][8]);
    texte(cr, "DejaVu Serif Bold 11", "<span letter_spacing='2200'>CARTHAGE AIRWAYS</span>", 28, 26);

    cairo_destroy(cr);
    enregistrer(s, nom);
    cairo_surface_destroy(s);
}

/* ---------- 4. logo ---------- */

static void logo(int taille, const char *nom)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, taille, taille);
    cairo_t *cr = cairo_create(s);
    double c = taille / 2.0, r = taille / 2.0 - 1;
    cairo_pattern_t *pat = cairo_pattern_create_linear(0, 0, taille, taille);
    cairo_pattern_add_color_stop_rgb(pat, 0, 0.64, 0.15, 0.23);
    cairo_pattern_add_color_stop_rgb(pat, 1, 0.36, 0.07, 0.12);
    cairo_arc(cr, c, c, r, 0, 2 * M_PI);
    cairo_set_source(cr, pat);
    cairo_fill(cr);
    cairo_pattern_destroy(pat);
    cairo_arc(cr, c, c, r - taille * 0.06, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, 0.85, 0.72, 0.45, 0.95);
    cairo_set_line_width(cr, taille * 0.035);
    cairo_stroke(cr);
    avion(cr, c, c, -M_PI / 4, taille / 44.0);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_fill(cr);
    cairo_destroy(cr);
    enregistrer(s, nom);
    cairo_surface_destroy(s);
}

/* ---------- 5. illustration des modules réservés ---------- */

static void illustration(void)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 168, 96);
    cairo_t *cr = cairo_create(s);
    static const double tirets[] = {3.0, 6.0};
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(cr, 1.6);
    cairo_set_dash(cr, tirets, 2, 0);
    cairo_move_to(cr, 16, 78);
    cairo_curve_to(cr, 50, 8, 118, 8, 152, 62);
    cairo_set_source_rgba(cr, 0.72, 0.57, 0.29, 0.75);
    cairo_stroke(cr);
    cairo_set_dash(cr, NULL, 0, 0);
    cairo_arc(cr, 16, 78, 5, 0, 2 * M_PI);
    cairo_fill(cr);
    cairo_arc(cr, 152, 62, 5, 0, 2 * M_PI);
    cairo_set_line_width(cr, 1.6);
    cairo_stroke(cr);
    avion(cr, 96, 25, 0.12, 1.1);
    cairo_set_source_rgba(cr, 0.72, 0.57, 0.29, 0.95);
    cairo_fill(cr);
    cairo_destroy(cr);
    enregistrer(s, "illus_module.png");
    cairo_surface_destroy(s);
}

int main(int argc, char *argv[])
{
    if (argc > 1) dossier = argv[1];
    printf("Images écrites dans %s/ :\n", dossier);
    fond_connexion(0, "fond_connexion_jour.png");
    fond_connexion(1, "fond_connexion_nuit.png");
    images_avion();
    carte(0, "carte_standard.png");
    carte(1, "carte_silver.png");
    carte(2, "carte_gold.png");
    carte(3, "carte_platinum.png");
    logo(64, "logo_ca.png");
    logo(32, "logo_ca_32.png");
    illustration();
    return 0;
}
