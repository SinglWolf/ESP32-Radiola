# -*- coding: utf-8 -*-

import hashlib
import subprocess
import requests
from sys import platform
from os.path import isfile, join
from importlib import reload
import os
import re
import sys
import shutil
import gzip

Import("env")

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
# print(env.Dump())
# print(env.subst("$PIOENV"))
# print(env.subst("$TARGET"))
# exit(0)

REMOVE_WS = re.compile(r"\s{2,}").sub
YUI_COMPRESSOR = 'java -jar tools/yuicompressor.jar '
CLOSURE_COMPILER = 'java -jar tools/compiler.jar  --compilation_level SIMPLE_OPTIMIZATIONS '

PROJECT_DIR = env.subst("$PROJECT_DIR")

PROJECT_BUILD_DIR = env.subst("$PROJECT_BUILD_DIR")

PROJECT_INCLUDE_DIR = env.subst("$PROJECT_INCLUDE_DIR")

file_list = ["favicon.png", "index.html", "logo.png",
             "script.js", "style.css", "tabbis.js", "icons.css"]


def get_hash_md5(filename):
    with open(filename, 'rb') as f:
        m = hashlib.md5()
        while True:
            data = f.read(8192)
            if not data:
                break
            m.update(data)
        return m.hexdigest()


def check_md5(name):
    check = True
    File = PROJECT_DIR + "/webpage/" + name
    File_md5 = File + '.md5'
    if isfile(File_md5):
        with open(File_md5, 'r') as file:
            lst = list()
            for line in file.readlines():
                lst.extend(line.rstrip().split(' '))
    else:
        print("Checksum file for:", name,
              "not found.")
        with open(File_md5, 'w') as fout:
            f_hash_md5 = get_hash_md5(File)
            print(f_hash_md5 + ' ' + name, file=fout)
        print("Checksum file for:", name, "created.")
        check = False
        return check

    if lst[0] != get_hash_md5(File):
        with open(File_md5, 'w') as fout:
            f_hash_md5 = get_hash_md5(File)
            print(f_hash_md5 + ' ' + name, file=fout)
        print("File:", name, "changed. Checksum overwritten.")
        check = False
    return check


def build_page():
    check = False
    for name in file_list:
        dir_build = PROJECT_DIR + "/webpage/"
        f_build = dir_build + name
        name_var = name.rsplit(".", 1)[0]
        if check_md5(name):
            # print("File ", name, " not changed.")
            pass
        else:
            for build_mode in ["debug", "release"]:
                file_build = PROJECT_BUILD_DIR + '/' + \
                    build_mode + "/esp-idf/main/webserver.c.o"
                if isfile(file_build):
                    os.remove(file_build)
                    print("File: " + build_mode + "/webserver.c.o removed.")
                print("Prepare ", name, " for build.")
                shutil.copy(f_build, f_build + '.bak')
                if build_mode == "release":
                    if ".html" in name:
                        f = open(f_build, 'r')
                        content = f.read()
                        content = REMOVE_WS(" ", content)
                        d = open(f_build, 'w+')
                        d.write(content)
                    if ".css" in name:
                        cmd = YUI_COMPRESSOR + f_build + ' -o ' + f_build + '.min'
                        subprocess.call(cmd, shell=True)
                    if ".js" in name:
                        cmd = CLOSURE_COMPILER + f_build + ' --js_output_file ' + f_build + '.min'
                        subprocess.call(cmd, shell=True)
                    if isfile(f_build + '.min'):
                        os.remove(f_build)
                        shutil.move(f_build + '.min', f_build)
                with open(f_build, 'rb') as f_in, gzip.open(f_build + '.gz', 'wb') as f_out:
                    f_out.writelines(f_in)
                shutil.move(f_build + '.gz', f_build)
                define = name_var + '_' + build_mode
                if ".png" in name:
                    define = name_var
                elif "tabbis.js" in name:
                    define = name_var
                header_f = define + '.h'
                subprocess.call("tools/xxd.py -i" + f_build + ' -v ' + name_var +
                                ' -d ' + define + ' -o ' + PROJECT_INCLUDE_DIR + '/' + header_f, shell=True)
                print("Header file ", header_f, " created.")
                if isfile(f_build + '.bak'):
                    os.remove(f_build)
                    shutil.move(f_build + '.bak', f_build)
                if ".png" in name:
                    break
                elif "tabbis.js" in name:
                    break
            check = True
    if check == False:
        print("Files the web page not changed.")


build_page()

# exit(0)
# if not isfile(file_build):


# if not check_md5(file_list):
#     env.AddPreAction("${BUILD_DIR}/esp-idf/main/webserver.c.o", before_build)
# else:
#     print("Files not changed.")
# env.AddPreAction("program", before_build)
