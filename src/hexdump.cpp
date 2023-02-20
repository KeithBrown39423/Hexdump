#include <cxxopts.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace cxxopts;

using std::cerr;
using std::cout;
using std::endl;
using std::string;

#define MAX_FILE_SIZE 0xFFFF
#define ERROR_HEADER "\x1b[1;31mError: \x1b[0m"

const string HEXDUMP_VERSION = "v1.0.0";

ParseResult initialize_options(int argc, char** argv);
string int_to_hex(int i, int len);

int main(int argc, char** argv) {
  ParseResult result = initialize_options(argc, argv);
  const string filename = result["file"].as<string>();

  std::ifstream input_stream;
  input_stream.open(filename, std::ios::binary | std::ios::in);

  if (!input_stream.is_open()) {
    cerr << ERROR_HEADER << "Could not open file '" << filename << "'" << endl;
    input_stream.clear();
    return EXIT_FAILURE;
  }

  input_stream.seekg(0, input_stream.end);
  const size_t file_size = input_stream.tellg();
  input_stream.seekg(0, input_stream.beg);

  if (file_size > MAX_FILE_SIZE) {
    cerr << ERROR_HEADER << "File is too big" << file_size << endl;
    exit(EXIT_FAILURE);
  } else if (file_size == 0) {
    cout << ERROR_HEADER << "File is empty" << endl;
    exit(EXIT_FAILURE);
  }

  char *buffer = (char *)malloc(file_size);
  input_stream.read(buffer, file_size);
  input_stream.close();
  unsigned char *file_content = (unsigned char *)buffer;

  bool output_color = true;
  if (result.count("output") || result.count("no-color")) output_color = false;

  if (result.count("output-color")) output_color = true;

  const string ansi_reset = !output_color ? "" : "\x1b[0m";
  const string offset_color = !output_color ? "" : "\x1b[38;2;0;144;255m";
  const string ascii_color = !output_color ? "" : "\x1b[38;2;0;144;48m";

  std::stringstream output = std::stringstream();

  output << offset_color << "  Offset: 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F"
         << endl << ansi_reset;

  for (size_t i = 0; i < (((file_size - 1) / 16) + 1); i++) {
    output << offset_color << "    " << int_to_hex(i * 16, 4) << ": " << ansi_reset;
    for (int j = 0; j < 16; j++) {
      if (i * 16 + j < file_size) {
        output << int_to_hex(file_content[i * 16 + j], 2) << " ";
      } else {
        output << "   ";
      }
    }

    if (result.count("ascii")) {
      output << "   " << ascii_color;

      char c;
      for (size_t k = 0; k < 16; k++) {
        if ((size_t)(i * 16 + k) < file_size) {
          c = file_content[i * 16 + k];
          if (c < 33 || c > 126) {
            output << ".";
          } else {
            output << c;
          }
        }
      }
    }

    output << ansi_reset << endl;
  }

  if (result.count("output")) {
    string output_file = result["output"].as<string>();

    std::ofstream output_stream = std::ofstream();
    output_stream.open(output_file, std::ios::binary);
    output_stream << output.str() << endl;
    output_stream.close();
  } else {
    cout << output.str() << endl;
  };

  return 0;
}

ParseResult initialize_options(int argc, char** argv) {
  Options options = Options(argv[0], "A simple hexdump utility");

  options
      .positional_help("<filename>")
      .show_positional_help()
      .set_width(80)
      .set_tab_expansion()
      .allow_unrecognised_options()
      .add_options()
        ("h,help", "Display help message and exit")
        ("v,version", "Display program information and exit")
        ("a,ascii", "Display ascii equivalent")
        ("o,output", "Print to OUTPUT instead of stdout", value<string>(), "OUTPUT")
        ("output-color", "Write color escape codes to output files")
        ("no-color", "Disable color output")
        ("file", "File to dump hex content of", value<string>(), "FILE");

  options.parse_positional({"file"});

  ParseResult result = options.parse(argc, argv);

  if (result.count("help")) {
    cout << options.help() << endl;
    exit(0);
  }

  if (result.count("version")) {
    cout << HEXDUMP_VERSION << endl;
    exit(0);
  }

  if (!result.count("file")) {
    cerr << ERROR_HEADER << "No file specified" << endl
         << endl
         << "Usage: hexdump [option]... [file]" << endl
         << "Try 'hexdump --help' for more information." << endl;

    exit(EXIT_FAILURE);
  }

  return result;
}

string int_to_hex(int i, int len) {
  std::stringstream stream;
  stream << std::setfill('0') << std::setw(len)
         << std::hex << std::uppercase << i;
  return stream.str();
}
