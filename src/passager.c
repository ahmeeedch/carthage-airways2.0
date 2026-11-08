/*
 * passager.c - module 1 : gestion des passagers (couche metier)
 * Aucun #include <gtk/gtk.h> : se teste avec tests/test_passager.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "passager.h"
#include "chemins.h"

/* ================= Format du fichier ================= */

void passager_init(Passager *p)
{
    memset(p, 0, sizeof(Passager));
    p->sexe = 'H';
    strcpy(p->motif, "-");
}

void passager_ecrire_ligne(FILE *f, const Passager *p)
{
    fprintf(f, "%s;%s;%s;%02d/%02d/%04d;%c;%s;%s;%s;%d;%d;%d;%s;%02d/%02d/%04d\n",
            p->cin, p->nom, p->prenom,
            p->naissance.jour, p->naissance.mois, p->naissance.annee,
            p->sexe, p->nationalite, p->tel, p->email,
            p->newsletter, p->points, p->bloque,
            p->motif[0] != '\0' ? p->motif : "-",
            p->date_blocage.jour, p->date_blocage.mois, p->date_blocage.annee);
}

/*
 * Lit la prochaine ligne VALIDE. Une ligne abimee (mauvais nombre de champs)
 * est ignoree au lieu de remplir le struct avec n'importe quoi.
 * Renvoie 1 si un passager a ete lu, 0 en fin de fichier.
 */
int passager_lire_ligne(FILE *f, Passager *p)
{
    char ligne[600];
    int n;

    while (fgets(ligne, sizeof ligne, f) != NULL) {
        ligne[strcspn(ligne, "\r\n")] = '\0';
        if (ligne[0] == '\0') continue;                 /* ligne vide */
        passager_init(p);
        n = sscanf(ligne,
                   "%8[^;];%60[^;];%60[^;];%d/%d/%d;%c;%40[^;];%8[^;];%50[^;];%d;%d;%d;%200[^;];%d/%d/%d",
                   p->cin, p->nom, p->prenom,
                   &p->naissance.jour, &p->naissance.mois, &p->naissance.annee,
                   &p->sexe, p->nationalite, p->tel, p->email,
                   &p->newsletter, &p->points, &p->bloque, p->motif,
                   &p->date_blocage.jour, &p->date_blocage.mois, &p->date_blocage.annee);
        if (n == 17) return 1;
        /* sinon : ligne corrompue, on passe a la suivante */
    }
    return 0;
}

/* ================= CRUD ================= */

int passager_chercher(const char *cin, Passager *out)
{
    Passager p;
    FILE *f = fopen(F_PASSAGERS, "r");
    if (f == NULL) return 0;                    /* pas encore de fichier = personne */
    while (passager_lire_ligne(f, &p)) {
        if (strcmp(p.cin, cin) == 0) {
            if (out != NULL) *out = p;
            fclose(f);
            return 1;
        }
    }
    fclose(f);
    return 0;
}

int passager_existe(const char *cin)
{
    return passager_chercher(cin, NULL);
}

int passager_ajouter(const Passager *p)
{
    FILE *f;
    if (passager_chercher(p->cin, NULL)) return 0;   /* unicite du CIN */
    f = fopen(F_PASSAGERS, "a");
    if (f == NULL) return -1;
    passager_ecrire_ligne(f, p);
    fclose(f);
    return 1;
}

/*
 * Reecrit tout le fichier dans tmp.txt :
 *   remplacant != NULL -> la ligne de ce CIN est remplacee (modifier)
 *   remplacant == NULL -> la ligne de ce CIN est sautee   (supprimer)
 * puis tmp.txt remplace l'original.
 */
static int reecrire(const char *cin, const Passager *remplacant)
{
    Passager p;
    int trouve = 0;
    FILE *f = fopen(F_PASSAGERS, "r");
    FILE *t;

    if (f == NULL) return 0;                    /* fichier absent : rien a modifier */
    t = fopen(F_TMP, "w");
    if (t == NULL) { fclose(f); return -1; }

    while (passager_lire_ligne(f, &p)) {
        if (strcmp(p.cin, cin) == 0) {
            trouve = 1;
            if (remplacant != NULL) passager_ecrire_ligne(t, remplacant);
        } else {
            passager_ecrire_ligne(t, &p);
        }
    }
    fclose(f);
    fclose(t);                                  /* fermer AVANT remove/rename */

    if (!trouve) { remove(F_TMP); return 0; }   /* original intact */
    remove(F_PASSAGERS);
    if (rename(F_TMP, F_PASSAGERS) != 0) return -1;
    return 1;
}

int passager_modifier(const Passager *p)
{
    return reecrire(p->cin, p);
}

int passager_supprimer(const char *cin)
{
    return reecrire(cin, NULL);
}

int passager_charger_tous(Passager t[], int max)
{
    int n = 0;
    FILE *f = fopen(F_PASSAGERS, "r");
    if (f == NULL) return 0;
    while (n < max && passager_lire_ligne(f, &t[n])) n++;
    fclose(f);
    return n;
}

/* ================= Tri et filtre ================= */

static int cmp_nom_croissant(const void *a, const void *b)
{
    const Passager *x = (const Passager *) a, *y = (const Passager *) b;
    int r = strcasecmp(x->nom, y->nom);
    return r != 0 ? r : strcasecmp(x->prenom, y->prenom);   /* meme nom : par prenom */
}

static int cmp_nom_decroissant(const void *a, const void *b)
{
    return -cmp_nom_croissant(a, b);
}

void passager_trier_nom(Passager t[], int n, int croissant)
{
    qsort(t, n, sizeof(Passager), croissant ? cmp_nom_croissant : cmp_nom_decroissant);
}

/* 1 si "mot" apparait dans "texte", sans tenir compte des majuscules */
static int contient_sans_casse(const char *texte, const char *mot)
{
    size_t i, j, lt = strlen(texte), lm = strlen(mot);
    if (lm == 0) return 1;
    for (i = 0; i + lm <= lt; i++) {
        for (j = 0; j < lm; j++)
            if (tolower((unsigned char) texte[i + j]) != tolower((unsigned char) mot[j])) break;
        if (j == lm) return 1;
    }
    return 0;
}

int passager_filtrer(const Passager t[], int n, const char *mot, Passager r[])
{
    int i, k = 0;
    for (i = 0; i < n; i++)
        if (contient_sans_casse(t[i].nom, mot) || contient_sans_casse(t[i].prenom, mot)
            || contient_sans_casse(t[i].cin, mot))
            r[k++] = t[i];
    return k;
}

/* ================= Fidelite ================= */
/* Seuils et remises : proposition a valider en equipe (deck, slide « A trancher »). */

Niveau passager_niveau(int pts)
{
    if (pts >= 3000) return PLATINUM;
    if (pts >= 1500) return GOLD;
    if (pts >= 500)  return SILVER;
    return STANDARD;
}

const char *niveau_nom(Niveau n)
{
    switch (n) {
    case SILVER:   return "Silver";
    case GOLD:     return "Gold";
    case PLATINUM: return "Platinum";
    default:       return "Standard";
    }
}

float niveau_remise(Niveau n)
{
    switch (n) {
    case SILVER:   return 0.05f;
    case GOLD:     return 0.10f;
    case PLATINUM: return 0.15f;
    default:       return 0.0f;
    }
}

int niveau_seuil_suivant(int pts)
{
    if (pts < 500)  return 500;
    if (pts < 1500) return 1500;
    if (pts < 3000) return 3000;
    return -1;
}

float passager_taux_remise(const char *cin)
{
    Passager p;
    if (!passager_chercher(cin, &p)) return 0.0f;
    return niveau_remise(passager_niveau(p.points));
}

/* CDC : 1 point par tranche de 10 DT depenses */
int passager_crediter_points(const char *cin, float prix_dt)
{
    Passager p;
    if (prix_dt < 0) return 0;
    if (!passager_chercher(cin, &p)) return 0;
    p.points += (int) (prix_dt / 10);
    return passager_modifier(&p);
}

int passager_debiter_points(const char *cin, float prix_dt)
{
    Passager p;
    if (prix_dt < 0) return 0;
    if (!passager_chercher(cin, &p)) return 0;
    p.points -= (int) (prix_dt / 10);
    if (p.points < 0) p.points = 0;
    return passager_modifier(&p);
}

/* ================= Blocage (option 2) ================= */

int passager_est_bloque(const char *cin)
{
    Passager p;
    return passager_chercher(cin, &p) && p.bloque;
}

int passager_bloquer(const char *cin, const char *motif)
{
    Passager p;
    if (!passager_chercher(cin, &p) || p.bloque) return 0;
    p.bloque = 1;
    strncpy(p.motif, motif, sizeof p.motif - 1);
    p.motif[sizeof p.motif - 1] = '\0';
    p.date_blocage = date_du_jour();
    return passager_modifier(&p);
}

int passager_reactiver(const char *cin)
{
    Passager p;
    if (!passager_chercher(cin, &p) || !p.bloque) return 0;
    p.bloque = 0;
    strcpy(p.motif, "-");
    p.date_blocage.jour = p.date_blocage.mois = p.date_blocage.annee = 0;
    return passager_modifier(&p);
}
