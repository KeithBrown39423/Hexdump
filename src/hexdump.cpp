#include <hexdump_errno.h>

#include <ctime>
#include <cxxopts.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

// Check windows
#ifdef _WIN32
#include <windows.h>
#endif

using namespace cxxopts;

using std::string;
namespace fs = std::filesystem;

const string hexdump_version = "v1.1.0";
const size_t max_file_size = 0xFFFFFFFF;
const size_t max_log_count = 128;

string binary_name;

string error_header = "\x1b[1;38;2;255;0;0mError: \x1b[0m";

string ansi_reset = "\x1b[0m";
string offset_color = "\x1b[38;2;0;144;255m";
string ascii_color = "\x1b[38;2;0;144;48m";

enum log_level {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

const string enum_vals[] = {
    "INFO",
    "WARN",
    "ERROR"
};

ParseResult initialize_options(int argc, char** argv);
string int_to_hex(int value, int width);
void outputHexLine(std::ostream& output, std::vector<unsigned char> buffer, size_t offset, size_t size, int ascii);
void log(string message, log_level level = LOG_INFO, bool output = false);

int main(int argc, char** argv) {
    ParseResult result = initialize_options(argc, argv);
    const string filename = result["file"].as<string>();

    if (!fs::exists(filename)) {
        std::cerr << error_header << "File '" + filename + "' does not exist";
        return HEX_ENOENT;
    }

    std::ifstream input_stream(filename, std::ios::binary | std::ios::in);

    if (!input_stream.is_open()) {
        std::cerr << error_header << "Failed to open file '" << filename << "'" << std::endl;
        return HEX_EIO;
    }

    input_stream.seekg(0, input_stream.end);
    const size_t file_size = input_stream.tellg();
    input_stream.seekg(0, input_stream.beg);

    if (file_size > max_file_size) {
        std::cerr << error_header << "File size is too large" << std::endl;
        return HEX_EFBIG;
    } else if (file_size == 0) {
        std::cerr << error_header << "File is empty" << std::endl;
        return HEX_EFEMPTY;
    }

    std::vector<unsigned char> buffer;
    unsigned char byte;
    while (input_stream >> std::noskipws >> byte) {
        buffer.push_back(byte);
    }

    std::stringstream output = std::stringstream();

    output << offset_color
           << "  Offset: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  ";
    if (result.count("ascii")) {
        output << "0123456789ABCDEF";
    }
    output << ansi_reset << "\n";

    for (size_t i = 0; i < file_size; i += 16) {
        outputHexLine(output, buffer, i, file_size, result.count("ascii"));
    }

    if (result.count("output")) {
        const string output_filename = result["output"].as<string>();
        std::ofstream output_stream;
        output_stream.open(output_filename, std::ios::out);
        output_stream << output.str() << std::endl;
        output_stream.close();
        std::cout << "Wrote output to " << output_filename << std::endl;
    } else {
        std::cout << output.str() << std::endl;
    }

    return 0;
}

ParseResult initialize_options(int argc, char** argv) {
    binary_name = fs::path(argv[0]).filename().string();

    Options options(binary_name, "A simple hexdump utility\n");

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

    ParseResult result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(EXIT_SUCCESS);
    }

    if (result.count("version")) {
        std::cout << "Hexdump " << hexdump_version << std::endl;
        exit(EXIT_SUCCESS);
    }
    if ((result.count("output-color") &&
         result["output-color"].as<string>() == "false") ||
        (result.count("output") && !(result.count("output-color") &&
                                     result["output-color"].as<string>() == "true"))) {
        ansi_reset = "";
        offset_color = "";
        ascii_color = "";
        error_header = "Error: ";
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
        }
    }
#endif

    if (!result.count("file")) {
        std::cerr << error_header << "Missing required argument: file\nUsage: "
                  << binary_name << " [options...] <file>\nTry `" << binary_name
                  << " --help` for more information." << std::endl;

        exit(HEX_EMISARG);
    }

    return result;
}

string int_to_hex(int value, int width) {
    std::stringstream stream = std::stringstream();
    stream << std::hex << std::uppercase << std::setfill('0') << std::setw(width) << value;
    return stream.str();
}

void outputHexLine(std::ostream& output, std::vector<unsigned char> buffer, size_t offset, size_t size, int ascii) {
    const int bytes_per_line = 16;
    output << offset_color << int_to_hex(offset, 8) << ": " << ansi_reset;
    for (size_t i = 0; i < bytes_per_line; i++) {
        if (offset + i < size) {
            output << int_to_hex(buffer[offset + i], 2) << " ";
        } else {
            output << "   ";
        }
    }
    output << " ";
    if (!ascii) {
        output << ansi_reset << "\n";
        return;
    }
    for (size_t i = 0; i < bytes_per_line; i++) {
        output << ascii_color;
        if (offset + i > size - 1) {
            output << " ";
            continue;
        };
        if (buffer[offset + i] <= 32 || buffer[offset + i] >= 126) {
            output << ".";
            continue;
        }
        output << buffer[offset + i];
    }
    output << ansi_reset << "\n";
}

string timestamp() {
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y-%m-%d, %H:%M:%S", tm_info);

    return string(buffer);
}

void log(string message, log_level level, bool output) {
    string home_directory;
#ifdef _WIN32
    home_directory = string(std::getenv("HOMEDRIVE")) + string(std::getenv("HOMEPATH"));
#else
    home_directory = std::getenv("HOME");
#endif
    fs::path log_path = fs::path(home_directory)
                            .append("." + binary_name);

    if (!fs::exists(log_path)) {
        fs::create_directory(log_path);
    }

    log_path = log_path.append("logs");

    if (!fs::exists(log_path)) {
        fs::create_directory(log_path);
    }

    if (!fs::is_directory(log_path)) {
        // TODO: Implement failsafe if log_path cannot be created
    }

    fs::path log = log_path.append(binary_name + ".log");

    string log_message = "[" + timestamp() + "] [" + enum_vals[level] + "]: " + message;

    std::ofstream log_stream = std::ofstream(log, std::ios::out | std::ios::app);
    log_stream << log_message << std::endl;
    log_stream.close();

    if (output) {
        if (level == LOG_ERROR) {
            std::cerr << error_header << message << std::endl;
        } else {
            std::cout << message << std::endl;
        }
    }
}