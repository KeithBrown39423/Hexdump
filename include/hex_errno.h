#ifndef _HEX_ERRNO_H
#define _HEX_ERRNO_H

enum hex_errno {
    HEX_OK,             // No error
    HEX_EMISARG,        // Missing argument
    HEX_EIO,            // I/O error
    HEX_EFBIG,          // File too big
    HEX_EFEMPTY,        // File is empty
    HEX_ENOENT,         // File does not exist
    HEX_ELENEX,         // Length exceeds maximum limit
    HEX_EINVAL,         // Invalid argument

    HEX_RETURN = 127    // Return code | Used in exit_hexdump() at the end of main()
};

#endif // _HEX_ERRNO_H