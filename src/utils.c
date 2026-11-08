/*
 * utils.c - dates et controles de saisie
 * Couche metier : aucun #include <gtk/gtk.h>, testable avec un simple main().
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "utils.h"

/* ================= Dates ================= */

int est_bissextile(int a)
{
    return (a % 4 == 0 && a % 100 != 0) || a % 400 == 0;
}

int jours_dans_mois(int m, int a)
{
    static const int j[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m < 1 || m > 12) return 0;
    if (m == 2 && est_bissextile(a)) return 29;
    return j[m - 1];
}

Date date_du_jour(void)
{
    Date d;
    time_t t = time(NULL);
    struct tm *now = localtime(&t);
    d.jour = now->tm_mday;
    d.mois = now->tm_mon + 1;
    d.annee = now->tm_year + 1900;
    return d;
}

/* compare deux dates : <0 si a avant b, 0 si egales, >0 si a apres b */
static int comparer_dates(Date a, Date b)
{
    if (a.annee != b.annee) return a.annee - b.annee;
    if (a.mois != b.mois) return a.mois - b.mois;
    return a.jour - b.jour;
}

int calculer_age(Date n)
{
    Date auj = date_du_jour();
    int age = auj.annee - n.annee;
    if (auj.mois < n.mois || (auj.mois == n.mois && auj.jour < n.jour))
        age--;
    return age;
}

int est_date_valide(Date d)
{
    if (d.annee < 1900) return 0;
    if (d.mois < 1 || d.mois > 12) return 0;
    if (d.jour < 1 || d.jour > jours_dans_mois(d.mois, d.annee)) return 0;
    if (comparer_dates(d, date_du_jour()) > 0) return 0;   /* dans le futur */
    if (calculer_age(d) > 120) return 0;
    return 1;
}

/* ================= Chaines ================= */

void nettoyer(char *s)
{
    int debut = 0, fin;
    if (s == NULL) return;
    while (s[debut] != '\0' && isspace((unsigned char) s[debut])) debut++;
    if (debut > 0) memmove(s, s + debut, strlen(s + debut) + 1);
    fin = (int) strlen(s) - 1;
    while (fin >= 0 && isspace((unsigned char) s[fin])) s[fin--] = '\0';
}

int contient_separateur(const char *s)
{
    return s != NULL && strpbrk(s, ";\n\r") != NULL;
}

static int que_des_chiffres(const char *s, int longueur)
{
    int i;
    if (s == NULL || (int) strlen(s) != longueur) return 0;
    for (i = 0; i < longueur; i++)
        if (!isdigit((unsigned char) s[i])) return 0;
    return 1;
}

/* ================= Controles de saisie ================= */

int est_cin_valide(const char *s)
{
    return que_des_chiffres(s, 8);
}

int est_tel_valide(const char *s)
{
    return que_des_chiffres(s, 8) && strchr("24579", s[0]) != NULL;
}

int est_nom_valide(const char *s)
{
    int i, lettres = 0;
    if (s == NULL || s[0] == '\0') return 0;
    for (i = 0; s[i] != '\0'; i++) {
        unsigned char c = (unsigned char) s[i];
        if (isalpha(c) || c >= 0x80) lettres++;   /* >= 0x80 : octets UTF-8 (é, è, ç...) */
        else if (c != ' ' && c != '-' && c != '\'') return 0;
    }
    return lettres > 0;
}

int est_email_valide(const char *s)
{
    const char *arobase, *point;
    if (s == NULL || s[0] == '\0') return 0;
    if (strchr(s, ' ') != NULL || contient_separateur(s)) return 0;
    arobase = strchr(s, '@');
    if (arobase == NULL || arobase == s) return 0;          /* @ absent ou en 1re position */
    if (strchr(arobase + 1, '@') != NULL) return 0;          /* deux @ */
    point = strrchr(arobase + 1, '.');
    if (point == NULL || point == arobase + 1) return 0;     /* pas de point, ou "@." */
    if (point[1] == '\0') return 0;                          /* point en derniere position */
    return 1;
}

int est_texte_valide(const char *s, int max_octets)
{
    return s != NULL && s[0] != '\0'
        && (int) strlen(s) <= max_octets
        && !contient_separateur(s);
}
