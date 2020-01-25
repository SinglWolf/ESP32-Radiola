Import("env") 
with open("./version.txt", 'r') as myfile:
  data = myfile.read()

my_flags =  env.ParseFlags( env[ 'BUILD_FLAGS']) 
defines =  { k:  v for (k,  v)  in my_flags.get("CPPDEFINES")} 
# print(defines)

env.Replace( PROGNAME= "%s"  %  defines.get("FILENAME") + "_" + data)
