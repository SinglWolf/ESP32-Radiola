#!/bin/bash
# Need python3. Install it with pacman -Sy python3
echo style
cp style.css style.ori
python3 ./css-html-js-minify.py style.css
gzip style.min.css 
mv style.min.css.gz style.css
xxd -i style.css > .style
# sed -i 's/\[\]/\[\]/g' .style
sed -i 's/unsigned/const/g' .style
mv style.ori style.css

echo icons.css
cp icons.css icons.ori
python3 ./css-html-js-minify.py icons.css
gzip icons.min.css 
mv icons.min.css.gz icons.css
xxd -i icons.css > .icons
# sed -i 's/\[\]/\[\]/g' .icons
sed -i 's/unsigned/const/g' .icons
mv icons.ori icons.css

echo script
cp script.js script.ori
python minify.py script.js
gzip script.min.js 
mv script.min.js.gz script.js
xxd -i script.js > .script
sed -i 's/unsigned/const/g' .script
mv script.ori script.js

echo tabbis
cp tabbis.js tabbis.ori
python minify.py tabbis.js 
gzip tabbi.min.js 
mv tabbi.min.js.gz tabbis.js
xxd -i tabbis.js > .tabbis
sed -i 's/unsigned/const/g' .tabbis
mv tabbis.ori tabbis.js

echo index
cp index.html index.htm
python3 ./css-html-js-minify.py index.htm
gzip index.html
mv index.html.gz index.html
xxd -i index.html > .index
#sed -i 's/\[\]/\[\]/g' .index
sed -i 's/unsigned/const/g' .index
mv index.htm index.html

echo logo
cp logo.png logo.ori
gzip logo.png
mv logo.png.gz logo.png
xxd -i logo.png > .logo
#sed -i 's/\[\]/\[\]/g' logo
sed -i 's/unsigned/const/g' .logo
mv logo.ori logo.png

echo favicon
cp favicon.png favicon.ori
gzip favicon.png
mv favicon.png.gz favicon.png
xxd -i favicon.png > .favicon
# sed -i 's/\[\]/\[\]/g' .favicon
sed -i 's/unsigned/const/g' .favicon
mv favicon.ori favicon.png
