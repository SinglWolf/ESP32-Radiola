REM  Need python3
ECHO style
COPY style.css style.ori
python ./css-html-js-minify.py style.css
gzip.exe  style.min.css 
MOVE style.min.css.gz style.css
xxd.exe -i style.css > .style
REM sed.exe -i 's/\[\]/\[\]/g' style
sed.exe -i 's/unsigned/const/g' .style
MOVE style.ori style.css

ECHO style1
COPY style1.css style1.ori
python ./css-html-js-minify.py style1.css
gzip.exe  style1.min.css 
MOVE style1.min.css.gz style1.css
xxd.exe -i style1.css > .style1
REM sed.exe -i 's/\[\]/\[\]/g' style1
sed.exe -i 's/unsigned/const/g' .style1
MOVE style1.ori style1.css

ECHO script
COPY script.js script.ori
REM python ./css-html-js-minify.py script.js 
gzip.exe  script.js 
MOVE script.js.gz script.js
xxd.exe -i script.js > .script
REM sed.exe -i 's/\[\]/\[\]/g' script
sed.exe -i 's/unsigned/const/g' .script
MOVE script.ori script.js

ECHO tabbis
COPY tabbis.js tabbis.ori
python ./css-html-js-minify.py tabbis.js 
gzip.exe  tabbis.js 
MOVE tabbis.js.gz tabbis.js
xxd.exe -i tabbis.js > .tabbis
REM sed.exe -i 's/\[\]/\[\]/g' script
sed.exe -i 's/unsigned/const/g' .tabbis
MOVE tabbis.ori tabbis.js

ECHO index
COPY index.html index.htm
python ./css-html-js-minify.py index.htm
gzip.exe index.html
MOVE index.html.gz index.html
xxd.exe -i index.html > .index
REM sed.exe -i 's/\[\]/\[\]/g' index
sed.exe -i 's/unsigned/const/g' .index
MOVE index.htm index.html

ECHO logo
COPY logo.png logo.ori
gzip.exe logo.png
MOVE logo.png.gz logo.png
xxd.exe -i logo.png > .logo
REM sed.exe -i 's/\[\]/\[\]/g' logo
sed.exe -i 's/unsigned/const/g' .logo
MOVE logo.ori logo.png

ECHO favicon
COPY favicon.png favicon.ori
gzip.exe favicon.png
MOVE favicon.png.gz favicon.png
xxd.exe -i favicon.png > .favicon
REM sed.exe -i 's/\[\]/\[\]/g' favicon
sed.exe -i 's/unsigned/const/g' .favicon
MOVE favicon.ori favicon.png
