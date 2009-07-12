#!/usr/bin/env sh

echo "Prepare language templates"
pygettext2.6 -a -n -d robotiq -p pot robotiq.py ui/*.py comm/*.py misc/*.py

echo "Compile languages to use in application"
msgfmt -o locale/en/LC_MESSAGES/robotiq.mo pot/en.po
msgfmt -o locale/pl/LC_MESSAGES/robotiq.mo pot/pl.po
