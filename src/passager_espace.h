/*
 * passager_espace.h — module 1 côté passager (fenêtre window_passager) : fidélité + profil
 */
#ifndef PASSAGER_ESPACE_H
#define PASSAGER_ESPACE_H

#include <gtk/gtk.h>
#include "passager.h"

void     espace_ouvrir(const Passager *p);
void     espace_fermer(void);              /* destroy de window_passager */
gboolean espace_touche(GdkEventKey *e);
void     espace_enregistrer_profil(void);

#endif
