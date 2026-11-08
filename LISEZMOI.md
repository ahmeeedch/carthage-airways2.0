# Carthage Airways · Module 1 « Gestion des passagers »
Interface « Clair et premium », en projet Glade 2 (GTK 2.24, Ubuntu 18)

## 1. Ce que contient le dossier

| Élément | Rôle |
|---|---|
| `carthage.glade` | **L'interface** : les 4 fenêtres en blocs Glade 2 (542 widgets, 44 signaux) |
| `carthage.gladep` | Les options du projet Glade (dossiers `src/` et `pixmaps/`, noms des widgets activés) |
| `src/interface.c/.h`, `support.c/.h` | Code **généré par Glade** : ne pas modifier à la main |
| `src/callbacks.c/.h` | Les fonctions appelées par les signaux Glade (courtes : elles appellent les modules) |
| `src/ui_theme.c` | Thème Jour / Nuit + dessin cairo des widgets (coins arrondis, ombres, pilules) |
| `src/ui_anim.c` | Animations : titre, compteurs, jauges, secousse, messages (toasts) |
| `src/ui_shell.c` | Barre du haut + menu ☰ repliable, commun aux deux espaces |
| `src/connexion.c` | Écran de connexion (avion animé) et fenêtre « Compte suspendu » |
| `src/passager_admin.c` | Admin : liste, fiche ajouter/modifier/supprimer, statistiques, blocage |
| `src/passager_espace.c` | Passager : carte de fidélité animée, progression, profil |
| `src/passager.c`, `utils.c`, `auth.c`, `session.c` | Couche métier (fichiers texte, contrôles de saisie), sans GTK |
| `pixmaps/` | Les 31 PNG du design (voir §6) |
| `data_init/` | Données de démonstration ; `./reset_data.sh` les copie dans `data/` |
| `tests/test_passager.c` | 38 tests de la couche métier, sans fenêtre |
| `outils/generer_png.c` | Programme qui a dessiné les PNG (pour changer une couleur puis régénérer) |
| `apercu/` | Captures d'écran de référence |

## 2. Installer et lancer (VM Ubuntu 18)

Paquets nécessaires (une seule fois) :

```sh
sudo apt-get install build-essential libgtk2.0-dev autoconf automake libglib2.0-dev
```

**Méthode A : autotools (comme un projet Glade classique)**

```sh
cd carthage
./autogen.sh          # génère configure + Makefile
make
./reset_data.sh       # crée data/ à partir de data_init/
./src/carthage        # TOUJOURS depuis la racine du projet
./src/carthage --nuit # même chose, en thème Nuit
```

**Méthode B : si autogen.sh pose problème**

```sh
make -f Makefile.simple
./reset_data.sh
./src/carthage
```

> Lancer depuis la racine est obligatoire : le programme cherche `pixmaps/` et `data/` à partir du dossier courant.

## 3. Comptes de démonstration

| Identifiant | Mot de passe | Ce qu'on voit |
|---|---|---|
| `admin` | `admin` | Administration, module Passagers |
| `07418529` | `rania` | Espace passager, niveau Silver |
| `40917263` | `ines` | Espace passager, niveau Gold |
| `85230961` | `karim` | Espace passager, niveau Platinum |
| `52896314` | `malek` | Fenêtre « Compte suspendu » |
| `PN0217` | `cabine17` | Message « module 4 pas encore intégré » |

Un passager ajouté par l'admin reçoit un compte : identifiant **et** mot de passe = son CIN.

Raccourcis : `Ctrl+B` replier le menu · `Ctrl+T` Jour/Nuit · `Ctrl+1…6` changer de page · `Alt+1…4` onglets Passagers · double-clic sur une ligne = ouvrir la fiche.

## 4. Travailler dans Glade 2

```sh
glade-2 carthage.glade      # Glade lit aussi carthage.gladep (même nom)
```

1. Modifie une fenêtre, puis **Enregistrer**.
2. **Construire** (Build) : Glade régénère `interface.c` et `support.c`, et ajoute **à la fin** de `callbacks.c` le squelette des nouveaux signaux (il ne touche pas à l'existant).
3. Remplis le nouveau callback, puis `make` (ou `make -f Makefile.simple`).

Dans *Options du projet → Options C*, l'option **« Définir les noms des widgets »** doit rester cochée (elle l'est dans `carthage.gladep`). Si elle est décochée, `ui_habiller()` retrouve quand même les noms, mais garde-la cochée.

## 5. La règle qui fait le design : le rôle est dans le nom

Le **2e mot** du nom d'un widget dans Glade dit comment le dessiner. Après `create_window_xxx()`, un seul appel `ui_habiller(fenetre)` parcourt la fenêtre et branche le bon dessin.

| Rôle | Widget Glade | Rendu |
|---|---|---|
| `btnp` / `btnf` / `btnd` / `btn` | GtkButton | pilule bordeaux / sans fond / rouge / secondaire |
| `carte` | GtkAlignment (padding = marge) | carte blanche arrondie avec ombre douce |
| `champ` | GtkAlignment autour d'un GtkEntry, GtkSpinButton ou GtkComboBox | boîte arrondie, bord bordeaux au focus, rouge en erreur |
| `chip`, `chipa`, `chipok`, `chiperr` | GtkEventBox + GtkLabel | pastille (neutre, accent, verte, rouge) |
| `nav` (+ `badge`, `navtxt`) | GtkEventBox | élément du menu ☰ : survol animé, badge carré |
| `onglet` (+ `ongtxt`) | GtkEventBox | onglet segmenté, la pastille blanche glisse |
| `icone`, `bascule` | GtkEventBox | bouton ☰, interrupteur ☀ Jour / ☾ Nuit |
| `coche` | GtkCheckButton / GtkRadioButton | case et rond redessinés |
| `jauge`, `jaugeor` | GtkProgressBar | barre arrondie bordeaux / dorée |
| `table` | GtkTreeView | lignes alternées, en-têtes à plat |
| `titre`, `stitre`, `chiffre`, `sur`, `t2`, `t3`, `val`, `err`, `mono`, `marque` | GtkLabel | typographie (serif, surtitres espacés, gris...) |
| `topbar`, `sidebar`, `surface`, `ligne` | GtkEventBox | fonds et filets de 1 px |
| `avatar`, `alerte`, `cartefid` | GtkDrawingArea | initiales, « ! », carte de fidélité |

Exemple : un nouveau bouton nommé `vol_btnp_rechercher` sera automatiquement une pilule bordeaux avec survol animé. Les autres modules (Vols, Réservations...) n'ont qu'à remplir la page réservée qui porte leur nom dans `window_admin` / `window_passager` et suivre la même règle.

Toutes les couleurs sont dans deux tableaux au début de `src/ui_theme.c` (`JOUR` et `NUIT`).

## 6. Animations et images PNG

| Où | Animation | PNG utilisés |
|---|---|---|
| Connexion | l'avion décolle le long de la trajectoire (GtkImage déplacé dans un GtkFixed, image choisie selon l'angle), la trace dorée se dessine derrière lui ; la fenêtre tremble si le mot de passe est faux | `fond_connexion_jour.png`, `fond_connexion_nuit.png`, `avion_00.png` … `avion_21.png` |
| Barre du haut | logo, menu ☰ qui se replie (248 → 76 px), interrupteur Jour/Nuit | `logo_ca_32.png`, `logo_ca.png` (icône des fenêtres) |
| Titres de page | fondu + lettres qui se resserrent | — |
| Boutons, menu, onglets | changement de couleur en douceur au survol (≈ 120 ms) | — |
| Statistiques | compteurs qui défilent, jauges en cascade | — |
| Ma fidélité | reflet qui traverse la carte, compteur de points, jauge dorée | `carte_standard.png`, `carte_silver.png`, `carte_gold.png`, `carte_platinum.png` |
| Pages des autres modules | — | `illus_module.png` |

Les anciens fonds (`fond_auth.png`, `fond_admin.png`...) ne sont plus utilisés.

## 7. Tester la couche métier seule

```sh
gcc -Wall -g -Isrc -DF_PASSAGERS='"tests/p.txt"' -DF_TMP='"tests/tmp.txt"' \
    tests/test_passager.c src/passager.c src/utils.c -o t && ./t
```

Attendu : `38 OK, 0 KO` (les vraies données de `data/` ne sont jamais touchées).

## 8. Si quelque chose ne marche pas

| Symptôme | Solution |
|---|---|
| `Couldn't find pixmap file` | Lance le programme depuis la racine : `./src/carthage` |
| « Fichier data/users.txt introuvable » | `./reset_data.sh` |
| `autogen.sh` échoue | Installe `autoconf automake libglib2.0-dev`, ou utilise `make -f Makefile.simple` |
| Les données de test sont abîmées | `./reset_data.sh` remet les 9 passagers d'origine |
| Les polices ne ressemblent pas aux captures | `sudo apt-get install fonts-dejavu` |
