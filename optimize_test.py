from time import time
import subprocess as sp
from os import stat, path, remove, listdir, devnull
import sys

# empty unless colorama is installed
ERROR_COLOR = ''
WARNING_COLOR = ''
VERBOSE_COLOR = ''
INFO_COLOR = ''
RESET_COLOR = ''

try:
    from colorama import Fore, Back, Style, init
    init()

    is_colorama = True

    ERROR_COLOR   = Style.BRIGHT + Fore.RED
    WARNING_COLOR = Style.BRIGHT + Fore.YELLOW
    VERBOSE_COLOR = Style.BRIGHT + Fore.MAGENTA
    INFO_COLOR    = Style.BRIGHT + Fore.CYAN
    RESET_COLOR   = Style.RESET_ALL

except ImportError:
    pass

from argparse import ArgumentParser, HelpFormatter

# TODO: error checking
#       work off abs not rel paths

# get some basic info on this python file
pyfile_name = path.basename(__file__)
pyfile_dir = path.dirname(__file__) 

###########################
#### START OF ARGPARSE ####
###########################
class CustomHelpFormatter(HelpFormatter):
    def _format_action_invocation(self, action):
        if not action.option_strings or action.nargs == 0:
            return super()._format_action_invocation(action)
        else:
            parts = []
            if action.option_strings:
                parts.extend(action.option_strings)
            if action.metavar:
                parts[-1] += ' %s' % action.metavar
            return ', '.join(parts)

# argparse init
parser = ArgumentParser(
    prog=pyfile_name,
    description="Tests optimization levels",
    formatter_class=CustomHelpFormatter
)

file_handling = parser.add_argument_group("File Handling")
test_execution = parser.add_argument_group("Test Execution")
control_and_output = parser.add_argument_group("Control and Output")

#--==--#
# FILE #
#--==--#
file_handling.add_argument(
    "-f", "--file",
    metavar="FILE",
    action="store",
    default=path.join(pyfile_dir, pyfile_name),
    help="File to read bytes from during binary tests (defaults to this file)"
)

#--==--#
# LOG! #
#--==--#
file_handling.add_argument(
    "-l", "--log",
    metavar="LOG",
    action="store",
    default="test_results.txt",
    help="Output file that tests are stored to (default: ./test_results.txt)"
)

#--==--==--==--#
#  BINARY OUT  #
#--==--==--==--#
file_handling.add_argument(
    "-b", "--bin-out",
    metavar="PATH",
    action="store",
    default="bin/test",
    help="Place to store test binary outputs (default: ./bin/test)"
)

#--==--==--#
#  CLEAN!  #
#--==--==--#
control_and_output.add_argument(
    "--clean",
    action="store_true",
    help="Delete all test binaries"
)

#--==--==--#
# COMP AVG #
#--==--==--#
test_execution.add_argument(
    "--compavg",
    metavar="N",
    action="store",
    type=int,
    help="Compute the average of all N tests"
)

#--==--==--#
# TEST ALL #
#--==--==--#
test_execution.add_argument(
    "-t", "--test-all",
    action="store_true",
    help="Test all optimizations"
)

#--==--==--#
#  SILENT  #
#--==--==--#
control_and_output.add_argument(
    "--silent",
    action="store_true",
    help="Redirect stdout to DEVNULL (supress output)"
)

#--==--==--==--#
# VERBOSE MODE #
#--==--==--==--#
control_and_output.add_argument(
    "--verbose",
    action="store_true",
    help="Display detailed processing information"
)

# parse args
args = parser.parse_args()
###########################
####  END OF ARGPARSE  ####
###########################

if args.silent:
    f = open(devnull, 'w')
    sys.stdout = f

base_command = ["g++", "-Wall", "-Werror", "-Wpedantic", "-I", "lib", "-I", "include", "src/hexdump.cpp", "-o"]
optimization = ["0", "1", "2", "3", "s", "fast", "g"]
test_args    = ["--ascii", args.file]

ERROR   = 3
WARNING = 2
VERBOSE = 1
INFO    = 0

def log(type: int, *posargs, **kwargs):
    if type == INFO:
        print(f"[{INFO_COLOR}INFO{RESET_COLOR}]", *posargs, **kwargs)
    elif type == VERBOSE and args.verbose:
        print(f"[{VERBOSE_COLOR}VERBOSE{RESET_COLOR}]", *posargs, **kwargs)
    elif type == WARNING:
        print(f"[{WARNING_COLOR}WARNING{RESET_COLOR}]", *posargs, **kwargs)
    elif type == ERROR:
        print(f"[{ERROR_COLOR}ERROR{RESET_COLOR}]", *posargs, **kwargs)
    else:
        print("[UNKNOWN]", *posargs, **kwargs)

#==--         --==#
# HEXDUMP VERSION #
#==--         --==#
def hexdump_ver() -> str:
    hexdump_ver = 'UNKNOWN' 
    with open("src/hexdump.cpp", "r") as f:
        for line in f:
            if "hexdump_version" in line:
                ver_start = line.find('"') + 1
                ver_end = line.find('"', ver_start)
                return line[ver_start:ver_end]


#==--  --==#
# RUN TEST #
#==--  --==#
def run_test(opt, base_command=base_command, test_args=test_args) -> dict:
    file_name = path.join(args.bin_out, f"hexdump-{opt}")
    log(VERBOSE, f"file_name = {file_name}")

    #----------------#
    # COMPILE BINARY #
    #----------------#
    compile_command = base_command + [file_name, f"-O{opt}"]
    log(VERBOSE, f"compile_command = {compile_command}")
    t1 = time()
    log(VERBOSE, "running compile_command")
    sp.call(compile_command, stdout=sp.DEVNULL, stderr=sp.STDOUT)
    log(VERBOSE, "compile_command executed")
    t2 = time()

    #------------#
    # RUN BINARY #
    #------------#
    run_command = [file_name] + test_args
    log(VERBOSE, f"run_command = {run_command}")
    t3 = time()
    log(VERBOSE, "running run_command")
    sp.call(run_command, stdout=sp.DEVNULL, stderr=sp.STDOUT)
    log(VERBOSE, "run_command executed")
    t4 = time()
    
    log(VERBOSE, f"crunching test data into dict")
    test = {
        'opt': opt,
        'comptime': t2 - t1,
        'size': stat(file_name).st_size,
        'file': file_name,
        'runtime': t4 - t3
    }
    log(VERBOSE, f"crunched dict:\n{test}")
    return test

def test_2_pretty(test: dict) -> str:
    return f"""
Optimization : {test['opt']}
Compile Time : {test['comptime']} seconds
Size         : {test['size']} bytes
File         : {test['file']}
Run Time     : {test['runtime']} seconds
    """

# header slapped at the top of each log
log_header = f"""
{'='*80}
THIS PROGRAM ({pyfile_name}) IS STORED AT {pyfile_dir}
YOU ARE USING VERSION {hexdump_ver()} OF HEXDUMP!

READING BYTES FROM {path.basename(args.file)}
\tPATH: {path.abspath(args.file)}
\tSIZE: {stat(args.file).st_size}
{'='*80}
"""


#########
# CLEAN #
#########
if args.clean:
    log(VERBOSE, "searching for test binaries")
    binaries = [f for f in listdir(args.bin_out) if any("hexdump-"+opt in f for opt in optimization)]
    if len(binaries) == 0:
        log(VERBOSE, "no test binaries")
        exit(0)

    log(VERBOSE, f"found {binaries}")

    for bin in binaries:
        log(VERBOSE, f"removing {bin}")
        remove(path.join(args.bin_out, bin))
        log(VERBOSE, "removed")
    
    exit(0)

###################
# COMPUTE AVERAGE #
###################
if args.compavg:
    log(VERBOSE, f"args.compavg = {args.compavg}")

    tests = []

    for _ in range(args.compavg):
        for opt in optimization:
            tests.append(run_test(opt))

    log(VERBOSE, "sorting tests by optimization")
    lists_dict = {}
    for d in tests:
        log(VERBOSE, f"sorting test:\n{d}")
        lists_dict.setdefault(d['opt'], []).append(d)

    log(VERBOSE, "get comptime and runtime averages by optimization")
    avg_dict = {}
    for opt, opt_tests in lists_dict.items():
        log(VERBOSE, f"opt = {opt}")
        avg_comptime = sum(test['comptime'] for test in opt_tests) / len(opt_tests)
        log(VERBOSE, f"avg_comptime = {avg_comptime}")
        avg_runtime = sum(test['runtime'] for test in opt_tests) / len(opt_tests)
        log(VERBOSE, f"avg_runtime = {avg_runtime}")
        log(VERBOSE, "Crunching data into dict & adding it to avg_dict")
        avg_dict[opt] = {'avg_comptime': avg_comptime, 'avg_runtime': avg_runtime}

    log(VERBOSE, "computing the average comptime and runtime across all optimizations")
    total_avg_comptime = sum(opt_dict['avg_comptime'] for opt_dict in avg_dict.values()) / len(avg_dict)
    total_avg_runtime = sum(opt_dict['avg_runtime'] for opt_dict in avg_dict.values()) / len(avg_dict)

    log(VERBOSE, "writing to log file")
    with open(args.log, "a") as f:
        f.write(log_header+f"COMPAVG OVER THE SPAN OF {args.compavg} TESTS\n\n")

        for opt, opt_dict in avg_dict.items():
            f.write(f"Optimization: {opt}\navg_comptime: {opt_dict['avg_comptime']}\navg_runtime: {opt_dict['avg_runtime']}\n\n")
        f.write(f"Overall:\navg_comptime: {total_avg_comptime}\navg_runtime: {total_avg_runtime}\n")

        f.write(f"\n\n{'-'*80}\n\n")
    exit(0)

############
# TEST ALL #
############
if args.test_all:
    with open(args.log, "a") as f:
        f.write(log_header)

        for idx, opt in enumerate(optimization):
            f.write(test_2_pretty(run_test(opt)))
            log(INFO, f"Finished test {idx + 1} (optimization: {opt})")
        
        f.write(f"\n\n{'-'*80}\n\n")
    exit(0)

