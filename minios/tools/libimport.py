##################################################################
#
# Copies the contents of the MicroPython lib repo
# (https://github.com/micropython/micropython-lib)
# to MINIPYTHON_TARGET_DIR so that the libs are ready
# to use by minipython.
#
##################################################################
import os
import subprocess

MICROPY_LIB_DIR       =  "/root/workspace/micropython-lib/"
MINIPYTHON_TARGET_DIR =  "/mnt/fat/lib/"
IGNORE_DIRS           =  [".git", "__future__"]

# For reporting at the end
ADDED_LIBS            = []
IGNORED_LIBS          = []

def process_lib(libname, path):
    t_dir = MINIPYTHON_TARGET_DIR + libname
    
    print "\nprocessing library: " + libname
    print "===================================="

    # Only skip if python file of same name as lib exists and it's empty
    # (some libs don't have such a base file, we don't skip those.).
    basefile = path + "/" + libname + ".py"    
    if os.path.isfile(basefile) and os.stat(basefile).st_size == 0:
        IGNORED_LIBS.append(libname)
        print "main library file is empty, skipping..."
        return
    
    # Process library directory
    for f in os.listdir(path):
        print "processing file: " + f

        # File has same name as library name, i.e., socket.py
        # for socket library. Make a new subdirectory in
        # MINIPYTHON_TARGET_DIR and copy the file over
        if f == libname + ".py":
            full_f = path + "/" + f
            cmd = "cp " + full_f + " " + MINIPYTHON_TARGET_DIR
            run_cmd(cmd)
            add_lib(libname)
        # We've found a subdirectory in the library, simply copy it
        # over (recursively)
        elif os.path.isdir(path + "/" + f):
            t_subdir = t_dir + "/" + f
            cmd = "mkdir -p " + t_dir 
            run_cmd(cmd)
            cmd = "cp -R " + path + "/ " + MINIPYTHON_TARGET_DIR
            run_cmd(cmd)
            add_lib(libname)            

def add_lib(lib):
    if lib not in ADDED_LIBS:
        ADDED_LIBS.append(lib)
        
def run_cmd(cmd):
    print cmd
    subprocess.call(cmd, shell=True) 
    
def is_ignore_dir(path):
    for d in IGNORE_DIRS:
        if path.find(d) != -1:
            return True
    return False
    
def main():
    run_cmd("mkdir -p " + MINIPYTHON_TARGET_DIR)
    for libname in os.listdir(MICROPY_LIB_DIR):
        path = MICROPY_LIB_DIR + libname        
        if os.path.isdir(path) and not is_ignore_dir(path):
            process_lib(libname, path)

    print "\n\nAdded libs:\n=====================\n" + str(ADDED_LIBS)
    print "\n\nIgnored libs:\n=====================\n" + str(IGNORED_LIBS)    
    
main()
