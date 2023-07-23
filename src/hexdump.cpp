#include <hexdump_errno.h>

#include <cxxopts.hpp>
#include <filesystem>
#include <iterator>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace cxxopts;

using std::string;

const string hexdump_version = "v1.1.0";
const unsigned int max_file_size = 0xFFFFFFFF;

const string error_header = "\x1b[1;38;2;255;0;0mError: \x1b[0m";

string ansi_reset = "\x1b[0m";
string offset_color = "\x1b[38;2;0;144;255m";
string ascii_color = "\x1b[38;2;0;144;48m";

bool output_color = true;

ParseResult initialize_options(int argc, char** argv);
string int_to_hex(int value, int width);
void outputHexLine(std::ostream& output, std::vector<unsigned char> buffer, size_t offset, size_t size, int ascii);

int main(int argc, char** argv) {
    ParseResult result = initialize_options(argc, argv);
    const string filename = result["filename"].as<string>();

    std::ifstream input_stream(filename, std::ios::binary | std::ios::in);

    if (!input_stream.is_open()) {
        std::cerr << error_header << "Could not open file '" << filename << "'" << std::endl;
        return EIO;
    }

    input_stream.seekg(0, input_stream.end);
    const size_t file_size = input_stream.tellg();
    input_stream.seekg(0, input_stream.beg);

    if (file_size > max_file_size) {
        std::cerr << "Error: File is too big" << std::endl;
        return HEX_EFBIG;
    } else if (file_size == 0) {
        std::cerr << "Error: File is empty" << std::endl;
        return HEX_EFEMPTY;
    }

    std::vector<unsigned char> buffer;
    unsigned char byte;
    while (input_stream >> std::noskipws >> byte) {
        buffer.push_back(byte);
    }

    if (!output_color) {
        ansi_reset = "";
        offset_color = "";
        ascii_color = "";
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
    Options options(
        std::filesystem::path{argv[0]}.filename(),
        "A simple hexdump utility\n"
    );

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

    if (!result.count("filename")) {
        std::cout << error_header << "No file specified\n"
         << "\n"
         << "Usage: hexdump [options...] <file>\n"
         << "Try `hexdump --help` for more information." << std::endl;
        exit(HEX_EMISARG);
    }

    if ((result.count("output-color") &&
        result["output-color"].as<string>() == "false") ||
        result.count("output")
    ) {
        output_color = false;
    }

    if ((result.count("output-color") &&
        result["output-color"].as<string>() == "true")
    ) {
        output_color = true;
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
