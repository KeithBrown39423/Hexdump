#ifdef _WIN32
#include <cwindows>
#endif

#include <hexdump.h>
#include <display.h>
#include <log.h>

#include <cstdio>
#include <chrono>

const string hexdump_version = "1.2.0";
string error_header = "\x1b[1;38;2;255;0;0mError: \x1b[0m";
string binary_name = "";

filedata_t filedata;

string ansi_reset = "\x1b[0m";
string offset_color = "\x1b[38;2;0;144;255m";
string ascii_color = "\x1b[38;2;0;144;48m";

bool output_color = true;

std::map<string, bool> progress_logged;

unsigned int offset;

ParseResult options;
void initialize_options(int argc, char** argv);

int main(int argc, char** argv) {
    auto program_start = high_resolution_clock::now();
    binary_name = fs::path(argv[0]).filename().string();
    initialize_log();

    initialize_options(argc, argv);

    filedata.filename = options["file"].as<string>();

    if (!fs::exists(filedata.filename)) {
        std::cerr << error_header << "File '" + filedata.filename + "' does not exist";
        log("File '" + filedata.filename + "' does not exist", LOG_ERROR);
        exit_hexdump(HEX_ENOENT);
    }

    std::ifstream input_stream(filedata.filename, std::ios::binary | std::ios::in);

    if (!input_stream.is_open()) {
        std::cerr << error_header << "Failed to open file '" << filedata.filename << "'" << std::endl;
        log("Failed to open file '" + filedata.filename + "'", LOG_ERROR);
        exit_hexdump(HEX_EIO);
    }

    input_stream.seekg(0, input_stream.end);
    filedata.size = input_stream.tellg();
    input_stream.seekg(0, input_stream.beg);

    if (filedata.size > max_file_size) {
        std::cerr << error_header << "File size is too large" << std::endl;
        log("File size is too large", LOG_ERROR);
        exit_hexdump(HEX_EFBIG);
    } else if (filedata.size == 0) {
        std::cerr << error_header << "File is empty" << std::endl;
        log("File is empty", LOG_WARN);
        exit_hexdump(HEX_EFEMPTY);
    }

    filedata.absolute_path = fs::absolute(filedata.filename).string();

    append_execution_info();

    if (filedata.size > large_file_size) {
        log("File size is large. Execution might take a long time", LOG_WARN);
    }

    for (int i : progress_log_points) {
        progress_logged[std::to_string(i)] = false;
    }

    if (options.count("offset")) {
        offset = options["offset"].as<unsigned int>();
    } else {
        offset = 0;
    }

    if (offset > filedata.size) {
        std::cerr << error_header << "Offset is larger than file size" << std::endl;
        log("Offset is larger than file size", LOG_ERROR);
        exit_hexdump(HEX_EFBIG);
    }

    if (options.count("length")) {
        unsigned int length = options["length"].as<unsigned int>() + offset;
        if (length > filedata.size) {
            log("Length is larger than file size", LOG_WARN);
        } else {
            filedata.size = length;
        }
    }

    if (options.count("output")) {
        if (!output_color) {
            log("Disabling color output for file output", LOG_INFO);
        }
        const string output_filename = options["output"].as<string>();
        std::ofstream output_stream;
        output_stream.open(output_filename, std::ios::out);
        output_hexdump(output_stream, input_stream);
        output_stream.close();
        std::cout << "Wrote output to " << output_filename << std::endl;
        log("Wrote output to " + output_filename, LOG_INFO);
    } else {
        output_hexdump(std::cout, input_stream);
    }

    auto program_end = high_resolution_clock::now();

    auto program_duration = duration_cast<milliseconds>(program_end - program_start).count();

    std::stringstream program_duration_ss;
    program_duration_ss << std::fixed << std::setprecision(3) << program_duration / 1000.0;

    log("Program duration: " + program_duration_ss.str() + " seconds", LOG_INFO);
    log("Exiting with code 0", LOG_INFO);

    exit_hexdump(HEX_RETURN);

    return 0;
}

void initialize_options(int argc, char** argv) {
    Options choices(binary_name, "An alternative cross platfrom hex dumping utility\n");

    choices.custom_help("[options...]")
        .positional_help("<file>")
        .show_positional_help()
        .set_width(80)
        .set_tab_expansion()
        .allow_unrecognised_options();
    choices.add_options("Options")
            ("h,help", "Display help message and exit")
            ("v,version", "Display program information and exit");
    choices.add_options("Display")
            ("a,ascii", "Display ascii equivalent")
            ("m,squash", "Squash duplicate lines into one line")
            ("b,one-byte-octal", "Display data in one-byte octal format")
            ("c,one-byte-decimal", "Display data in one-byte decimal format")
            ("x,two-byte-hex", "Display data in two-byte hex format")
            ("t,two-byte-octal", "Display data in two-byte octal format")
            ("d,two-byte-decimal", "Display data in two-byte decimal format")
            ("output-color", "Write color escape codes", value<string>()->default_value("true"), "true|false");
    choices.add_options("Output")
            ("n,length", "The amount of bytes to dump", value<unsigned int>()->default_value("0"), "length")
            ("s,offset", "The offset to start dumping from", value<unsigned int>()->default_value("0"), "offset")
            ("o,output", "Print to output instead of stdout. Note: Disables color", value<string>(), "output");
    choices.add_options("File")
            ("file", "File to be dumped", value<string>(), "filename");

    choices.parse_positional({"file"});

    try {
        options = choices.parse(argc, argv);
    } catch (std::exception& e) {
        string message = e.what();
        std::cerr << error_header << message << std::endl;
        log("Exception: ", LOG_ERROR, message);
        exit_hexdump(HEX_EINVAL);
    }

    if (options.count("help")) {
        std::cout << choices.help({"Options", "Display", "Output"}) << std::endl;
        log("Called with --help", LOG_INFO);
        exit_hexdump(HEX_OK);
    }

    if (options.count("version")) {
        std::cout << "Hexdump " << hexdump_version << std::endl;
        log("Called with --version", LOG_INFO);
        exit_hexdump(HEX_OK);
    }
    if ((options.count("output-color") &&
         options["output-color"].as<string>() == "false") ||
        (options.count("output") && !(options.count("output-color") &&
                                     options["output-color"].as<string>() == "true"))) {
        ansi_reset = "";
        offset_color = "";
        ascii_color = "";
        error_header = "Error: ";
        output_color = false;
    }
#ifdef _WIN32
    else {
        DWORD dwMode = 0;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleMode(hConsole, &dwMode);
        DWORD dwNewMode = dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hConsole, dwNewMode);
        GetConsoleMode(hConsole, &dwMode);

        if (dwMode == dwNewMode) {
            log("Enabled VT100 escape codes", LOG_INFO);
        } else {
            log("Failed to enable VT100 escape codes", LOG_WARN);
            ansi_reset = "";
            offset_color = "";
            ascii_color = "";
            error_header = "Error: ";
            output_color = false;
        }
        auto hexdump_end = high_resolution_clock::now();
        auto hexdump_duration = duration_cast<milliseconds>(hexdump_end - hexdump_start).count();
        std::stringstream hexdump_duration_ss;
        hexdump_duration_ss << std::fixed << std::setprecision(3) << hexdump_duration / 1000.0;
        log("Hexdump duration: " + hexdump_duration_ss.str() + " seconds", LOG_INFO);
    }
#endif

    if (!options.count("file")) {
        std::cerr << error_header << "Missing required argument: file\nUsage: "
                  << binary_name << " [options...] <file>\nTry `" << binary_name
                  << " --help` for more information." << std::endl;
        log("Missing required argument: file", LOG_ERROR);
        exit_hexdump(HEX_EMISARG);
    }
}