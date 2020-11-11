from sys import platform
from os.path import isfile, join
import os
import shutil
import hashlib

Import("env")
# print(env.Dump())
# print(env.subst("$PIOENV"))
# print(env.subst("$TARGET"))
PIOENV = env.subst("$PIOENV")
PROGNAME = env.subst("$PROGNAME")
PROJECT_DIR = env.subst("$PROJECT_DIR")
frim_bin = PROGNAME + ".bin"
file_list = [frim_bin, "bootloader.bin", "partitions.bin"]

if platform == "linux":
    # linux
    BUILD_DIR = env.subst("$BUILD_DIR") + '/'
    PROJECT_BIN_DIR = PROJECT_DIR + '/binaries/' + PIOENV + '/'
elif platform == "darwin":
    # OS X
    pass
elif platform == "win32":
    # Windows...
    BUILD_DIR = env.subst("$BUILD_DIR") + '\\'
    PROJECT_BIN_DIR = PROJECT_DIR + '\\binaries\\' + PIOENV + '\\'

def get_hash_sha1(filename):
    with open(filename, 'rb') as f:
        m = hashlib.sha1()
        while True:
            data = f.read(2**16)
            if not data:
                break
            m.update(data)
        return m.hexdigest()

def check_sha1(name):
    check = True
    File = PROJECT_BIN_DIR + name
    File_sha1 = PROJECT_BIN_DIR + name.rsplit(".", 1)[0] + '.sha'
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

def del_files():
    for name in file_list:
        if isfile(BUILD_DIR + name):
            print(name)
            os.remove(BUILD_DIR + name)

# exit(0)
del_files()

def after_build(source, target, env):
    for name in file_list:
        if isfile(BUILD_DIR + name):
            shutil.copy(BUILD_DIR + name, PROJECT_BIN_DIR + name)
            check_sha1(name)


# if not check_sha1(file_list):
#     env.AddPreAction("${BUILD_DIR}/esp-idf/main/webserver.c.o", before_build)
# else:
#     print("Files not changed.")
# env.AddPreAction("program", before_build)
env.AddPostAction('buildprog', after_build)
# env.AddPostAction("${BUILD_DIR}/${PROGNAME}.bin", after_build)
