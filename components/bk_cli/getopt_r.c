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
------------------------------------------------------------------------------
getopt_long_r

This is a reentrant implementation of getopt_long(). It operates similarly to
standard getopt_long() with the addition of a struct getopt_state struct to avoid global
state. The `argc` parameter is not used in this implementation. The argument list
*must* terminate with a NULL pointer. The long option index value is returned in
the state struct.

The state struct should be initialized to 0 on creation or by using the
:c:macro:`getopt_init` macro.

The struct fields `optarg`, `optind`, and `optopt` correspond to the same globals
used by standard getopt*().

Setting the `permute_args` member of the struct getopt_state struct to true enables shifting
non-option arguments to the end of the argument list. When option parsing
completes, the `optind` member points to the first non-option, if any.

Verbose error reporting is enabled by setting `report_errors` true in the struct.


When the short option string begins with a colon ':', a missing option argument
is reported by returning ':'. Otherwise, missing options are treated as a normal
error with a return of '?'.

The GNU extension for optional short arguments is supported. These use a double
colon in the optstring:

  getopt_long_r(argv, "a",   NULL, &state); // No argument       "-a"
  getopt_long_r(argv, "a:",  NULL, &state); // Required argument "-a [foo]" or "-a[foo]"
  getopt_long_r(argv, "a::", NULL, &state); // Optional argument "-a [foo]?"

------------------------------------------------------------------------------
*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "getopt_r.h"
#include "cli.h"


#define ERR_MISSING_OPT   ':'
#define ERR_UNKNOWN_OPT   '?'


// Shift an argv param to the right
static inline void permute_right(char *argv[], int move_ix, int shift_right)
{
    char *permuted = argv[move_ix]; // Save permuted arg

    // Shift following args back
    for (int i = move_ix + 1; i <= move_ix + shift_right; i++)
        argv[i - 1] = argv[i];

    argv[move_ix + shift_right] = permuted;
}


// Permute non-options toward end of argv
static void permute_args(char *argv[], int non_opts, int non_opt_start, struct getopt_state *state)
{
    int shift_right = non_opts;

    if (state->optarg && state->optarg == argv[state->optind - 1])
        shift_right++; // optarg is in separate parameter

    for (int i = 0; i < non_opts; i++)
        permute_right((char **)argv, non_opt_start, shift_right);

    // Adjust indices
    state->optind -= non_opts;
}



/*
 * Parse command line options
 *
 * Args:
 * argv:         Array of command arguments. Must be terminated by a NULL entry.
 * optstring:    Short option definition string
 * long_options: Optional list of long option definitions terminated by NULL object
 * state:        State from previous invocation of getopt_long_r()
 *
 * Returns:
 * New option character on success. 0 for long option with non-NULL flag.
 * -1 on end of parsing. '?' or ':' on parse failure
 */
int getopt_long_r(int argc, char *argv[], const char *optstring, const struct option *long_options,
                  struct getopt_state *state)
{
#define CUR_OPT argv[state->optind]

    int rval = -1;

    if (!optstring || !state)
        return -1;

    // Initialize state if it looks dirty
    if (state->optind < 1) {
        state->optind = 1;
        state->next_char = NULL;
    }

    // Wipe out previous results
    state->optarg = NULL;
    state->optopt = '\0';
    state->long_index = -1;


    // Scan for non-options
    int non_opts = 0;
    int non_opt_start = state->optind;
    if (!state->dont_permute_args) {
        while (CUR_OPT && (CUR_OPT[0] != '-' || CUR_OPT[1] == '\0')) {
            state->optind++;
            non_opts++;
        }
    }

    // Check for end of args, non-option, or bare "-"
    if (!CUR_OPT || CUR_OPT[0] != '-' || !strcmp(CUR_OPT, "-")) {
        // Done processing options
        state->next_char = NULL;
        if (non_opts > 0) // Correct optind so it points to first non-option
            state->optind -= non_opts;
        return -1;
    }

    // Check for explicit option termination
    if (!strcmp(CUR_OPT, "--")) {
        // Done processing options
        state->optind++; // Skip over "--" for extra arguments

        if (non_opts > 0)
            permute_args((char **)argv, non_opts, non_opt_start, state);

        state->next_char = NULL;
        return -1;
    }


    if (CUR_OPT[1] == '-') { // Parse long option
        state->next_char = NULL;

        if (!long_options)
            return -1;

        const char *long_opt = &CUR_OPT[2];
        state->optind++;

        char *equal_pos = strchr(long_opt, '=');
        size_t long_opt_len = (equal_pos != NULL) ? (size_t)(equal_pos - long_opt) : strlen(long_opt);
        int long_match = -1;

        // Scan for matching option
        for (int i = 0; long_options[i].name; i++) {
            if (strncmp(long_opt, long_options[i].name, long_opt_len) != 0) // Prefix match
                continue;

            if (strlen(long_options[i].name) == long_opt_len) { // Check for exact match
                long_match = i;
                break;
            }
        }

        if (long_match >= 0) {
            switch (long_options[long_match].has_arg) {
            case no_argument:
                if (equal_pos != NULL) { // Invalid option with argument
                    if (state->report_errors)
                        CLI_LOGE("getopt_long_r() Option '--%.*s' takes no argument\n",
                                 (int)long_opt_len, long_opt);

                    // Indicate equivalent short option that caused error
                    state->optopt = long_options[long_match].flag ? '\0' : long_options[long_match].val;
                    rval = ERR_UNKNOWN_OPT;
                }
                break;

            case required_argument:
                if (equal_pos != NULL) {    // Argument in this option
                    state->optarg = equal_pos + 1;
                } else {                    // Argument in next option
                    state->optarg = CUR_OPT;
                    state->optind++;
                }

                if (!state->optarg) { // Validate arg was found
                    if (state->report_errors)
                        CLI_LOGE("ERROR: getopt_long_r() Missing required argument to option '--%s'\n",
                                 long_opt);

                    // Indicate equivalent short option that caused error
                    state->optopt = long_options[long_match].flag ? '\0' : long_options[long_match].val;
                    state->optind--; // Revert to this option's arg position
                    rval = ERR_UNKNOWN_OPT;
                }
                break;

            case optional_argument:
                // Optional arguments in long opts are only usable with '=' form
                if (equal_pos != NULL) // Argument in this option
                    state->optarg = equal_pos + 1;
                break;
            }
        } else { // Unknown long option
            if (state->report_errors)
                CLI_LOGE("ERROR: getopt_long_r() Unknown long option '--%s'\n",
                         long_opt);
            state->optopt = '\0';
            rval = ERR_UNKNOWN_OPT;
        }

        if (rval == -1) { // No errors
            state->long_index = long_match;

            if (long_options[long_match].flag) {
                *long_options[long_match].flag = long_options[long_match].val;
                rval = '\0';
            } else {
                rval = long_options[long_match].val;
            }
        }
    } else {                                                // Parse short options
        if (!state->next_char || *state->next_char == '\0') // We consumed previous arg; Start on next options
            state->next_char = &CUR_OPT[1];                 // Skip over hyphen to set new start of option text

        state->optopt = *state->next_char;                  // Report current option to caller if return value is discarded
        rval = state->optopt;

        // Search optstring for the current option
        const char *opt_def = strchr(optstring, state->optopt);

        if (opt_def) {
            if (opt_def[1] == ':') { // This option takes an argument
                // Assume the argument follows immediately without space
                state->optarg = &state->next_char[1];

                if (state->optarg[0] == '\0') { // There is no arg included in this option
                    if (opt_def[2] != ':') {    // Mandatory argument
                        state->optind++;        // Look for argument in next position
                        if (CUR_OPT) {
                            state->optarg = CUR_OPT;
                        } else { // Exhausted all options without finding an argument
                            state->optarg = NULL;

                            if (state->report_errors)
                                CLI_LOGE("ERROR: getopt_r() Missing required argument to option '-%c'\n",
                                         state->optopt);

                            // Leading ':' in optstring affects error condition but not error reporting
                            rval = optstring[0] == ':' ? ERR_MISSING_OPT : ERR_UNKNOWN_OPT;
                        }
                    } else { // Optional argument (GNU extension using "::")
                        // Check if the next arg doesn't begin with '-'
                        state->optind++;
                        if (CUR_OPT && CUR_OPT[0] != '-') { // We use this as the argument
                            state->optarg = CUR_OPT;
                        } else {                            // Revert and work without an argument
                            state->optind--;
                            state->optarg = NULL;
                        }
                    }
                }

                state->next_char = NULL;    // Force switch to next arg
            }
        } else {                            // Option not found
            if (state->report_errors)
                CLI_LOGE("ERROR: getopt_r() Unknown option '-%c'\n", state->optopt);

            rval = ERR_UNKNOWN_OPT;
        }

        if (!state->next_char) {    // All options in this arg are consumed
            state->optind++;        // Work on next arg
        } else {                    // Prep for next option within this arg
            state->next_char++;
            if (*state->next_char == '\0')
                state->optind++;
        }
    }

    // Permute non-options
    if (non_opts > 0)
        permute_args((char **)argv, non_opts, non_opt_start, state);

    return rval;
}

