from time import time
import subprocess as sp
from os import stat, path, remove, listdir, devnull 
import sys
from concurrent.futures import ProcessPoolExecutor, as_completed

# TODO: refactor code flow (mainly with test execution)
#       possibly remove verbose logging? (cleans code)

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

# TODO: work off abs not rel paths

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

#-=-=-==-==-=-=-#
# FILE HANDLING #
#-=-=-==-==-=-=-#
# FILE #
file_handling.add_argument(
    "-f", "--file",
    metavar="FILE",
    action="store",
    default=path.join(pyfile_dir, pyfile_name),
    help="File to read bytes from during binary tests (defaults to this file)"
)

# LOG #
file_handling.add_argument(
    "-l", "--log",
    metavar="LOG",
    action="store",
    default="test_results.txt",
    help="Output file that tests are stored to (default: ./test_results.txt)"
)

# BINARY OUT #
file_handling.add_argument(
    "-b", "--bin-out",
    metavar="PATH",
    action="store",
    default="bin/test",
    help="Place to store test binary outputs (default: ./bin/test)"
)

#--==--==--==--==--#
#  TEST EXECUTION  #
#--==--==--==--==--#
# COMP AVG #
test_execution.add_argument(
    "--compavg",
    metavar="N",
    action="store",
    type=int,
    help="Compute the average of all N tests"
)

# AVG RUNTIME #
test_execution.add_argument(
    "--avg-runtime",
    metavar="N",
    action="store",
    type=int,
    help="Compute the average runtimes over N tests (compiles hexdump binaries asynchronously)"
)

# TEST ALL #
test_execution.add_argument(
    "-t", "--test-all",
    action="store_true",
    help="Test all optimizations"
)

#--==--==--==--==--==--#
#  CONTROL AND OUTPUT  #
#--==--==--==--==--==--#
# SILENT #
control_and_output.add_argument(
    "--silent",
    action="store_true",
    help="Redirect stdout to DEVNULL (supress output)"
)

# VERBOSE #
control_and_output.add_argument(
    "--verbose",
    action="store_true",
    help="Display detailed processing information"
)

# CLEAN #
control_and_output.add_argument(
    "--clean",
    action="store_true",
    help="Delete all test binaries"
)

# parse args
args = parser.parse_args()

#############
# Constants #
#############
ERROR = 3
WARNING = 2
VERBOSE = 1
INFO = 0

PYFILE_NAME = path.basename(__file__)
PYFILE_DIR = path.dirname(__file__)

BASE_COMPILE_COMMAND = ["g++", "-Wall", "-Werror", "-Wpedantic", "-I",
                        "lib", "-I", "include", "src/log.cpp", "src/hexdump.cpp", "-o"]
OPTIMIZATION_LEVELS  = ["0", "1", "2", "3", "s", "fast", "g"]
TEST_ARGS            = ["--ascii", args.file]

###########
# LOGGING #
###########
def log(type: int, *posargs, **kwargs):
    # don't output verbose logs if flag isn't set
    if type == VERBOSE and not args.verbose: return
    
    status = ""
    if   type == INFO    : status = f"[{INFO_COLOR}INFO{RESET_COLOR}]"
    elif type == VERBOSE : status = f"[{VERBOSE_COLOR}VERBOSE{RESET_COLOR}]"
    elif type == WARNING : status = f"[{WARNING_COLOR}WARNING{RESET_COLOR}]"
    elif type == ERROR   : status = f"[{ERROR_COLOR}ERROR{RESET_COLOR}]"
    else                 : status = "[UNKNOWN]"
    print(status, *posargs, **kwargs)


#######################
# arg integrity check #
#######################
if not path.exists(args.file):
    log(ERROR, f"{args.file} DOES NOT EXIST!")
    sys.exit(1)

if not path.exists(args.bin_out):
    log(ERROR, f"{args.bin_out} DOES NOT EXIST!")
    sys.exit(1)

if args.silent:
    f = open(devnull, 'w')
    sys.stdout = f

###########################
####  END OF ARGPARSE  ####
###########################

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

class TestHandler:
    #==--   --==#
    #  COMPILE  # ~ quick
    #==--   --==#
    @staticmethod
    def quick_compile():
        """ should not be used for tracking comptime """
        start = time()
        with ProcessPoolExecutor() as executor:
            compile_futures = {executor.submit(TestHandler.compile, BASE_COMPILE_COMMAND + [path.join(args.bin_out, f"hexdump-{opt}"), f"-O{opt}"]): opt for opt in OPTIMIZATION_LEVELS}

            for future in as_completed(compile_futures):
                opt = compile_futures[future]
                try:
                    t1, t2 = future.result()
                    log(INFO, f"Compiled binary... took {t2 - t1} seconds")
                except Exception as exc:
                    log(ERROR, f"An error occurred while compiling a binary: {exc}")
        end = time()
        log(VERBOSE, "Final time taken:", end-start)

    #==--   --==#
    #  COMPILE  #
    #==--   --==#
    @staticmethod
    def compile(compile_command):
        log(VERBOSE, f"compile_command = {compile_command}")
        start = time()
        log(VERBOSE, "running compile_command")
        try:
            sp.call(compile_command, stdout=sp.DEVNULL, stderr=sp.STDOUT)
        except Exception as exc:
            log(ERROR, f"An error occurred while compiling: {exc}")
        else:
            log(VERBOSE, "compile_command executed")
        end = time()
        
        return (start, end)
    
    #==-- --==#
    #   RUN   #
    #==-- --==#
    @staticmethod
    def run(run_command):
        log(VERBOSE, f"run_command = {run_command}")
        start = time()
        log(VERBOSE, "running run_command")
        sp.call(run_command, stderr=sp.STDOUT)
        log(VERBOSE, "run_command executed")
        end = time()
    
        return (start, end)
    
    #==--  --==#
    # RUN TEST #
    #==--  --==#
    @staticmethod
    def run_test(opt):
        start = time()
        file_name = path.join(args.bin_out, f"hexdump-{opt}")
        log(VERBOSE, f"file_name = {file_name}")

        #----------------#
        # COMPILE BINARY #
        #----------------#
        compile_command = BASE_COMPILE_COMMAND + [file_name, f"-O{opt}"]
        t1, t2 = TestHandler.compile(compile_command)

        #------------#
        # RUN BINARY #
        #------------#
        run_command = [file_name] + TEST_ARGS
        t3, t4 = TestHandler.run(run_command)

        log(VERBOSE, f"crunching test data into dict")
        test = {
            'opt': opt,
            'comptime': t2 - t1,
            'size': stat(file_name).st_size,
            'file': file_name,
            'runtime': t4 - t3
        }
        log(VERBOSE, f"crunched dict:\n{test}")
        end = time()
        log(VERBOSE, "Final time taken:", end-start)

        return test

def test_2_pretty(test: dict) -> str:
    return f"""
Optimization : {test['opt']}
Compile Time : {test['comptime']} seconds
Size         : {test['size']} bytes
File         : {test['file']}
Run Time     : {test['runtime']} seconds
    """

############
# TEST ALL #
############
if args.test_all:
    with open(args.log, 'a') as f:
        f.write(log_header)

        for idx, opt in enumerate(OPTIMIZATION_LEVELS):
            f.write(test_2_pretty(TestHandler.run_test(opt)))
            log(INFO, f"Finished test {idx + 1} (optimization: {opt})")
        
        f.write(f"\n\n{'-'*80}\n\n")


#########
# CLEAN #
#########
if args.clean:
    log(VERBOSE, "searching for test binaries")
    binaries = [f for f in listdir(args.bin_out) if any("hexdump-"+opt in f for opt in OPTIMIZATION_LEVELS)]
    if len(binaries) == 0:
        log(VERBOSE, "no test binaries")
        sys.exit(0)

    log(VERBOSE, f"found {binaries}")

    for bin in binaries:
        log(VERBOSE, f"removing {bin}")
        remove(path.join(args.bin_out, bin))
        log(VERBOSE, "removed")
    
    sys.exit(0)

###################
# COMPUTE AVERAGE #
###################
if args.compavg:
    log(VERBOSE, f"args.compavg = {args.compavg}")

    tests = []

    log(INFO, f"0/{args.compavg} tests...")
    for i in range(args.compavg):
        for opt in OPTIMIZATION_LEVELS:
            tests.append(TestHandler.run_test(opt))
        log(INFO, f"{i+1}/{args.compavg} Done")

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
    with open(args.log, 'a') as f:
        f.write(log_header+f"COMPAVG OVER THE SPAN OF {args.compavg} TESTS\n\n")

        for opt, opt_dict in avg_dict.items():
            f.write(f"Optimization: {opt}\navg_comptime: {opt_dict['avg_comptime']}\navg_runtime: {opt_dict['avg_runtime']}\n\n")
        f.write(f"Overall:\navg_comptime: {total_avg_comptime}\navg_runtime: {total_avg_runtime}\n")

        f.write(f"\n\n{'-'*80}\n\n")
    sys.exit(0)

###############
# AVG RUNTIME #
###############
if args.avg_runtime:
    TestHandler.quick_compile()

    log(VERBOSE, "calulate and sort runtimes")
    runtimes = {}
    for _ in range(args.avg_runtime):
        for opt in OPTIMIZATION_LEVELS:
            file_name = path.join(args.bin_out, f"hexdump-{opt}")
            t1, t2 = TestHandler.run([file_name] + TEST_ARGS)
            runtimes.setdefault(opt, []).append(t2 - t1)

    avg_dict = {}
    log(VERBOSE, "get runtime averages by optimization")
    for opt, opt_runtimes in runtimes.items():
        log(VERBOSE, f"opt = {opt}")
        avg_runtime = sum(runtime for runtime in opt_runtimes) / len(opt_runtimes)
        log(VERBOSE, f"avg_runtime = {avg_runtime}")
        log(VERBOSE, "Crunching data into dict & adding it to avg_dict")
        avg_dict[opt] = {'avg_runtime': avg_runtime}
        
    log(VERBOSE, "computing the average comptime and runtime across all optimizations")
    total_avg_runtime = sum(opt_dict['avg_runtime'] for opt_dict in avg_dict.values()) / len(avg_dict)

    log(VERBOSE, "writing to log file")
    with open(args.log, 'a') as f:
        f.write(log_header+f"AVG_RUNTIME OVER THE SPAN OF {args.avg_runtime} TESTS\n\n")

        for opt, opt_dict in avg_dict.items():
            f.write(f"Optimization: {opt}\navg_runtime: {opt_dict['avg_runtime']}\n\n")
        f.write(f"Overall:\navg_runtime: {total_avg_runtime}\n")

        f.write(f"\n\n{'-'*80}\n\n")
    
    sys.exit(0)

log(WARNING, "Please supply a test to execute!")
