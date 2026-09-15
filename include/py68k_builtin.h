/* 2026 by Piotr Rozentreter (Rozsoft) */

#ifndef PY68K_BUILTIN_H
#define PY68K_BUILTIN_H

#include "py68k_runtime.h"

Py68Status py68_builtin_set_copy(Py68Runtime *runtime, const Py68U8 *name,
                                 Py68U16 name_length, Py68Value value);
Py68Status py68_builtin_get_copy(Py68Runtime *runtime, const Py68U8 *name,
                                 Py68U16 name_length, Py68Value *result);
void py68_builtin_clear(Py68Runtime *runtime);
Py68Status py68_builtins_install(Py68Runtime *runtime);
Py68Status py68_builtin_print(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_input(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_len(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_range(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_list_pop(Py68Runtime *runtime, Py68U16 argument_count,
                                Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_list_append(Py68Runtime *runtime,
                                    Py68U16 argument_count,
                                    Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_int(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_str(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_bool(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_abs(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_min(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_max(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_exit(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_fopen(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_fclose(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_fread(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_freadline(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_fwrite(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_exists(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_remove(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_rename(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_getenv(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_setenv(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_unsetenv(Py68Runtime *runtime, Py68U16 argument_count,
                                 Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_assign_get(Py68Runtime *runtime,
                                   Py68U16 argument_count,
                                   Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_assign_add(Py68Runtime *runtime,
                                   Py68U16 argument_count,
                                   Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_assign_remove(Py68Runtime *runtime,
                                      Py68U16 argument_count,
                                      Py68Value *arguments, Py68Value *result);
#if defined(PY68K_AMIGA)
Py68Status py68_builtin_load_library(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments, Py68Value *result);
#endif
Py68Status py68_builtin_list(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_tuple(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_dict(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_set(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_float(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_exception(Py68Runtime *runtime,
                                  Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_ord(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_chr(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_repr(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_ascii(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_all(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_any(Py68Runtime *runtime, Py68U16 argument_count,
                            Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_format(Py68Runtime *runtime, Py68U16 argument_count,
                               Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_time(Py68Runtime *runtime, Py68U16 argument_count,
                             Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_sleep(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_ctime(Py68Runtime *runtime, Py68U16 argument_count,
                              Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_localtime(Py68Runtime *runtime, Py68U16 argument_count,
                                  Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_strftime(Py68Runtime *runtime, Py68U16 argument_count,
                                 Py68Value *arguments, Py68Value *result);
Py68Status py68_builtin_perf_counter(Py68Runtime *runtime,
                                     Py68U16 argument_count,
                                     Py68Value *arguments,
                                     Py68Value *result);
Py68Status py68_builtin_time_tick(Py68Runtime *runtime,
                                  Py68U16 argument_count,
                                  Py68Value *arguments,
                                  Py68Value *result);

#endif
