/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_STRING_METHODS_H
#define PY68K_STRING_METHODS_H

#include "py68k_status.h"
#include "py68k_value.h"

struct Py68Runtime;
struct Py68String;
struct Py68Dict;

/* Attr-bound native callbacks (arguments[0] is self string). */
Py68Status py68_attr_string_upper(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);
Py68Status py68_attr_string_lower(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);
Py68Status py68_attr_string_casefold(struct Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_capitalize(struct Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_swapcase(struct Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_title(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);
Py68Status py68_attr_string_find(struct Py68Runtime *runtime,
                                 Py68U16 argument_count, Py68Value *arguments,
                                 Py68Value *result);
Py68Status py68_attr_string_rfind(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);
Py68Status py68_attr_string_index(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);
Py68Status py68_attr_string_rindex(struct Py68Runtime *runtime,
                                   Py68U16 argument_count, Py68Value *arguments,
                                   Py68Value *result);
Py68Status py68_attr_string_count(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);
Py68Status py68_attr_string_startswith(struct Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_endswith(struct Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_strip(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);
Py68Status py68_attr_string_lstrip(struct Py68Runtime *runtime,
                                   Py68U16 argument_count, Py68Value *arguments,
                                   Py68Value *result);
Py68Status py68_attr_string_rstrip(struct Py68Runtime *runtime,
                                   Py68U16 argument_count, Py68Value *arguments,
                                   Py68Value *result);
Py68Status py68_attr_string_removeprefix(struct Py68Runtime *runtime,
                                         Py68U16 argument_count,
                                         Py68Value *arguments,
                                         Py68Value *result);
Py68Status py68_attr_string_removesuffix(struct Py68Runtime *runtime,
                                         Py68U16 argument_count,
                                         Py68Value *arguments,
                                         Py68Value *result);
Py68Status py68_attr_string_split(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);
Py68Status py68_attr_string_rsplit(struct Py68Runtime *runtime,
                                   Py68U16 argument_count, Py68Value *arguments,
                                   Py68Value *result);
Py68Status py68_attr_string_splitlines(struct Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_partition(struct Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_rpartition(struct Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_join(struct Py68Runtime *runtime,
                                 Py68U16 argument_count, Py68Value *arguments,
                                 Py68Value *result);
Py68Status py68_attr_string_replace(struct Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_center(struct Py68Runtime *runtime,
                                   Py68U16 argument_count, Py68Value *arguments,
                                   Py68Value *result);
Py68Status py68_attr_string_ljust(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);
Py68Status py68_attr_string_rjust(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);
Py68Status py68_attr_string_zfill(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);
Py68Status py68_attr_string_expandtabs(struct Py68Runtime *runtime,
                                       Py68U16 argument_count,
                                       Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_translate(struct Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_isalnum(struct Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_isalpha(struct Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_isascii(struct Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_isdecimal(struct Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_isdigit(struct Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_isidentifier(struct Py68Runtime *runtime,
                                         Py68U16 argument_count,
                                         Py68Value *arguments,
                                         Py68Value *result);
Py68Status py68_attr_string_islower(struct Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_isnumeric(struct Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_isprintable(struct Py68Runtime *runtime,
                                        Py68U16 argument_count,
                                        Py68Value *arguments,
                                        Py68Value *result);
Py68Status py68_attr_string_isspace(struct Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_istitle(struct Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result);
Py68Status py68_attr_string_isupper(struct Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result);

/* Builtin helpers shared with text builtins. */
Py68Status py68_string_repr_escape(struct Py68Runtime *runtime,
                                   struct Py68String *string, int escape_high,
                                   struct Py68String **result);
Py68Status py68_builtin_maketrans(struct Py68Runtime *runtime,
                                  Py68U16 argument_count, Py68Value *arguments,
                                  Py68Value *result);

#endif
