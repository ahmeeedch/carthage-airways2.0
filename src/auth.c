#include <stdio.h>
#include <string.h>
#include "auth.h"
#include "chemins.h"

int auth_verifier(const char *login, const char *mdp, char *role)
{
    char ligne[100], l[21], m[21], r[12];
    FILE *f = fopen(F_USERS, "r");
    if (f == NULL) return -1;

    while (fgets(ligne, sizeof ligne, f) != NULL) {
        ligne[strcspn(ligne, "\r\n")] = '\0';
        if (sscanf(ligne, "%20[^;];%20[^;];%11s", l, m, r) != 3) continue;
        if (strcmp(l, login) == 0 && strcmp(m, mdp) == 0) {
            strcpy(role, r);
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

static int login_existe(const char *login)
{
    char ligne[100], l[21];
    int trouve = 0;
    FILE *f = fopen(F_USERS, "r");
    if (f == NULL) return 0;
    while (!trouve && fgets(ligne, sizeof ligne, f) != NULL)
        if (sscanf(ligne, "%20[^;]", l) == 1 && strcmp(l, login) == 0) trouve = 1;
    fclose(f);
    return trouve;
}

int auth_ajouter(const char *login, const char *mdp, const char *role)
{
    FILE *f;
    if (login_existe(login)) return 0;
    f = fopen(F_USERS, "a");
    if (f == NULL) return -1;
    fprintf(f, "%s;%s;%s\n", login, mdp, role);
    fclose(f);
    return 1;
}

/* recopie toutes les lignes sauf celle du login, puis remplace le fichier */
int auth_supprimer(const char *login)
{
    char ligne[100], l[21];
    int trouve = 0;
    FILE *f = fopen(F_USERS, "r"), *t;
    if (f == NULL) return -1;
    t = fopen(F_USERS_TMP, "w");
    if (t == NULL) { fclose(f); return -1; }
    while (fgets(ligne, sizeof ligne, f) != NULL) {
        if (sscanf(ligne, "%20[^;]", l) == 1 && strcmp(l, login) == 0) { trouve = 1; continue; }
        fputs(ligne, t);
    }
    fclose(f);
    fclose(t);
    if (rename(F_USERS_TMP, F_USERS) != 0) return -1;
    return trouve;
}
