@ECHO OFF
REM  Need python3

CD webpage
ECHO style
COPY style.css style.ori
python ./css-html-js-minify.py style.css
gzip.exe  style.min.css 
MOVE style.min.css.gz style.css
xxd.exe -i style.css > .style
sed.exe -i 's/unsigned/const/g' .style
MOVE style.ori style.css

ECHO script
COPY script.js script.ori
IF EXIST script.min.js (
	gzip.exe script.min.js
	MOVE script.min.js.gz script.js
) ELSE (
	gzip.exe script.js 
	MOVE script.js.gz script.js
)
xxd.exe -i script.js > .script
sed.exe -i 's/unsigned/const/g' .script
MOVE script.ori script.js

ECHO tabbis
COPY tabbis.js tabbis.ori
python ./css-html-js-minify.py tabbis.js
gzip.exe  tabbis.js 
MOVE tabbis.js.gz tabbis.js
xxd.exe -i tabbis.js > .tabbis
sed.exe -i 's/unsigned/const/g' .tabbis
MOVE tabbis.ori tabbis.js

ECHO index
COPY index.html index.htm
python ./css-html-js-minify.py index.htm
gzip.exe index.html
MOVE index.html.gz index.html
xxd.exe -i index.html > .index
sed.exe -i 's/unsigned/const/g' .index
MOVE index.htm index.html

ECHO logo
COPY logo.png logo.ori
gzip.exe logo.png
MOVE logo.png.gz logo.png
xxd.exe -i logo.png > .logo
sed.exe -i 's/unsigned/const/g' .logo
MOVE logo.ori logo.png

ECHO favicon
COPY favicon.png favicon.ori
gzip.exe favicon.png
MOVE favicon.png.gz favicon.png
xxd.exe -i favicon.png > .favicon
sed.exe -i 's/unsigned/const/g' .favicon
MOVE favicon.ori favicon.png

ECHO icons.css
COPY icons.css icons.ori
python ./css-html-js-minify.py icons.css
gzip.exe  icons.min.css 
MOVE icons.min.css.gz icons.css
xxd.exe -i icons.css > .icons
sed.exe -i 's/unsigned/const/g' .icons
MOVE icons.ori icons.css
