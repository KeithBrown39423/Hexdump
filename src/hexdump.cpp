#ifdef _WIN32
#include <windows.h>
#endif

#include <hexdump.h>
#include <log.h>

#include <cstdio>
#include <chrono>

const string hexdump_version = "1.2.0";
string error_header = "\x1b[1;38;2;255;0;0mError: \x1b[0m";
string binary_name = "";

filedata_t filedata;

const unsigned int max_file_size = 0xFFFFFFFF;
const unsigned int large_file_size = 0x2000000;

string ansi_reset = "\x1b[0m";
string offset_color = "\x1b[38;2;0;144;255m";
string ascii_color = "\x1b[38;2;0;144;48m";

bool output_color = true;

ParseResult initialize_options(int argc, char** argv);
void output_hex_line(std::ostream& output, std::ifstream& input_stream, unsigned int offset, unsigned int size, int ascii);
void output_hexdump(std::ostream& output_stream, std::ifstream& input_stream, int ascii);

int main(int argc, char** argv) {
    auto program_start = std::chrono::high_resolution_clock::now();
    binary_name = fs::path(argv[0]).filename().string();
    initialize_log();

    ParseResult result = initialize_options(argc, argv);

    filedata.filename = result["file"].as<string>();

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

    append_execution_info(result);

    if (filedata.size > large_file_size) {
        log("File size is large. Execution might take a long time", LOG_WARN);
    }

    if (result.count("output")) {
        if (!output_color) {
            log("Disabling color output for file output", LOG_INFO);
        }
        const string output_filename = result["output"].as<string>();
        std::ofstream output_stream;
        output_stream.open(output_filename, std::ios::out);
        auto hexdump_start = std::chrono::high_resolution_clock::now();
        output_hexdump(output_stream, input_stream, result.count("ascii"));
        auto hexdump_end = std::chrono::high_resolution_clock::now();
        auto hexdump_duration = std::chrono::duration_cast<std::chrono::milliseconds>(hexdump_end - hexdump_start).count();
        std::stringstream hexdump_duration_ss;
        hexdump_duration_ss << std::fixed << std::setprecision(3) << hexdump_duration / 1000.0;
        log("Hexdump duration: " + hexdump_duration_ss.str() + " seconds", LOG_INFO);
        output_stream.close();
        std::cout << "Wrote output to " << output_filename << std::endl;
        log("Wrote output to " + output_filename, LOG_INFO);
    } else {
        auto hexdump_start = std::chrono::high_resolution_clock::now();
        output_hexdump(std::cout, input_stream, result.count("ascii"));
        auto hexdump_end = std::chrono::high_resolution_clock::now();
        auto hexdump_duration = std::chrono::duration_cast<std::chrono::milliseconds>(hexdump_end - hexdump_start).count();
        std::stringstream hexdump_duration_ss;
        hexdump_duration_ss << std::fixed << std::setprecision(3) << hexdump_duration / 1000.0;
        log("Hexdump duration: " + hexdump_duration_ss.str() + " seconds", LOG_INFO);
    }

    auto program_end = std::chrono::high_resolution_clock::now();

    auto program_duration = std::chrono::duration_cast<std::chrono::milliseconds>(program_end - program_start).count();

    std::stringstream program_duration_ss;
    program_duration_ss << std::fixed << std::setprecision(3) << program_duration / 1000.0;

    log("Program duration: " + program_duration_ss.str() + " seconds", LOG_INFO);
    log("Exiting with code 0", LOG_INFO);

    exit_hexdump(HEX_RETURN);

    return 0;
}

ParseResult initialize_options(int argc, char** argv) {
    Options options(binary_name, "An alternative cross platfrom hex dumping utility\n");

    options.custom_help("[options...]")
        .positional_help("<file>")
        .show_positional_help()
        .set_width(80)
        .set_tab_expansion()
        .allow_unrecognised_options()
        .add_options()
            ("h,help", "Display help message and exit")
            ("v,version", "Display program information and exit")
            ("a,ascii", "Display ascii equivalent")
            ("o,output", "Print to OUTPUT instead of stdout", value<string>(), "OUTPUT")
            ("output-color", "Write color escape codes", value<string>()->default_value("true"), "true|false")
            ("file", "File to be dumped", value<string>(), "filename");

    options.parse_positional({"file"});

    ParseResult result;
    try {
        result = options.parse(argc, argv);
    } catch (std::exception& e) {
        string message = e.what();
        std::cerr << error_header << message << std::endl;
        log("Exception: ", LOG_ERROR, message);
        exit_hexdump(HEX_EINVAL);
    }

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        log("Called with --help", LOG_INFO);
        exit_hexdump(HEX_OK);
    }

    if (result.count("version")) {
        std::cout << "Hexdump " << hexdump_version << std::endl;
        log("Called with --version", LOG_INFO);
        exit_hexdump(HEX_OK);
    }
    if ((result.count("output-color") &&
         result["output-color"].as<string>() == "false") ||
        (result.count("output") && !(result.count("output-color") &&
                                     result["output-color"].as<string>() == "true"))) {
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
    }
#endif

    if (!result.count("file")) {
        std::cerr << error_header << "Missing required argument: file\nUsage: "
                  << binary_name << " [options...] <file>\nTry `" << binary_name
                  << " --help` for more information." << std::endl;
        log("Missing required argument: file", LOG_ERROR);
        exit_hexdump(HEX_EMISARG);
    }

    return result;
}

void output_hex_line(std::ostream& output, std::ifstream& input_stream, unsigned int offset, unsigned int size, int ascii) {
    const int bytes_per_line = 16;
    std::vector<unsigned char> buffer(bytes_per_line);
    unsigned char byte;
    input_stream.seekg(offset);
    for (int i = 0; i < bytes_per_line; i++) {
        if (input_stream.peek() != EOF) {
            input_stream >> std::noskipws >> byte;
            buffer[i] = byte;
        }
    }
    char offset_str[9];
    sprintf(offset_str, "%08X", offset);
    output << offset_color << offset_str << ": " << ansi_reset;

    char byte_str[3];
    for (unsigned int i = 0; i < bytes_per_line; i++) {
        if (offset + i == (unsigned int)(size * .25)) {
            log("25% complete", LOG_INFO);
        } else if (offset + i == (unsigned int)(size * .50)) {
            log("50% complete", LOG_INFO);
        } else if (offset + i == (unsigned int)(size * .75)) {
            log("75% complete", LOG_INFO);
        }
        if (offset + i < size) {
            sprintf(byte_str, "%02X", buffer[i]);
            output << byte_str << " ";
        } else {
            output << "   ";
        }
    }
    output << "   ";
    if (!ascii) return;
    output << ascii_color;
    for (unsigned int i = 0; i < bytes_per_line; i++) {
        if (offset + i > size - 1) {
            output << " ";
            continue;
        };
        if (buffer[i] <= 32 || buffer[i] >= 126) {
            output << ".";
            continue;
        }
        output << buffer[i];
    }
    output << "\n";
}

void output_hexdump(std::ostream& output_stream, std::ifstream& input_stream, int ascii) {
    output_stream << offset_color
           << "  Offset: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F";
    if (ascii) {
        output_stream << "    0123456789ABCDEF";
    }
    output_stream << "\n";
    for (unsigned int i = 0; i < filedata.size; i += 16) {
        output_hex_line(output_stream, input_stream, i, filedata.size, ascii);
    }
    output_stream << ansi_reset;
}