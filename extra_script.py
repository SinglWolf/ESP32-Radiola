Import("env")

filename=""
my_flags =  env.ParseFlags( env[ 'BUILD_FLAGS']) 
# print(my_flags.get("CPPDEFINES"))
# some flags are just defines without value
# some are key, value pairs
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
