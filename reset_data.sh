#!/bin/sh
# Remet des données propres avant un test ou une démo (lancer depuis la racine du projet)
cd "$(dirname "$0")"
mkdir -p data
cp data_init/*.txt data/
echo "data/ réinitialisé depuis data_init/ (9 passagers, 11 comptes)"
