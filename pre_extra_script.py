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
import io

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

REMOVE_WS = re.compile(r"\s{2,}").sub

if platform == "linux":
    # linux
    PROJECT_DIR = env.subst("$PROJECT_DIR") + '/'
    PROJECT_INCLUDE_DIR = env.subst("$PROJECT_INCLUDE_DIR") + '/'
    PROJECT_BUILD_DIR = env.subst("$PROJECT_BUILD_DIR") + '/'
    WEBSERWER_C_O = "/esp-idf/main/webserver.c.o"
    WEBPAGE = "webpage/"
    YUI_COMPRESSOR = 'java -jar tools/yuicompressor.jar '
    CLOSURE_COMPILER = 'java -jar tools/compiler.jar  --compilation_level SIMPLE_OPTIMIZATIONS '
    XXD = 'tools/xxd.py -i '
elif platform == "darwin":
    # OS X
    pass
elif platform == "win32":
    # Windows...
    PROJECT_DIR = env.subst("$PROJECT_DIR") + '\\'
    PROJECT_INCLUDE_DIR = env.subst("$PROJECT_INCLUDE_DIR") + '\\'
    PROJECT_BUILD_DIR = env.subst("$PROJECT_BUILD_DIR") + '\\'
    WEBSERWER_C_O = "\\esp-idf\\main\\webserver.c.o"
    WEBPAGE = "webpage\\"
    YUI_COMPRESSOR = 'java -jar tools\\yuicompressor.jar '
    CLOSURE_COMPILER = 'java -jar tools\\compiler.jar  --compilation_level SIMPLE_OPTIMIZATIONS '
    XXD = 'python tools\\xxd.py -i '


file_list = ["favicon.png", "mainpage.html", "logo.png",
             "script.js", "style.css", "tabbis.js", "icons.css"]


def get_hash_sha1(filename):
    if ".png" in filename:
        with open(filename, 'rb') as f:
            m = hashlib.sha1()
            while True:
                data = f.read(2**16)
                if not data:
                    break
                m.update(data)
    else:
        with io.open(filename, 'r', encoding='utf-8') as f:
            m = hashlib.sha1()
            while True:
                data = f.read(2**16)
                if not data:
                    break
                m.update(data.encode('utf-8'))
    return m.hexdigest()


def check_sha1(name):
    check = True
    File = PROJECT_DIR + WEBPAGE + name
    File_sha1 = PROJECT_DIR + WEBPAGE + name.rsplit(".", 1)[0] + '.sha'
    if isfile(File_sha1):
        with open(File_sha1, 'r') as file:
            lst = list()
            for line in file.readlines():
                lst.extend(line.rstrip().split(' '))
    else:
        print("Checksum file for:", name,
              "not found.")
        with open(File_sha1, 'w') as fout:
            f_hash_sha1 = get_hash_sha1(File)
            print(f_hash_sha1 + ' *' + name, file=fout)
        print("Checksum file for:", name, "created.")
        check = False
        return check

    if lst[0] != get_hash_sha1(File):
        with open(File_sha1, 'w') as fout:
            f_hash_sha1 = get_hash_sha1(File)
            print(f_hash_sha1 + ' ' + name, file=fout)
        print("File:", name, "changed. Checksum overwritten.")
        check = False
    return check


def build_page():
    for name in file_list:
        dir_build = PROJECT_DIR + WEBPAGE
        src_file = dir_build + name
        build_file = dir_build + 'build_' + name
        name_var = name.rsplit(".", 1)[0]
        if check_sha1(name):
            pass
        else:
            for build_mode in ["debug", "release"]:
                file_build = PROJECT_BUILD_DIR + build_mode + WEBSERWER_C_O
                if isfile(file_build):
                    os.remove(file_build)
                    print("File webserver.c.o for " + build_mode + " removed.")
                print("Prepare ", name, " for "+ build_mode + ".")
                shutil.copy(src_file, build_file)
                if build_mode == "release":
                    if ".html" in name:
                        with io.open(build_file, 'r', encoding='utf-8') as f:
                            content = f.read()
                        content = REMOVE_WS(" ", content)
                        with io.open(build_file, 'w+', encoding='utf-8') as d:
                            d.write(content)
                    if ".css" in name:
                        cmd = YUI_COMPRESSOR + build_file + ' -o ' + WEBPAGE + 'build_' + name + '.min'
                        subprocess.call(cmd, shell=True)
                    if ".js" in name:
                        cmd = CLOSURE_COMPILER + build_file + ' --js_output_file ' + build_file + '.min'
                        subprocess.call(cmd, shell=True)
                    if isfile(build_file + '.min'):
                        os.remove(build_file)
                        shutil.move(build_file + '.min', build_file)
                with open(build_file, 'rb') as f_in, gzip.open(build_file + '.gz', 'wb') as f_out:
                    f_out.writelines(f_in)
                shutil.move(build_file + '.gz', build_file)
                define = name_var + '_' + build_mode
                if ".png" in name:
                    define = name_var
                elif "tabbis.js" in name:
                    define = name_var
                header_f = define + '.h'
                if isfile(PROJECT_INCLUDE_DIR + header_f):
                    os.remove(PROJECT_INCLUDE_DIR + header_f)
                subprocess.call(XXD + build_file + ' -v ' + name_var +
                                ' -d ' + define + ' -o ' + PROJECT_INCLUDE_DIR + header_f, shell=True)
                print("Header file ", header_f, " created.")
                if isfile(build_file):
                    os.remove(build_file)
                if ".png" in name:
                    break
                elif "tabbis.js" in name:
                    break


build_page()
