/*
 * connexion.h — fenêtre de connexion (window_connexion) et fenêtre Compte suspendu
 */
#ifndef CONNEXION_H
#define CONNEXION_H

#include <gtk/gtk.h>

void connexion_ouvrir(void);      /* au démarrage (main.c) */
void connexion_revenir(void);     /* après une déconnexion */
void connexion_valider(void);     /* bouton « Se connecter » ou Entrée dans le mot de passe */
void connexion_focus_mdp(void);   /* Entrée dans l'identifiant */
void bloque_fermer(void);         /* bouton « Retour à la connexion » de window_compte_bloque */

#endif
