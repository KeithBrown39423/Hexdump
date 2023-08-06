#include <hexdump.h>

#include <ctime>

const string header_divider = string(80, '=');
const string end_divider = string(80, '-');
const size_t max_log_count = 48;

std::ofstream out_stream;

bool output_logs = true;

#ifdef _WIN32
string home_directory = string(std::getenv("HOMEDRIVE")) + string(std::getenv("HOMEPATH"));
#else
string home_directory = string(std::getenv("HOME"));
#endif

string timestamp() {
    time_t timer;
    char buffer[26];
    struct tm* tm_info;

    timer = time(NULL);
    tm_info = localtime(&timer);

    strftime(buffer, 26, "%Y-%m-%d, %H:%M:%S", tm_info);

    return string(buffer);
}

fs::path log_path;

void initialize_log() {
    std::ifstream log_stream;

#ifdef _WIN32
    // If the user's home directory is not set, or they are runing
    // as administrator, use the Program Files directory instead.
    if (home_directory == "") {
        string home_drive = std::getenv("HOMEDRIVE");
        if (home_drive == "") {
            home_drive = "C:";
        }
        fs::path location = fs::path(home_drive)
                                .append("Program Files")
                                .append("Hexdump");
        if (!fs::exists(location)) {
            fs::create_directory(location);
            if (!fs::is_directory(location)) {
                output_logs = false;
                std::cerr << "Failed to create log directory" << std::endl;
                std::cerr << "Log output will be disabled" << std::endl;
                return;
            }
            home_directory = location.string();
        }
    }
#endif

    log_path = fs::path(home_directory)
                   .append("." + binary_name);

    if (!fs::exists(log_path)) {
        fs::create_directory(log_path);
        if (!fs::is_directory(log_path)) {
            output_logs = false;
            std::cerr << "Failed to create log directory" << std::endl;
            std::cerr << "Log output will be disabled" << std::endl;
            return;
        }
    }

    log_path.append(binary_name + ".log");
    
    if (!fs::exists(log_path)) {
        std::ofstream tmp(log_path, std::ios::out | std::ios::trunc);
        tmp.close();
    }

    log_stream = std::ifstream(log_path, std::ios::in);
    log_stream.seekg(0, log_stream.beg);

    if (!log_stream.is_open()) {
        output_logs = false;
        std::cerr << "Failed to access logs" << std::endl;
        std::cerr << "Log output will be disabled" << std::endl;
        return;
    }

    std::vector<string> buffer;
    string line;
    while (std::getline(log_stream, line)) {
        buffer.push_back(line);
    }

    std::vector<size_t> log_indexes;
    for (size_t i = 0; i < buffer.size(); i++) {
        if (buffer[i] == end_divider) {
            log_indexes.push_back(i);
        }
    }

    size_t log_count = log_indexes.size();

    if (log_count > max_log_count) {
        size_t overflow = log_count - max_log_count;
        buffer.erase(buffer.begin(), buffer.begin() + log_indexes[overflow] + 1);
    }

    out_stream = std::ofstream(log_path, std::ios::out | std::ios::trunc);

    for (size_t i = 0; i < buffer.size(); i++) {
        out_stream << buffer[i] << "\n";
    }

    out_stream << header_divider << "\n"
               << std::endl;

    log_stream.close();
    out_stream.close();

    out_stream = std::ofstream(log_path, std::ios::out | std::ios::app);
}

void append_execution_info() {
    if (!output_logs) return;

    out_stream << "HEXDUMP V" << hexdump_version << "\n\n"
               << "File: " << filedata.filename << "\n"
               << "Size: " << filedata.size << " bytes\n"
               << "Path: " << filedata.absolute_path << "\n\n"
               << "Options: "
               << "\n";

    const std::vector<KeyValue> option_names = options.arguments();

    std::vector<string> option_keys;
    std::vector<string> option_values;

    for (const KeyValue& option : option_names) {
        if (option.key() == "output-color") {
            option_keys.push_back("output-color");
            if (output_color) {
                option_values.push_back("true");
            } else {
                option_values.push_back("false");
            }
            continue;
        } else if (option.key() == "file") {
            continue;
        }

        option_keys.push_back(option.key());
        option_values.push_back(option.value());
    }

    size_t max_key_length = 0;
    for (const string& key : option_keys) {
        if (key.length() > max_key_length) {
            max_key_length = key.length();
        }
    }

    std::vector<string> option_lines;
    for (const string& key : option_keys) {
        std::stringstream ss;
        ss << std::left << std::setw(max_key_length) << key;
        option_lines.push_back(ss.str());
    }

    for (size_t i = 0; i < option_lines.size(); i++) {
        out_stream << "  " << option_lines[i] << " = " << option_values[i] << "\n";
    }

    out_stream << "\n"
               << header_divider << "\n"
               << std::endl;
}

void log(string message, log_level level, string other = "") {
    if (!output_logs) return;

    string log_message = "[" + timestamp() + "] [" + enum_vals[level] + "]: " + message;
    out_stream << log_message << std::endl;
    if (other != "") {
        out_stream << other << std::endl;
    }
}

void exit_hexdump(hex_errno error_code) {
    if (output_logs) {
        out_stream << "\n"
                   << end_divider << std::endl;

    }

    out_stream.close();

    if (error_code == HEX_RETURN) return;

    exit(error_code);
}