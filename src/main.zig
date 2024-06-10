const std = @import("std");
const clap = @import("clap");

const fs = std.fs;

const VERSION = "v2.0.0";

const Formats = enum {
    /// Default
    ONE_BYTE_HEX,
    ONE_BYTE_CHAR,
    ONE_BYTE_DECIMAL,
    ONE_BYTE_OCTAL,
    TWO_BYTE_HEX,
    TWO_BYTE_DECIMAL,
    TWO_BYTE_OCTAL,
};

const args_struct = struct {
    file: []const u8,
    ascii: ?u8,
    skip: ?u64,
    length: ?u64,
    disable_color: ?u8,
    force_color: ?u8,
    squeeze: ?u8,
    format: Formats,
};

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer {
        const status = gpa.deinit();
        if (status != .ok) {
            std.log.err("Memory leak detected when deinitializing GPA", .{});
            std.process.exit(1);
        }
    }
    const a = gpa.allocator();

    const args = parse_args(a);
    const stdout_handle = std.io.getStdOut();
    var stdout_buffer = std.io.bufferedWriter(stdout_handle.writer());
    const stdout = stdout_buffer.writer();

    var stdout_config = std.io.tty.detectConfig(stdout_handle);
    if (args.disable_color != 0) stdout_config = .no_color;
    if (args.force_color != 0) stdout_config = .escape_codes;

    const file = fs.cwd().openFile(args.file, .{}) catch |err| {
        std.log.err("Failed to open file: {!}", .{err});
        std.process.exit(1);
    };
    a.free(args.file);
    defer file.close();

    var buffered_file_reader = std.io.bufferedReader(file.reader());
    const fstream = buffered_file_reader.reader();

    const filesize = file.getEndPos() catch |err| {
        std.log.err("Failed to get file size: {!}", .{err});
        std.process.exit(1);
    };

    const offset = args.skip orelse 0;
    if (offset > filesize) {
        std.log.err("The specified offset is greater than the size of the file", .{});
        std.process.exit(1);
    }

    if (offset != 0) file.seekTo(offset) catch |err| {
        std.log.err("Failed to seek to offset {d}: {!}", .{ offset, err });
        std.process.exit(1);
    };

    const opts_t = struct { width: u8, base: u8 };
    const opts: opts_t = switch (args.format) {
        Formats.ONE_BYTE_HEX => .{ .width = 2, .base = 16 },
        Formats.ONE_BYTE_CHAR => .{ .width = 3, .base = 10 },
        Formats.ONE_BYTE_DECIMAL => .{ .width = 3, .base = 10 },
        Formats.ONE_BYTE_OCTAL => .{ .width = 3, .base = 8 },
        Formats.TWO_BYTE_HEX => .{ .width = 4, .base = 16 },
        Formats.TWO_BYTE_DECIMAL => .{ .width = 5, .base = 10 },
        Formats.TWO_BYTE_OCTAL => .{ .width = 6, .base = 8 },
    };

    stdout_config.setColor(stdout, .blue) catch |err| {
        std.log.err("Failed to write color escape sequence: {!}", .{err});
        std.process.exit(1);
    };

    _ = stdout.write("  Offset: ") catch |err| {
        std.log.err("Failed to write offset header: {!}", .{err});
        std.process.exit(1);
    };

    const header_offset: u8 = @truncate(offset % 16);
    for (0..16) |i| {
        if (i == 8) {
            stdout.writeByte(' ') catch |err| {
                std.log.err("Failed to write offset header: {!}", .{err});
                std.process.exit(1);
            };
        }
        std.fmt.formatInt(@as(u8, @truncate(i)) + header_offset, opts.base, .lower, .{ .fill = '0', .width = opts.width }, stdout) catch |err| {
            std.log.err("Failed to write offset header: {!}", .{err});
            std.process.exit(1);
        };

        stdout.writeByte(' ') catch |err| {
            std.log.err("Failed to write offset header: {!}", .{err});
            std.process.exit(1);
        };
    }

    stdout_config.setColor(stdout, .reset) catch |err| {
        std.log.err("Failed to write color escape sequence: {!}", .{err});
        std.process.exit(1);
    };

    var length = args.length orelse (filesize - offset);

    const lines = std.math.divCeil(u64, length, 16) catch |err| {
        std.log.err("Failed to calculate number of lines: {!}", .{err});
        std.process.exit(1);
    };

    stdout.writeByte('\n') catch |err| {
        std.log.err("Failed to write newline: {!}", .{err});
        std.process.exit(1);
    };
    var previous_line: [16]u8 = undefined;

    for (0..@intCast(lines)) |line_index| {
        stdout_config.setColor(stdout, .blue) catch |err| {
            std.log.err("Failed to write color escape sequence: {!}", .{err});
            std.process.exit(1);
        };

        std.fmt.formatInt(@divFloor(offset + (line_index * 16), 16) * 16, opts.base, .lower, .{ .fill = '0', .width = 8 }, stdout) catch |err| {
            std.log.err("Failed to write offset: {!}", .{err});
            std.process.exit(1);
        };

        stdout.writeByte(':') catch |err| {
            std.log.err("Failed to write offset: {!}", .{err});
            std.process.exit(1);
        };

        stdout_config.setColor(stdout, .reset) catch |err| {
            std.log.err("Failed to write color escape sequence: {!}", .{err});
            std.process.exit(1);
        };

        var current_color: std.io.tty.Color = .reset;

        var bytes: [16]u8 = undefined;
        var bytes_length: u8 = 0;
        var byte: u8 = undefined;

        const bytes_in_line = @as(usize, @intCast(if (line_index * 16 >= filesize) 0 else if (length >= 16) 16 else length));

        const bytes_read = fstream.readAll(&bytes) catch |err| {
            std.log.err("Failed to read bytes: {!}", .{err});
            std.process.exit(1);
        };
        if (bytes_read != bytes_in_line) {
            std.log.err("Failed to read bytes: expected {d}, got {d}", .{ bytes_in_line, bytes_read });
            std.process.exit(1);
        }

        if (std.mem.eql(u8, &bytes, &previous_line) and args.squeeze != 0) {
            stdout_config.setColor(stdout, .yellow) catch |err| {
                std.log.err("Failed to write color escape sequence: {!}", .{err});
                std.process.exit(1);
            };
            _ = stdout.write(" *\n") catch |err| {
                std.log.err("Failed to write squeezed line: {!}", .{err});
                std.process.exit(1);
            };
            stdout_config.setColor(stdout, .reset) catch |err| {
                std.log.err("Failed to write color escape sequence: {!}", .{err});
                std.process.exit(1);
            };
            continue;
        } else {
            for (0..bytes_read) |i| {
                if (i == 8) {
                    _ = stdout.write(" ") catch |err| {
                        std.log.err("Failed to write byte separator: {!}", .{err});
                        std.process.exit(1);
                    };
                }

                stdout.writeByte(' ') catch |err| {
                    std.log.err("Failed to write byte separator: {!}", .{err});
                    std.process.exit(1);
                };

                byte = bytes[i];
                bytes[bytes_length] = byte;
                bytes_length += 1;
                length -= 1;

                if (byte == 0) {
                    if (current_color != .bright_black) {
                        current_color = .bright_black;
                        stdout_config.setColor(stdout, .bright_black) catch |err| {
                            std.log.err("Failed to write color escape sequence: {!}", .{err});
                            std.process.exit(1);
                        };
                    }
                } else if (current_color != .reset) {
                    current_color = .reset;
                    stdout_config.setColor(stdout, .reset) catch |err| {
                        std.log.err("Failed to write color escape sequence: {!}", .{err});
                        std.process.exit(1);
                    };
                }
                fmtByte(stdout, byte, opts.base, opts.width, args.format);

                previous_line[i] = byte;
            }
        }

        if (args.ascii != 0) {
            stdout_config.setColor(stdout, .green) catch |err| {
                std.log.err("Failed to write color escape sequence: {!}", .{err});
                std.process.exit(1);
            };

            for (bytes_length..16) |i| {
                if (i == 8) {
                    stdout.writeByte(' ') catch |err| {
                        std.log.err("Failed to write space: {!}", .{err});
                        std.process.exit(1);
                    };
                }
                stdout.writeByteNTimes(' ', opts.width + 1) catch |err| {
                    std.log.err("Failed to write space: {!}", .{err});
                    std.process.exit(1);
                };
            }

            _ = stdout.write("  ") catch |err| {
                std.log.err("Failed to write ASCII separator: {!}", .{err});
                std.process.exit(1);
            };

            for (0..bytes_length) |i| {
                byte = bytes[i];
                if (byte < 32 or byte > 126) {
                    if (byte == 0x00) {
                        stdout_config.setColor(stdout, .bright_black) catch |err| {
                            std.log.err("Failed to write color escape sequence: {!}", .{err});
                            std.process.exit(1);
                        };
                    }
                    stdout.writeByte('.') catch |err| {
                        std.log.err("Failed to write ASCII byte: {!}", .{err});
                        std.process.exit(1);
                    };
                    if (byte == 0x00) {
                        stdout_config.setColor(stdout, .green) catch |err| {
                            std.log.err("Failed to write color escape sequence: {!}", .{err});
                            std.process.exit(1);
                        };
                    }
                } else {
                    stdout.print("{c}", .{byte}) catch |err| {
                        std.log.err("Failed to write ASCII byte: {!}", .{err});
                        std.process.exit(1);
                    };
                }
            }
        }

        stdout.writeByte('\n') catch |err| {
            std.log.err("Failed to write newline: {!}", .{err});
            std.process.exit(1);
        };

        stdout_buffer.flush() catch |err| {
            std.log.err("Failed to flush stdout: {!}", .{err});
            std.process.exit(1);
        };
    }

    return;
}

fn parse_args(allocator: std.mem.Allocator) args_struct {
    const params = comptime clap.parseParamsComptime(
        \\<file>                  The file to print the hexdump of
        \\-a, --ascii             Show ASCII representation in column on the side
        \\-s, --skip <usize>      Skip reading the first <skip> bytes
        \\-n, --length <usize>    Only read <length> bytes
        \\-h, --help              Display this message
        \\-v, --version           Display the current version
        \\    --disable_color     Disables color output. The flag is also set if stdout is piped
        \\    --force_color       Forces colored output, even if stdout is piped. This takes priority over --disable-color
        \\    --squeeze           Show identical lines as *
        \\    --format <format>   Specify the output format
    );

    const parsers = comptime .{
        .usize = clap.parsers.string,
        .file = clap.parsers.string,
        .format = clap.parsers.string,
    };

    var diag = clap.Diagnostic{};
    var res = clap.parse(clap.Help, &params, parsers, .{
        .diagnostic = &diag,
        .allocator = allocator,
    }) catch |err| {
        diag.report(std.io.getStdErr().writer(), err) catch {
            std.log.err("Failed to report error", .{});
        };
        std.process.exit(1);
    };
    defer res.deinit();

    if (res.args.help != 0) {
        const helpWriter = std.io.getStdErr().writer();
        _ = helpWriter.write(
            \\Hexdump v2.0.0 ~ The alternative cross-platform hexdump utility
            \\
            \\Usage:
            \\    hexdump [options...] <file>
        ) catch |err| {
            std.log.err("Failed to write help message: {!}", .{err});
            std.process.exit(1);
        };

        _ = helpWriter.write(
            \\
            \\
            \\Options:
            \\
        ) catch |err| {
            std.log.err("Failed to write help message: {!}", .{err});
            std.process.exit(1);
        };

        clap.help(std.io.getStdErr().writer(), clap.Help, &params, .{ .description_on_new_line = false, .spacing_between_parameters = 0 }) catch |err| {
            std.log.err("Failed to write help message: {!}", .{err});
            std.process.exit(1);
        };

        _ = helpWriter.write(
            \\                                      * 1bhex: 1-byte hexadecimal / Default
            \\                                      * 1bchar: 1-byte character
            \\                                      * 1bdec: 1-byte decimal
            \\                                      * 1boct: 1-byte octal
            \\                                      * 2bhex: 2-byte hexadecimal
            \\                                      * 2bdec: 2-byte decimal
            \\                                      * 2boct: 2-byte octal
            \\
            \\ Note:
            \\   <usize> parameters can be followed by xxx suffixes.
            \\     Lowercase suffixes (k, m, g, ...) indicate a base of 1000, while
            \\     uppercase suffixes (K, M, G, ...) represent a base of 1024.
            \\
        ) catch |err| {
            std.log.err("Failed to write help message: {!}", .{err});
            std.process.exit(1);
        };

        std.process.exit(0);
        return .{};
    }

    if (res.args.version != 0) {
        _ = std.io.getStdOut().writer().print("Hexdump {s}\n", .{VERSION}) catch |err| {
            std.log.err("Failed to write version message: {any}", .{err});
            std.process.exit(1);
        };
        std.process.exit(0);
    }

    if (res.positionals.len != 1) {
        std.log.err("Expected exactly one positional argument, got {d}", .{res.positionals.len});
        std.process.exit(1);
    }

    const length_str: ?[]const u8 = res.args.length orelse null;
    var length: ?u64 = null;
    if (length_str != null) {
        length = parse_suffix(length_str.?);
        if (length == 0) {
            std.log.err("Invalid length: {s}", .{length_str.?});
            std.process.exit(1);
        }
    }

    const skip_str: ?[]const u8 = res.args.skip orelse null;
    var skip: ?u64 = null;
    if (skip_str != null) {
        skip = parse_suffix(skip_str.?);
        if (skip == 0) {
            std.log.err("Invalid skip: {s}", .{skip_str.?});
            std.process.exit(1);
        }
    }

    var format = Formats.ONE_BYTE_HEX;

    if (res.args.format != null) {
        const format_str = res.args.format orelse "";
        if (std.mem.eql(u8, format_str, "1bhex")) {
            format = Formats.ONE_BYTE_HEX;
        } else if (std.mem.eql(u8, format_str, "1bchar")) {
            format = Formats.ONE_BYTE_CHAR;
        } else if (std.mem.eql(u8, format_str, "1bdec")) {
            format = Formats.ONE_BYTE_DECIMAL;
        } else if (std.mem.eql(u8, format_str, "1boct")) {
            format = Formats.ONE_BYTE_OCTAL;
        } else if (std.mem.eql(u8, format_str, "2bhex")) {
            // TODO: v2.1.0 - format = Formats.TWO_BYTE_HEX;
            std.log.err("2-byte hexadecimal format is not supported yet. (Will be added in v2.1.0)", .{});
            std.process.exit(1);
        } else if (std.mem.eql(u8, format_str, "2bdec")) {
            // TODO: v2.1.0 - format = Formats.TWO_BYTE_DECIMAL;
            std.log.err("2-byte hexadecimal format is not supported yet. (Will be added in v2.1.0)", .{});
            std.process.exit(1);
        } else if (std.mem.eql(u8, format_str, "2boct")) {
            // TODO: v2.1.0 - format = Formats.TWO_BYTE_OCTAL;
            std.log.err("2-byte hexadecimal format is not supported yet. (Will be added in v2.1.0)", .{});
            std.process.exit(1);
        } else {
            std.log.err("Invalid format: {s}", .{format_str});
            std.process.exit(1);
        }
    }

    const file_str = allocator.alloc(u8, res.positionals[0].len) catch |err| {
        std.log.err("Failed to allocate memory: {!}", .{err});
        std.process.exit(1);
    };
    @memcpy(file_str, res.positionals[0].ptr);

    return .{
        .file = file_str,
        .ascii = res.args.ascii,
        .skip = skip,
        .length = length,
        .disable_color = res.args.disable_color,
        .force_color = res.args.force_color,
        .squeeze = res.args.squeeze,
        .format = format,
    };
}

fn parse_suffix(param: []const u8) u64 {
    if (param.len == 0) return 0;

    const suffix = param[param.len - 1];

    if (suffix >= '0' and suffix <= '9') {
        return std.fmt.parseInt(u64, param, 10) catch |err| {
            std.log.err("Failed to parse integer: {s}\nError: {!}", .{ param, err });
            std.process.exit(1);
        };
    }
    const modifier = std.fmt.parseInt(u64, param[0 .. param.len - 1], 10) catch |err| {
        std.log.err("Failed to parse integer: {s}\nError: {!}", .{ param[0 .. param.len - 1], err });
        std.process.exit(1);
    };

    if (suffix == 'k') {
        return modifier * 1000;
    } else if (suffix == 'm') {
        return modifier * 1000000;
    } else if (suffix == 'g') {
        return modifier * 1000000000;
    } else if (suffix == 't') {
        return modifier * 1000000000000;
    } else if (suffix == 'p') {
        return modifier * 1000000000000000;
    } else if (suffix == 'e') {
        return modifier * 1000000000000000000;
    } else if (suffix == 'K') {
        return modifier * 1024;
    } else if (suffix == 'M') {
        return modifier * 1048576;
    } else if (suffix == 'G') {
        return modifier * 1073741824;
    } else if (suffix == 'T') {
        return modifier * 1099511627776;
    } else if (suffix == 'P') {
        return modifier * 1125899906842624;
    } else if (suffix == 'E') {
        return modifier * 1152921504606846976;
    }

    return modifier;
}

fn fmtByte(stdout: anytype, byte: u8, base: u8, width: u8, format: Formats) void {
    if (format == Formats.ONE_BYTE_CHAR) {
        if (byte < 32 or byte > 126) {
            std.fmt.formatInt(byte, base, .lower, .{ .fill = '0', .width = width }, stdout) catch |err| {
                std.log.err("Failed to write byte: {d}: {any}", .{ byte, err });
                std.process.exit(1);
            };
        } else if (byte == '\n') {
            _ = stdout.write(" \\n") catch |err| {
                std.log.err("Failed to write byte: {d}: {any}", .{ byte, err });
                std.process.exit(1);
            };
        } else if (byte == '\r') {
            _ = stdout.write(" \\r") catch |err| {
                std.log.err("Failed to write byte: {d}: {any}", .{ byte, err });
                std.process.exit(1);
            };
        } else if (byte == '\t') {
            _ = stdout.write(" \\t") catch |err| {
                std.log.err("Failed to write byte: {d}: {any}", .{ byte, err });
                std.process.exit(1);
            };
        } else {
            stdout.print(" {c} ", .{byte}) catch |err| {
                std.log.err("Failed to write byte: {d}: {any}", .{ byte, err });
                std.process.exit(1);
            };
        }
    } else {
        std.fmt.formatInt(byte, base, .lower, .{ .fill = '0', .width = width }, stdout) catch |err| {
            std.log.err("Failed to write byte: {d}: {any}", .{ byte, err });
            std.process.exit(1);
        };
    }
}
