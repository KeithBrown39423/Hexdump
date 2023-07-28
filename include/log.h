#ifndef _LOG_H
#define _LOG_H

#include <hexdump.h>

void initialize_log();
void append_execution_info(ParseResult options);
void log(string message, log_level level, string other = "");
void exit_hexdump(hex_errno error_code);

#endif // _LOG_H