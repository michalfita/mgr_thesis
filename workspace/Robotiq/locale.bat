"C:\Program Files\Python26\Tools\i18n\pygettext.py" -a -n -d robotiq -p pot robotiq.py ui\*.py

"C:\Program Files\Python26\Tools\i18n\msgfmt.py" -o locale\en\LC_MESSAGES\robotiq.mo pot\en.po
"C:\Program Files\Python26\Tools\i18n\msgfmt.py" -o locale\pl\LC_MESSAGES\robotiq.mo pot\pl.po
