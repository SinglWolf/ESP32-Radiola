# -*- coding: utf-8 -*-
import sys
from importlib import reload
from os.path import isfile, join
from sys import platform
import requests
import subprocess
Import("env")

# print(env.Dump())
# sys.exit()
filename = ""
my_flags = env.ParseFlags(env['BUILD_FLAGS'])
#
for x in my_flags.get("CPPDEFINES"):
    # print(x)
    if isinstance(x, list):
        # grab as key, value
        k, v = x
        if k == "FILENAME":
            filename = v
            # no need to iterate further
            break
env.Replace(PROGNAME="%s" % str(filename))
PROJECT_INCLUDE_DIR = env.subst("$PROJECT_INCLUDE_DIR")
servfile = PROJECT_INCLUDE_DIR + "/servfile.h"
if not isfile(servfile):
    DEBUG = "1"
    if filename == 'ESP32Radiola-release':
        DEBUG = "0"
        try:
            js_file = 'webpage/script.js'
        except:
            print("Missing input file")
            sys.exit()
        # Grab the file contents
        with open(js_file, 'r') as c:
            js = c.read()
        # Pack it, ship it
        payload = {'input': js}
        url = 'https://javascript-minifier.com/raw'
        print("Requesting mini-me of {}. . .".format(c.name))
        r = requests.post(url, payload)
        # Write out minified version
        minified = js_file.rstrip('.js')+'.min.js'
        with open(minified, 'w') as m:
            m.write(r.text)
        print("Minification complete. See {}".format(m.name))
    if platform == "linux":
        # linux
        subprocess.call("./generate.sh " + PROJECT_INCLUDE_DIR + ' ' + DEBUG, shell=True)
    elif platform == "darwin":
        # OS X
        pass
    elif platform == "win32":
        # Windows...
        subprocess.call('./generate.bat' + PROJECT_INCLUDE_DIR + DEBUG, shell=True)
else:
    print(index_h)
# if not isfile("${PROJECT_DIR}/webpage/.index"):
#     before_build
#env.AddPreAction("${BUILD_DIR}/esp-idf/main/webserver.c.o", before_build)
