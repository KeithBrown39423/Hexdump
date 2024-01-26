const std = @import("std");
const error_handler = @import("./error_handling.zig");

const io   = std.io;
const fs   = std.fs;
const os   = std.os;
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

    const byteFmtFunction = switch (format) {
        Formats.ONE_BYTE_HEX => fmtHexByte,
        Formats.ONE_BYTE_DECIMAL => fmtDecByte,
        Formats.ONE_BYTE_OCTAL => fmtOctByte,
        Formats.ONE_BYTE_CHAR => fmtChrByte,
    };

    displayHeader(offset, &byteFmtFunction);
    const lines = math.divCeil(u64, bytes_remaining, 16) catch |err| {
        std.debug.panic("{!}", .{err});
    };
    const locationFmtFunction = switch (format) {
        Formats.ONE_BYTE_HEX => fmtLocationHex,
        Formats.ONE_BYTE_DECIMAL => fmtLocationDec,
        Formats.ONE_BYTE_OCTAL => fmtLocationOct,
        Formats.ONE_BYTE_CHAR => fmtLocationChr,
    };

    stdout.writeByteNTimes('\n', 2) catch |err| error_handler.writeError(err);

    for (0..lines) |line_index| {
        locationFmtFunction(@divFloor(offset + (line_index * 16), 16) * 16);

        for (0..(
            if (bytes_remaining < 8) bytes_remaining
            else 8 
        )) |_| {
            const byte = fstream.readByte() catch |err| error_handler.readError(err);
            bytes_remaining -= 1;
            // TODO: GRAY ASCII
            byteFmtFunction(byte);
        }

        stdout.writeByte(' ') catch |err| error_handler.writeError(err);

        for (0..(
            if (bytes_remaining < 8) bytes_remaining
            else 8 
        )) |_| {
            const byte = fstream.readByte() catch |err| error_handler.readError(err);
            bytes_remaining -= 1;
            // TODO: GRAY ASCII
            byteFmtFunction(byte);
        }

        stdout.writeByte('\n') catch |err| error_handler.writeError(err);
    }

    flush();
}

// TODO: Finish
fn fmtHexByte(byte: u8) void {
    stdout.print(" {x:0>2}", .{byte}) catch |err| error_handler.writeError(err);
}

fn fmtDecByte(byte: u8) void {
    stdout.print(" {d:0>3}", .{byte}) catch |err| error_handler.writeError(err);
}

fn fmtChrByte(byte: u8) void {
    if (byte < 32 or byte > 126) {
        stdout.print(" {d:0>3}", .{byte}) catch |err| error_handler.writeError(err);
    } else {
        stdout.print("  {c} ", .{byte}) catch |err| error_handler.writeError(err);
    }
}

fn fmtOctByte(byte: u8) void {
    stdout.print(" {o:0>3}", .{byte}) catch |err| error_handler.writeError(err);
}

fn fmtLocationHex(loc: u64) void {
    stdout.print("{x:0>8} ", .{loc}) catch |err| error_handler.writeError(err);
}

fn fmtLocationDec(loc: u64) void {
    stdout.print("{d:0>8} ", .{loc}) catch |err| error_handler.writeError(err);
}

fn fmtLocationChr(loc: u64) void {
    stdout.print("{d:0>8} ", .{loc}) catch |err| error_handler.writeError(err);
}

fn fmtLocationOct(loc: u64) void {
    stdout.print("{o:0>8} ", .{loc}) catch |err| error_handler.writeError(err);
}

fn displayHeader(offset: u64, fmtFunction: *const fn(u8) void) void {
    _ = stdout.write("  Offset ") catch |err| error_handler.writeError(err); // TODO: Pad this based on row offset
    const header_offset: u8 = @truncate(offset % 16);
    for (0..8) |i| {
        fmtFunction(@as(u8, @truncate(i)) + header_offset);
    }
    stdout.writeByte(' ') catch |err| error_handler.writeError(err);
    for (8..16) |i | {
        fmtFunction(@as(u8, @truncate(i)) + header_offset);
    }
}