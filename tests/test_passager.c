/*
 * tests/test_passager.c - teste la couche metier SANS ouvrir de fenetre.
 *
 * Depuis la racine du projet :
 *   gcc -Wall -g -Isrc -DF_PASSAGERS='"tests/p.txt"' -DF_TMP='"tests/tmp.txt"' \
 *       tests/test_passager.c src/passager.c src/utils.c -o t && ./t
 *
 * Le -DF_PASSAGERS redirige vers tests/p.txt : les vraies donnees ne sont jamais touchees.
 */
#include <stdio.h>
#include <string.h>
#include "passager.h"
#include "utils.h"
#include "chemins.h"

static int ok = 0, ko = 0;
#define TEST(c, nom) do { if (c) { ok++; printf("[OK] %s\n", nom); } \
                          else   { ko++; printf("[KO] %s\n", nom); } } while (0)

static Passager fabriquer(const char *cin, const char *nom, const char *prenom, int points)
{
    Passager p;
    Date d = {14, 6, 1991};
    passager_init(&p);
    strcpy(p.cin, cin);
    strcpy(p.nom, nom);
    strcpy(p.prenom, prenom);
    p.naissance = d;
    p.sexe = 'F';
    strcpy(p.nationalite, "Tunisienne");
    strcpy(p.tel, "55123987");
    strcpy(p.email, "contact@exemple.tn");
    p.points = points;
    return p;
}

int main(void)
{
    Date bissextile = {29, 2, 2024}, non_bissextile = {29, 2, 2023};
    Date fevrier31 = {31, 2, 2000}, futur = {1, 1, 2099};
    Passager p, lu, t[10], r[10];
    int n;

    remove(F_PASSAGERS);                                  /* fichier de test vierge */

    printf("--- utils ---\n");
    TEST(est_date_valide(bissextile), "29/02/2024 valide");
    TEST(!est_date_valide(non_bissextile), "29/02/2023 refusee");
    TEST(!est_date_valide(fevrier31), "31/02/2000 refusee");
    TEST(!est_date_valide(futur), "date future refusee");
    TEST(est_cin_valide("07418529"), "CIN 8 chiffres");
    TEST(!est_cin_valide("0741a529"), "CIN avec lettre refuse");
    TEST(!est_cin_valide("1234567"), "CIN 7 chiffres refuse");
    TEST(est_tel_valide("98765432"), "tel commencant par 9");
    TEST(!est_tel_valide("38765432"), "tel commencant par 3 refuse");
    TEST(est_email_valide("rania.mejri@gmail.com"), "e-mail valide");
    TEST(!est_email_valide("rania@@gmail.com"), "deux @ refuse");
    TEST(!est_email_valide("rania@gmail"), "sans point refuse");
    TEST(est_nom_valide("Ben Ammar"), "nom compose");
    TEST(est_nom_valide("Ch\xc3\xa9rif"), "nom accentue (UTF-8)");
    TEST(!est_nom_valide("R2D2"), "nom avec chiffres refuse");
    TEST(!est_texte_valide("Impaye;billet", 200), "';' refuse dans un texte");

    printf("--- CRUD ---\n");
    p = fabriquer("07418529", "Mejri", "Rania", 640);
    TEST(passager_ajouter(&p) == 1, "ajout");
    TEST(passager_ajouter(&p) == 0, "doublon refuse");
    TEST(passager_chercher("07418529", &lu) && strcmp(lu.nom, "Mejri") == 0, "relecture");

    p = fabriquer("13572468", "Gharbi", "Oussama", 1720);  passager_ajouter(&p);
    p = fabriquer("24681357", "Ben Ammar", "Skander", 90); passager_ajouter(&p);
    p = fabriquer("31415926", "haddad", "Yasmine", 3150);  passager_ajouter(&p);

    lu.points = 700; strcpy(lu.email, "r.mejri@topnet.tn");
    TEST(passager_modifier(&lu) == 1, "modification");
    TEST(passager_chercher("07418529", &lu) && lu.points == 700
         && strcmp(lu.email, "r.mejri@topnet.tn") == 0, "modification relue");
    TEST(passager_chercher("13572468", &p) && p.points == 1720, "les autres lignes intactes");

    printf("--- liste, tri, filtre ---\n");
    n = passager_charger_tous(t, 10);
    TEST(n == 4, "4 passagers charges");
    passager_trier_nom(t, n, 1);
    TEST(strcmp(t[0].nom, "Ben Ammar") == 0 && strcmp(t[3].nom, "Mejri") == 0,
         "tri croissant, casse ignoree (haddad entre Gharbi et Mejri)");
    passager_trier_nom(t, n, 0);
    TEST(strcmp(t[0].nom, "Mejri") == 0, "tri decroissant");
    TEST(passager_filtrer(t, n, "AMM", r) == 1 && strcmp(r[0].prenom, "Skander") == 0,
         "recherche partielle insensible a la casse");

    printf("--- fidelite ---\n");
    TEST(passager_niveau(499) == STANDARD && passager_niveau(500) == SILVER, "seuil Silver");
    TEST(passager_niveau(3000) == PLATINUM, "seuil Platinum");
    TEST(passager_crediter_points("24681357", 389.5f) == 1, "credit 389,5 DT");
    TEST(passager_chercher("24681357", &p) && p.points == 90 + 38, "+38 points");

    printf("--- blocage ---\n");
    TEST(passager_bloquer("13572468", "Impaye billet TU514") == 1, "blocage");
    TEST(passager_bloquer("13572468", "encore") == 0, "double blocage refuse");
    TEST(passager_est_bloque("13572468"), "statut bloque");
    TEST(passager_reactiver("13572468") == 1 && !passager_est_bloque("13572468"), "reactivation");

    printf("--- suppression ---\n");
    TEST(passager_supprimer("31415926") == 1, "suppression");
    TEST(passager_chercher("31415926", NULL) == 0, "introuvable ensuite");
    TEST(passager_supprimer("99999999") == 0, "supprimer un CIN absent : 0, pas de crash");
    TEST(passager_charger_tous(t, 10) == 3, "il reste 3 passagers");

    printf("\n%d OK, %d KO\n", ok, ko);
    return ko != 0;
}
