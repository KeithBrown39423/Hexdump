const std = @import("std");
const main = @import("./main.zig");

const io = std.io;
const fs = std.fs;
const os = std.os;

const stderr_handle = io.getStdErr();
var stderr = stderr_handle.writer();

const Errno = enum {
    OK,
    ENOENT,
    EIO,
    EINVAL,
    EISDIR,
    ESPIPE,
    EPIPE,
    ENAMETOOLONG,
    ELOOP,
    ECONNRESET,
    ENOBUFS
};

pub fn writeError(err: fs.File.WriteError) noreturn {
    std.debug.panic("{s}", .{ switch (err) {
        error.DiskQuota => "Disk quota exceeded during write operation.",
        error.FileTooBig => "File size exceeds the allowed limit.",
        error.InputOutput => "Input/output error occurred.",
        error.NoSpaceLeft => "No space left on device.",
        error.DeviceBusy => "Device or resource is busy.",
        error.InvalidArgument => "Invalid argument.",
        error.AccessDenied => "Access denied.",
        error.BrokenPipe => "Broken pipe.",
        error.SystemResources => "Insufficient system resources.",
        error.OperationAborted => "Operation aborted.",
        error.NotOpenForWriting => "Not open for writing.",
        error.LockViolation => "Lock violation.",
        error.WouldBlock => "Operation would block.",
        error.ConnectionResetByPeer => "Connection reset by peer.",
        else => "Unexpected Error."
    }});
}

pub fn openError(err: fs.File.OpenError) noreturn {
    std.debug.panic("{s}", .{ switch (err) {
        error.AccessDenied => "Access denied",
        error.BadPathName => "Bad path name.",
        error.DeviceBusy => "Device is busy.",
        error.FileBusy => "File is busy.",
        error.FileLocksNotSupported => "File locks not supported.",
        error.FileNotFound => "File not found.",
        error.FileTooBig => "File size exceeds the limit.",
        error.InvalidHandle => "Invalid handle.",
        error.InvalidUtf8 => "Invalid UTF-8 encoding.",
        error.IsDir => "Is a directory, not a file.",
        error.NameTooLong => "File name is too long.",
        error.NetworkNotFound => "Network not found.",
        error.NoDevice => "No device found.",
        error.NoSpaceLeft => "No space left on the device.",
        error.NotDir => "Not a directory.",
        error.PathAlreadyExists => "Path already exists.",
        error.PipeBusy => "Pipe is busy.",
        error.ProcessFdQuotaExceeded => "Process file descriptor quota exceeded.",
        error.SharingViolation => "Sharing violation.",
        error.SymLinkLoop => "Symbolic link forms a loop.",
        error.SystemFdQuotaExceeded => "System file descriptor quota exceeded.",
        error.SystemResources => "Insufficient system resources.",
        error.WouldBlock => "Operation would block.",
        else => "Unexpected Error."
    }});
}

pub fn getSeekPosError(err: fs.File.GetSeekPosError) noreturn {
    std.debug.panic("{s}", .{
        switch (err) {
            error.Unseekable => "The operation cannot be performed on an unseekable entity.",
            error.AccessDenied => "Access Denied.",
            error.SystemResources => "Insufficient system resources.",
            else => "Unexpected Error."
        }
    });
}

pub fn seekError(err: os.SeekError) noreturn {
    std.debug.panic("{s}", .{
        switch (err) {
            error.Unseekable => "The operation cannot be performed on an unseekable entity.",
            error.AccessDenied => "Access Denied.",
            else => "Unexpected Error."
        }
    });
}

pub fn readError(err: (os.ReadError || error{EndOfStream})) noreturn {
    std.debug.panic("{s}", .{
        switch (err) {
            error.InputOutput => "Input or output error",
            error.SystemResources => "Insufficient system resources",
            error.IsDir => "Is a directory",
            error.OperationAborted => "Operation aborted",
            error.BrokenPipe => "Broken pipe",
            error.ConnectionResetByPeer => "Connection reset by peer",
            error.ConnectionTimedOut => "Connection timed out",
            error.NotOpenForReading => "Not open for reading",
            error.NetNameDeleted => "Network name deleted",
            error.WouldBlock => "Operation would block",
            error.AccessDenied => "Access denied",
            else => "Unexpected Error."
        }
    });
}