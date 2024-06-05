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
};

pub fn main() !void {
    _ = parse_args();
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
            \\  hexdump 
        ) catch |err| help_error(err);

        clap.usage(std.io.getStdErr().writer(), clap.Help, &params) catch |err| help_error(err);

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

    return .{
        .file = res.positionals[0],
        .ascii = res.args.ascii,
        .skip = res.args.skip orelse -1,
        .length = res.args.length orelse -1,
        .disable_color = res.args.disable_color,
        .force_color = res.args.force_color,
        .squeeze = res.args.squeeze,
    };
}

fn help_error(err: anytype) noreturn {
    std.log.err("Failed to write help message: {any}", .{ err });
    std.process.exit(1);
}