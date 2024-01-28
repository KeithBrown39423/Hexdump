const std = @import("std");
const error_handler = @import("./error_handling.zig");

const io   = std.io;
const fs   = std.fs;
const os   = std.os;
const fmt  = std.fmt;
const math = std.math;

pub const stdout_handle = io.getStdOut();
pub var stdout_buffer = io.bufferedWriter(stdout_handle.writer());
pub const stdout = stdout_buffer.writer();
pub const stderr_handle = io.getStdErr();
pub const stderr = stderr_handle.writer();

fn flush() void {
    stdout_buffer.flush() catch |err| {
        error_handler.writeError(err);
    };
}

pub const Formats = enum {
    /// Default
    ONE_BYTE_HEX,
    ONE_BYTE_CHAR,
    ONE_BYTE_DECIMAL,
    ONE_BYTE_OCTAL,
};

pub fn main() void {
    // TODO: Parse Arguments
    // TODO: if (help or version) print and exit
    const format = Formats.ONE_BYTE_HEX; //*TMP*
    const filename = "/home/keith/Desktop/Projects/Hexdump/HexdumpNew/random.bin"; //*TMP*

    const file = fs.cwd().openFile(filename, .{}) catch |err| error_handler.openError(err);
    defer file.close();

    var buffered_file_reader = io.bufferedReader(file.reader());
    const fstream = buffered_file_reader.reader();

    const filesize = file.getEndPos() catch |err| {
        error_handler.getSeekPosError(err);
    };

    // TODO: ZARG ~ If offset option is supplied, `offset = opt` otherwise, `offset = 0`
    // TODO: ZARG ~ If length option is supplied, `length = opt` otherwise, `length = filesize`
    const offset = 0;
    if (offset > filesize) {
        _ = stderr.write("The specified offset is greater than the size of the file.") catch |err| error_handler.writeError(err); // TODO: Create error header and handle ansi coloring
        os.exit(1);
    }

    const length = filesize;

    var bytes_remaining: u64 = @min(length, filesize - offset);
    file.seekTo(offset) catch |err| error_handler.seekError(err);
    const opts_t = struct {
        width: u8,
        base: u8
    };
    const opts: opts_t = switch (format) {
        Formats.ONE_BYTE_HEX => .{ .width = 2, .base = 16 },
        Formats.ONE_BYTE_DECIMAL => .{ .width = 3, .base = 10 },
        Formats.ONE_BYTE_OCTAL => .{ .width = 3, .base = 8 },
        Formats.ONE_BYTE_CHAR => .{ .width = 3, .base = 10 },
    };

    displayHeader(offset, opts.base);
    const lines = math.divCeil(u64, bytes_remaining, 16) catch |err| {
        std.debug.panic("{!}", .{err});
    };

    stdout.writeByteNTimes('\n', 2) catch |err| error_handler.writeError(err);

    for (0..lines) |line_index| {
        fmt.formatInt(@divFloor(offset + (line_index * 16), 16) * 16, opts.base, fmt.Case.lower, .{
            .fill = '0',
            .width = 8
        }, stdout) catch |err| error_handler.writeError(err);
        var bytes: [16]u8 = undefined;
        var bytes_length: u8 = 0;
        var byte: u8 = undefined;

        for (0..(
            if (bytes_remaining < 8) bytes_remaining
            else 8 
        )) |_| {
            stdout.writeByte(' ') catch |err| error_handler.writeError(err);
            byte = fstream.readByte() catch |err| error_handler.readError(err);
            bytes[bytes_length] = byte;
            bytes_length += 1;
            bytes_remaining -= 1;
            // TODO: GRAY ASCII
            fmtByte(byte, opts.base, opts.width, format);
        }

        stdout.writeByte(' ') catch |err| error_handler.writeError(err);

        for (0..(
            if (bytes_remaining < 8) bytes_remaining
            else 8 
        )) |_| {
            stdout.writeByte(' ') catch |err| error_handler.writeError(err);
            byte = fstream.readByte() catch |err| error_handler.readError(err);
            bytes[bytes_length] = byte;
            bytes_length += 1;
            bytes_remaining -= 1;
            // TODO: GRAY ASCII
            fmtByte(byte, opts.base, opts.width, format);
        }

        stdout.writeByteNTimes(' ', 2) catch |err| error_handler.writeError(err);

        for (0..bytes_length) |i| {
            byte = bytes[i];

            if (byte < 32 or byte > 126) {
                stdout.writeByte('.') catch |err| error_handler.writeError(err);
            } else {
                stdout.print("{c}", .{byte}) catch |err| error_handler.writeError(err);
            }
        }

        stdout.writeByte('\n') catch |err| error_handler.writeError(err);
    }

    flush();
}

// TODO: Finish

fn fmtByte(byte: u8, base: u8, width: u8, format: Formats) void {
    if (format == Formats.ONE_BYTE_CHAR) {
        if (byte < 32 or byte > 126) {
            fmt.formatInt(byte, base, fmt.Case.lower, .{
                .fill = '0',
                .width = width
            }, stdout) catch |err| error_handler.writeError(err);
        } else {
            stdout.print(" {c} ", .{byte}) catch |err| error_handler.writeError(err);
        }
    } else {
        fmt.formatInt(byte, base, fmt.Case.lower, .{
            .fill = '0',
            .width = width
        }, stdout) catch |err| error_handler.writeError(err);
    }
}

fn displayHeader(offset: u64, base: u8) void {
    _ = stdout.write("  Offset ") catch |err| error_handler.writeError(err); // TODO: Pad this based on row offset
    const header_offset: u8 = @truncate(offset % 16);
    for (0..8) |i| {
        fmt.formatInt(@as(u8, @truncate(i)) + header_offset, base, fmt.Case.lower, .{
            .fill = '0',
            .width = 8
        }, stdout) catch |err| error_handler.writeError(err);
    }
    stdout.writeByte(' ') catch |err| error_handler.writeError(err);
    for (8..16) |i | {
        fmt.formatInt(@as(u8, @truncate(i)) + header_offset, base, fmt.Case.lower, .{
            .fill = '0',
            .width = 2
        }, stdout) catch |err| error_handler.writeError(err);
    }
    stdout.writeByte(' ') catch |err| error_handler.writeError(err);
}