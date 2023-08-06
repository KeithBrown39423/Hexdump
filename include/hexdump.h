#ifndef _HEXDUMP_H
#define _HEXDUMP_H

#include <filesystem>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>

#include <hex_errno.h>

#include <cxxopts.hpp>

using std::string;

using namespace std::chrono;
using namespace cxxopts;

namespace fs = std::filesystem;

extern const string hexdump_version;

typedef struct {
    unsigned int size;
    string filename;
    string absolute_path;
} filedata_t;

extern filedata_t filedata;

extern string error_header;

extern string binary_name;

extern bool output_color;

extern ParseResult options;

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

const unsigned int max_file_size = 0xFFFFFFFF;
const unsigned int large_file_size = 0x2000000;
extern string ansi_reset;
extern string offset_color;
extern string ascii_color;
extern bool output_color;
extern std::map<string, bool> progress_logged;
const int progress_log_points[] = { 25, 50, 75 };
extern unsigned int offset;

#endif // _HEXDUMP_H