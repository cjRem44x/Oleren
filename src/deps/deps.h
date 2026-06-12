#ifndef OLRN_DEPS_H
#define OLRN_DEPS_H

#include <stddef.h>

/* System-library dependencies of stdlib modules (e.g. malkur → SDL2).
   Resolution order: pkg-config → per-platform fallback flags. */
typedef struct {
    const char *module;      /* stdlib module that pulls it in ("malkur") */
    const char *lib;         /* human name ("SDL2_mixer") */
    const char *pkg;         /* pkg-config package name ("SDL2_mixer") */
    const char *pkg_pacman;  /* Arch/MSYS2 pacman name ("sdl2_mixer") */
    const char *pkg_apt;     /* Debian/Ubuntu apt base name ("sdl2-mixer") */
    const char *header;      /* marker include in generated C++ */
    const char *flags_lin;   /* fallback link flags per platform */
    const char *flags_mac;
    const char *flags_win;
} SysDep;

/* registry, terminated by a NULL .module entry */
extern const SysDep SYS_DEPS[];

/* first dep for a stdlib module name, or NULL */
const SysDep *dep_for_module(const char *module);

/* all deps for a stdlib module; fills out[0..] up to max, returns count */
int deps_for_module(const char *module, const SysDep **out, int max);

/* true if the dependency is installed (pkg-config or header probe) */
int dep_available(const SysDep *d);

/* append compiler/link flags for d to buf (pkg-config output when
   available, platform fallback otherwise). Returns 0 on success. */
int dep_flags(const SysDep *d, char *buf, size_t n);

/* print "how to install" guidance for the current platform */
void dep_print_hint(const SysDep *d);

/* installed version via pkg-config; buf gets "" when unknown */
void dep_version(const SysDep *d, char *buf, size_t n);

/* scan a generated .cpp for dep marker headers; append all needed
   flags to buf. Prints hints and returns 1 if a dep is missing. */
int deps_flags_for_cpp(const char *cpp_path, char *buf, size_t n);

#endif /* OLRN_DEPS_H */
