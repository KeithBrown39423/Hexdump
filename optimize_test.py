from time import time
import subprocess as sp
from os import stat, path

# TODO: error checking
#       work off abs not rel paths
#       add command line options

# test file to read bytes from
tfile = "optimize_test.py"

base_command = ["g++", "-Wall", "-Werror", "-Wpedantic", "-I", "lib", "-I", "include", "src/hexdump.cpp", "-o"]
optimization = ["0", "1", "2", "3", "s", "fast", "g"]
test_args    = ["--ascii", tfile]

output = ""

# get some basic info on the file being read
tfile_path = path.abspath(tfile) 
tfile_size = stat(tfile).st_size

# get some basic info on this python file
pyfile_name = path.basename(__file__)
pyfile_dir = path.dirname(__file__) 

# get some basic info on hexdump
hexdump_ver = 'UNKNOWN' 
with open("src/hexdump.cpp", "r") as f:
    for line in f:
        if "hexdump_version" in line:
            ver_start = line.find('"') + 1
            ver_end = line.find('"', ver_start)
            hexdump_ver = line[ver_start:ver_end]
            break

output += f"""
{'='*80}
THIS PROGRAM ({pyfile_name}) IS STORED AT {pyfile_dir}
YOU ARE USING VERSION {hexdump_ver} OF HEXDUMP!

READING BYTES FROM {tfile}
\tPATH: {tfile_path}
\tSIZE: {tfile_size}
{'='*80}
"""

for idx, opt in enumerate(optimization):
    compile_command = base_command + [f"bin/test/hexdump-{opt}", f"-O{opt}"]
    t1 = time()
    sp.call(compile_command, stdout=sp.DEVNULL, stderr=sp.STDOUT)
    t2 = time()

    run_command = [f"bin/test/hexdump-{opt}"] + test_args
    t3 = time()
    sp.call(run_command, stdout=sp.DEVNULL, stderr=sp.STDOUT)
    t4 = time()
    
    size = stat("bin/test/hexdump-" + opt).st_size
    output += f"""
Optimization : {opt}
Compile Time : {t2 - t1} seconds
Size         : {size} bytes
File         : {"bin/test/hexdump-" + opt}
Run Time     : {t4 - t3} seconds
"""

    print(f"Finished test {idx + 1} (optimization: {opt})")

with open("test_results.txt", "a") as f:
    f.write(output + f"\n\n{'-'*80}\n\n")
