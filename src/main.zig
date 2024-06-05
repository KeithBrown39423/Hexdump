const std = @import("std");
const clap = @import("clap");

const args_struct = struct {
    file: ?[]const u8,
    ascii: ?u8,
    skip: ?i64,
    length: ?i64,
    disable_color: ?u8,
    force_color: ?u8,
    squeeze: ?u8,
    one_byte_char: ?u8,
    one_byte_decimal: ?u8,
    one_byte_octal: ?u8,
    two_byte_decimal: ?u8,
    two_byte_octal: ?u8,
    two_byte_hex: ?u8,
};

pub fn main() !void {
    var args = parse_args();
    const stdout = std.io.getStdOut();
    if (!stdout.isTty()) {
        args.disable_color = 1;
    }

    if (args.force_color != 0) {
        args.disable_color = 0;
    }

    std.debug.print("Args: {?any}\n", .{args.disable_color});
    return;
}

fn parse_args() args_struct {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer {
        const status = gpa.deinit();
        if (status != .ok) {
            std.log.err("Memory leak detected when deinitializing GPA for argument parsing", .{});
            std.process.exit(1);
        }
    }

    const params = comptime clap.parseParamsComptime(
        \\<file>                  The file to print the hexdump of
        \\-a, --ascii             Show ASCII representation in column on the side
        \\-s, --skip <i64>        Skip reading the first <offset> bytes
        \\-n, --length <i64>      Only read <length> bytes
        \\-h, --help              Display this message
        \\-v, --version           Display the current version
        \\    --disable_color     Disables color output. The flag is also set if stdout is piped
        \\    --force_color       Forces colored output, even if stdout is piped. This takes priority over --disable-color
        \\    --squeeze           Show identical lines as *
        \\    --one_byte_char     Display as ASCII character, escape code string ('\n', '\t', etc.), or as DEC
        \\    --one_byte_decimal  Display as decimal digit
        \\    --one_byte_octal    Display as octal digit
        \\    --two_byte_decimal  Display as decimal digit (two bytes)
        \\    --two_byte_octal    Display as octal digit (two bytes)
        \\    --two_byte_hex      Display as hexadecimal digit (two bytes)
    );

    const parsers = comptime .{
        .i64 = clap.parsers.int(i64, 10),
        .file = clap.parsers.string,
    };

    var diag = clap.Diagnostic{};
    var res = clap.parse(clap.Help, &params, parsers, .{
        .diagnostic = &diag,
        .allocator = gpa.allocator(),
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
        ) catch |err| help_error(err);

        _ = helpWriter.write(
            \\
            \\
            \\Options:
            \\
        ) catch |err| help_error(err);

        clap.help(std.io.getStdErr().writer(), clap.Help, &params, .{
            .description_on_new_line = false,
            .spacing_between_parameters = 0
        }) catch |err| help_error(err);

        _ = helpWriter.write(
            \\
            \\ Arguments:
            \\   <length> and <offset> arguments can be followed by xxx suffixes.
            \\     Lowercase suffixes (k, m, g, ...) indicate a base of 1000, while
            \\     uppercase suffixes (K, M, G, ...) represent a base of 1024.
            \\
        ) catch |err| help_error(err);

        std.process.exit(0);
        return .{};
    }

    if (res.args.version != 0) {
        _ = std.io.getStdOut().write("Hexdump v2.0.0\n") catch |err| {
            std.log.err("Failed to write version message: {any}", .{ err });
            std.process.exit(1);
        };
        std.process.exit(0);
    }

    if (res.positionals.len != 1) {
        std.log.err("Expected exactly one positional argument, got {d}", .{res.positionals.len});
        std.process.exit(1);
    }

    return .{
        .file = res.positionals[0],
        .ascii = res.args.ascii,
        .skip = res.args.skip orelse -1,
        .length = res.args.length orelse -1,
        .disable_color = res.args.disable_color,
        .force_color = res.args.force_color,
        .squeeze = res.args.squeeze,
        .one_byte_char = res.args.one_byte_char,
        .one_byte_decimal = res.args.one_byte_decimal,
        .one_byte_octal = res.args.one_byte_octal,
        .two_byte_decimal = res.args.two_byte_decimal,
        .two_byte_octal = res.args.two_byte_octal,
        .two_byte_hex = res.args.two_byte_hex,
    };
}

fn help_error(err: anytype) noreturn {
    std.log.err("Failed to write help message: {any}", .{ err });
    std.process.exit(1);
}