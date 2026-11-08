/*
 * ui_shell.h — « coquille » commune aux fenêtres Administration et Mon espace :
 * barre du haut + menu latéral ☰ repliable + titre animé + pages (GtkNotebook sans onglets).
 *
 * Les widgets viennent de Glade et suivent tous le même modèle de nom
 * (P = adm pour l'admin, esp pour le passager) :
 *   P_sidebar_menu, P_sur_navigation, P_nav_<code>, P_navtxt_<code>, P_titre_page,
 *   P_t2_soustitre, P_notebook_pages, P_toast_message, P_toasttxt_message,
 *   P_avatar_initiales, P_val_nom
 * Un autre module (Vols, Réservations...) n'a qu'à remplir la page qui lui est réservée.
 */
#ifndef UI_SHELL_H
#define UI_SHELL_H

#include <gtk/gtk.h>
#include "ui_anim.h"

#define SB_LARGE 248
#define SB_ETROIT 76
#define SHELL_MAX 8

typedef struct Shell Shell;
struct Shell {
    GtkWidget *fenetre, *sidebar, *titre, *sous_titre, *pages, *pied;
    GtkWidget *items[SHELL_MAX], *noms[SHELL_MAX];
    char titres[SHELL_MAX][64];
    const char *soustitres[SHELL_MAX];
    Toast toast;
    char initiales[4];
    int n, actif, largeur, a0, a1, etape;
    gboolean replie;
    guint anim;
    void (*montree)(Shell *s, int index);       /* appelé quand une page devient visible */
};

Shell *shell_attacher(GtkWidget *fenetre, const char *prefixe, const char *codes[], const char *soustitres[],
                      int n, const char *nom_affiche, const char *initiales);
Shell *shell_de(GtkWidget *w);                 /* retrouve la coquille depuis n'importe quel widget */
void   shell_montrer(Shell *s);
void   shell_selectionner(Shell *s, int i, gboolean anime);
void   shell_menu_basculer(Shell *s);
gboolean shell_touche(Shell *s, GdkEventKey *e); /* Ctrl+B, Ctrl+T, Ctrl+1..n */
void   shell_liberer(Shell *s);                /* à appeler depuis le signal destroy de la fenêtre */

#endif
