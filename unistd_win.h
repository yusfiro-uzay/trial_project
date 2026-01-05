#ifndef UNISTD_WIN_H
#define UNISTD_WIN_H

#ifdef _WIN32

#include <io.h>       // For _access, _close, _read, _write, _chsize, etc.
#include <process.h>  // For _getpid, _exec functions, etc.
#include <direct.h>   // For _chdir, _getcwd
#include <windows.h>  // For Sleep and other WinAPI functions

// If code uses sleep(seconds), replace with Sleep(milliseconds)
static inline unsigned int sleep(unsigned int seconds) {
    Sleep(seconds * 1000);
    return 0; // Sleep does not report remaining time on Windows.
}

// If code uses usleep(microseconds), replace with Sleep(milliseconds)
static inline int usleep(unsigned int usec) {
    Sleep(usec / 1000);
    return 0;
}

// Access mode constants differ slightly
#ifndef R_OK
#define R_OK 4  /* Test for read permission */
#endif
#ifndef W_OK
#define W_OK 2  /* Test for write permission */
#endif
#ifndef X_OK
#define X_OK 1  /* execute permission - not meaningful on Windows */
#endif
#ifndef F_OK
#define F_OK 0  /* Test for existence */
#endif

// Map POSIX functions to their Windows CRT equivalents
#define access    _access
#define chdir     _chdir
#define getcwd    _getcwd
#define unlink    _unlink
#define close     _close
#define read      _read
#define write     _write
#define lseek     _lseek

// pid_t is generally an int on Windows
typedef int pid_t;
#define getpid _getpid

// getppid is not available on Windows; define a stub if needed
static inline pid_t getppid(void) {
    // Windows doesn't have a direct equivalent of getppid.
    // Returning parent's PID isn't straightforward.
    // This is a placeholder that always returns 0.
    return 0;
}

#else  // Non-Windows platforms just include the original unistd.h

#include <unistd.h>

#endif // _WIN32

#endif // UNISTD_WIN_H
