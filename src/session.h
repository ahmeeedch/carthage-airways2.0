/*
 * session.h - utilisateur connecte (commun a l'equipe)
 * Les variables sont "static" dans session.c : on y accede uniquement
 * par ces fonctions, jamais par extern.
 */
#ifndef SESSION_H
#define SESSION_H

void        session_ouvrir(const char *login, const char *role);
void        session_fermer(void);
const char *session_login(void);   /* pour un passager : login = CIN */
const char *session_role(void);    /* "ADMIN", "PASSAGER", "PERSONNEL" ou "" */

#endif
