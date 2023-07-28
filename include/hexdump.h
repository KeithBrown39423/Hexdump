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

#endif // _HEXDUMP_H