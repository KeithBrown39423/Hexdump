#include <hexdump.h>
#include <log.h>

std::map<string, string> display_opts = {
    { "one-byte-hex", " %02X" },
    { "one-byte-octal", " %03o" },
    { "one-byte-decimal", " %03d" },
    { "two-byte-hex", " %04X" },
    { "two-byte-octal", " %06o" },
    { "two-byte-decimal", " %05d" }
};

std::map<string, int> display_lengths = {
    { "one-byte-hex", 3 },
    { "one-byte-octal", 4 },
    { "one-byte-decimal", 4 },
    { "two-byte-hex", 4 },
    { "two-byte-octal", 6 },
    { "two-byte-decimal", 5 }
};

char *display_type;
int display_length;

bool two_byte_mode = false;
unsigned int inc = 1;

bool is_big_endian() {
    union {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};

    return bint.c[0] == 1;
}

template <typename K, typename V>
std::vector<K> get_keys(const std::map<K, V>& m) {
    std::vector<K> result;
    result.reserve(m.size());

    for (typename std::map<K, V>::const_iterator itr = m.begin(); itr != m.end(); ++itr)
         result.push_back(itr->first);

    return result;
}

bool big_endian = false;

std::vector<unsigned char> prevline;
std::vector<unsigned char> identline; // line to identify duplicate lines
void output_hex_line(std::ostream& output, std::ifstream& input_stream, unsigned int offset) {
    const int bytes_per_line = 16;
    std::vector<unsigned char> line(bytes_per_line);
    unsigned char byte;
    input_stream.seekg(offset);
    for (int i = 0; i < bytes_per_line; i++) {
        if (input_stream.peek() != EOF) {
            input_stream >> std::noskipws >> byte;
            line[i] = byte;
        }
    }
    bool display_line = true;
    if (options.count("squash") && prevline == line) {
        if (identline != line) {
            identline = line;
            output << "*\n";
        }
        display_line = false;
    }

    if (options.count("squash")) prevline = line;

    if (display_line) {
        char offset_str[9];
        sprintf(offset_str, "%08X", offset);
        output << offset_color << offset_str << ":" << ansi_reset;
    }

    char byte_str[8];
    for (unsigned int i = 0; i < bytes_per_line; i += inc) {
        int percentage = static_cast<int>((offset + i) / static_cast<double>(filedata.size) * 100);
        for (int i : progress_log_points) {
            if (percentage >= i && !progress_logged[std::to_string(i)]) {
                log("Progress: " + std::to_string(i) + "%", LOG_INFO);
                progress_logged[std::to_string(i)] = true;
            }
        }
        if (!display_line) return;

        if (offset + i < filedata.size) {
            if (two_byte_mode) {
                if (big_endian) {
                    sprintf(byte_str, display_type, (line[i] << 8) | line[i + 1]);
                } else {
                    sprintf(byte_str, display_type, (line[i + 1] << 8) | line[i]);
                }
            } else {
                sprintf(byte_str, display_type, line[i]);
            }
            output << byte_str;
        } else {
            output << string(display_length, ' ');
            if (two_byte_mode && i + 1 == filedata.size) output << string(display_length, ' '); // pad last byte if necessary
        }
    }
    output << "   ";
    if (!options.count("ascii")) {
        output << "\n";
        return;
    };
    output << ascii_color;
    for (unsigned int i = 0; i < bytes_per_line; i++) {
        if (offset + i > filedata.size - 1) {
            output << " ";
            continue;
        };
        if (line[i] <= 32 || line[i] >= 126) {
            output << ".";
            continue;
        }
        output << line[i];
    }
    output << "\n";
}

void output_hexdump(std::ostream& output_stream, std::ifstream& input_stream) {   
    std::vector<string> keys;
    std::vector<string> opts = get_keys(display_opts);
    for (const KeyValue& option : options.arguments()) {
        if (std::find(opts.begin(), opts.end(), option.key()) != opts.end()) {
            keys.push_back(option.key());
        }
    }
    if (keys.size() == 0) {
        log("No display options specified. Defaulting to one-byte-hex.", LOG_INFO);
        keys.push_back("one-byte-hex");
    }
    string display_val = keys.back();
    display_type = (char*)display_opts[display_val].c_str();
    display_length = display_lengths[display_val];
    log("Display type: " + display_val, LOG_INFO);
    two_byte_mode = display_val.rfind("two-byte-", 0) == 0;
    if (two_byte_mode) inc = 2;
    auto hexdump_start = high_resolution_clock::now();

    big_endian = is_big_endian();

    string header = "  Offset:";
    int start = offset % 16;
    for (int i = start; i < (16 + start); i += inc) {
        char byte_str[8];
        sprintf(byte_str, display_type, i);
        header += byte_str;
    }

    output_stream << offset_color << header;
    if (options.count("ascii")) {
        output_stream << "   ";
        char ascii_offset[3];
        for (int i = start; i < (16 + start); i++) {
            sprintf(ascii_offset, "%02X", i % 16);
            output_stream << ascii_offset[1];
        }
    }
    output_stream << "\n";
    unsigned int increment = 16;
    for (unsigned int i = offset; i < filedata.size; i += increment) {
        output_hex_line(output_stream, input_stream, i);
        if (offset % 16 == 0) increment = 16; 
        else increment = 16 - (offset % 16);
    }
    output_stream << ansi_reset;

    auto hexdump_end = high_resolution_clock::now();
    auto hexdump_duration = duration_cast<milliseconds>(hexdump_end - hexdump_start).count();
    std::stringstream hexdump_duration_ss;
    hexdump_duration_ss << std::fixed << std::setprecision(3) << hexdump_duration / 1000.0;
    log("Hexdump duration: " + hexdump_duration_ss.str() + " seconds", LOG_INFO);
}