/* SPDX-License-Identifier: MIT
Copyright 2021 Kevin Thibedeau
(kevin 'period' thibedeau 'at' gmail 'punto' com)

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

/*
 * ------------------------------------------------------------------------------
 * getopt_long_r
 *
 * This is a reentrant implementation of getopt_long(). It operates similarly to
 * standard getopt_long() with the addition of a state struct to avoid global state.
 * The argc argument is not used in this version. The long option index value is
 * returned in the state struct.
 * ------------------------------------------------------------------------------
 */


#ifndef GETOPT_R_H
#define GETOPT_R_H

struct getopt_state {
    char *next_char; // For internal tracking by getopt_long_r()

    // Parser results
    char *optarg;             // Argument to current option
    int   optind;             // Index for next option to process
    int   optopt;             // Current option letter or error codes ('?' or ':')
    int   long_index;         // Index to long option array

    // Configuration settings
    unsigned char report_errors : 1;      // Set true for error messages
    unsigned char dont_permute_args : 1;  // Set true to don't shift non-options to right
};


// Long option definition
struct option {
    const char *name;     // Long option name
    int has_arg;          // Argument type (no, required, optional)
    int *flag;            // Option value if not NULL
    int val;              // Value to store in flag or short option letter
};

// Values for struct option has_arg field:
#define no_argument         0
#define required_argument   1
#define optional_argument   2

#define getopt_init(s)   memset(s, 0, sizeof(*(s)))

#ifdef __cplusplus
extern "C" {
#endif

int getopt_long_r(int argc, char *argv[], const char *optstring, const struct option *long_options, struct getopt_state *state);

static inline int getopt_r(int argc, char *argv[], const char *optstring, struct getopt_state *state)
{
    return getopt_long_r(argc, argv, optstring, NULL, state);
}

#ifdef __cplusplus
}
#endif


#endif // GETOPT_R_H

