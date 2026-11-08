/*
 * chemins.h - emplacement des fichiers de donnees (commun aux 6 modules)
 *
 * Ne PAS appeler ce fichier config.h : autotools genere deja un config.h
 * (AM_CONFIG_HEADER dans configure.in) et support.h fait #include <config.h>.
 * Un src/config.h ecraserait celui d'autotools -> PACKAGE non defini.
 *
 * Les chemins sont relatifs : lancer le programme depuis la racine du
 * projet (cd carthage && ./src/carthage).
 * Le #ifndef permet a un test de rediriger vers un autre fichier :
 *   gcc -DF_PASSAGERS='"tests/p.txt"' ...
 */
#ifndef CHEMINS_H
#define CHEMINS_H

#ifndef F_PASSAGERS
#define F_PASSAGERS "data/passagers.txt"
#endif

#ifndef F_USERS
#define F_USERS "data/users.txt"
#endif

#ifndef F_USERS_TMP
#define F_USERS_TMP "data/users_tmp.txt"
#endif

#ifndef F_TMP
#define F_TMP "data/tmp.txt"
#endif

#endif
