#!/bin/bash
# Need python3. Install it with pacman -Sy python3
SERVFILE="$1/servfile.h"
DEBUG=$2
cd $(pwd)/webpage
echo style
cp style.css style.ori
python3 ./css-html-js-minify.py style.css
gzip style.min.css 
mv style.min.css.gz style.css
echo -e "#ifndef __SERVFILE_H__\n#define __SERVFILE_H__\n" > ${SERVFILE}
xxd -i style.css | sed 's/unsigned/const/g' >> ${SERVFILE}
echo -e "\n" >> ${SERVFILE}
mv style.ori style.css

echo icons.css
cp icons.css icons.ori
python3 ./css-html-js-minify.py icons.css
gzip icons.min.css 
mv icons.min.css.gz icons.css
xxd -i icons.css | sed 's/unsigned/const/g' >> ${SERVFILE}
echo -e "\n" >> ${SERVFILE}
mv icons.ori icons.css

echo script
cp script.js script.ori
if $(test -f ./script.min.js); then
gzip script.min.js
mv script.min.js.gz script.js
else
gzip script.js 
mv script.js.gz script.js
fi
xxd -i script.js | sed 's/unsigned/const/g' >> ${SERVFILE}
echo -e "\n" >> ${SERVFILE}
mv script.ori script.js

echo tabbis
cp tabbis.js tabbis.ori
gzip tabbis.js 
mv tabbis.js.gz tabbis.js
xxd -i tabbis.js | sed 's/unsigned/const/g' >> ${SERVFILE}
echo -e "\n" >> ${SERVFILE}
mv tabbis.ori tabbis.js

echo index
cp index.html index.htm
if [ "${DEBUG}" -eq "0" ]; then
python3 ./css-html-js-minify.py index.htm
fi
gzip index.html
mv index.html.gz index.html
xxd -i index.html | sed 's/unsigned/const/g' >> ${SERVFILE}
echo -e "\n" >> ${SERVFILE}
mv index.htm index.html

echo logo
cp logo.png logo.ori
gzip logo.png
mv logo.png.gz logo.png
xxd -i logo.png | sed 's/unsigned/const/g' >> ${SERVFILE}
echo -e "\n" >> ${SERVFILE}
mv logo.ori logo.png

echo favicon
cp favicon.png favicon.ori
gzip favicon.png
mv favicon.png.gz favicon.png
xxd -i favicon.png | sed 's/unsigned/const/g' >> ${SERVFILE}
echo -e "\n#endif" >> ${SERVFILE}
mv favicon.ori favicon.png
