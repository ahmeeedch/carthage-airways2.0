/*
 * auth.h - authentification (fonctionnalite COMMUNE a l'equipe, CDC section 7.3)
 * data/users.txt : une ligne par compte  login;mot_de_passe;ROLE
 * ROLE vaut ADMIN, PASSAGER ou PERSONNEL. Pour un passager, login = CIN.
 */
#ifndef AUTH_H
#define AUTH_H

/* 1 si login + mdp corrects (role recopie dans role, 12 octets), 0 sinon, -1 fichier absent */
int auth_verifier(const char *login, const char *mdp, char *role);

/* 1 compte cree, 0 login deja pris, -1 erreur fichier */
int auth_ajouter(const char *login, const char *mdp, const char *role);

/* 1 compte supprime, 0 introuvable, -1 erreur fichier */
int auth_supprimer(const char *login);

#endif
