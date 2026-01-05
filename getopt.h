#ifndef __GETOPT_H__
#define __GETOPT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

    /* Global variables for getopt */
    extern int opterr; /* if error message should be printed */
    extern int optind; /* index into parent argv vector */
    extern int optopt; /* character checked for validity */
    extern int optreset; /* reset getopt */
    extern char* optarg; /* argument associated with option */

    /* Function prototypes */
    int getopt(int nargc, char* const nargv[], const char* options);
    int getopt_long(int nargc, char* const nargv[], const char* options,
        const struct option* long_options, int* idx);
    int getopt_long_only(int nargc, char* const nargv[], const char* options,
        const struct option* long_options, int* idx);

    /* Long option structure */
    struct option {
        const char* name;
        int has_arg;
        int* flag;
        int val;
    };

    enum { no_argument = 0, required_argument, optional_argument };

#ifdef __cplusplus
}
#endif

#endif /* __GETOPT_H__ */
