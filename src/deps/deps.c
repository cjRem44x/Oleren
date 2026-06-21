#define _POSIX_C_SOURCE 200809L
#include "deps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define popen  _popen
#define pclose _pclose
#endif

const SysDep SYS_DEPS[] = {
    /*  module     lib          pkg          pkg_pacman   pkg_apt
        header              flags_lin          flags_mac          flags_win */
    { "gdev", "SDL2",       "sdl2",       "sdl2",       "sdl2",
      "SDL2/SDL.h",       "-lSDL2",       "-lSDL2",       "-lmingw32 -lSDL2main -lSDL2" },
    { "gdev", "SDL2_mixer", "SDL2_mixer", "sdl2_mixer", "sdl2-mixer",
      "SDL2/SDL_mixer.h", "-lSDL2_mixer", "-lSDL2_mixer", "-lSDL2_mixer" },
    { "gdev", "SDL2_image", "SDL2_image", "sdl2_image", "sdl2-image",
      "SDL2/SDL_image.h", "-lSDL2_image", "-lSDL2_image", "-lSDL2_image" },
    { "gdev", "SDL2_ttf",   "SDL2_ttf",   "sdl2_ttf",   "sdl2-ttf",
      "SDL2/SDL_ttf.h",  "-lSDL2_ttf",  "-lSDL2_ttf",  "-lSDL2_ttf" },
    { "crypt", "libsodium", "libsodium", "libsodium", "libsodium",
      "sodium.h", "-lsodium", "-lsodium", "-lsodium" },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }
};

const SysDep *dep_for_module(const char *module)
{
    for (int i = 0; SYS_DEPS[i].module; i++)
        if (strcmp(SYS_DEPS[i].module, module) == 0) return &SYS_DEPS[i];
    return NULL;
}

int deps_for_module(const char *module, const SysDep **out, int max)
{
    int n = 0;
    for (int i = 0; SYS_DEPS[i].module && n < max; i++)
        if (strcmp(SYS_DEPS[i].module, module) == 0) out[n++] = &SYS_DEPS[i];
    return n;
}

/* run a command, capture first line of stdout into buf; 0 on success */
static int run_capture(const char *cmd, char *buf, size_t n)
{
    FILE *p = popen(cmd, "r");
    if (!p) return 1;
    if (buf) {
        buf[0] = '\0';
        if (fgets(buf, (int)n, p)) {
            char *nl = strchr(buf, '\n');
            if (nl) *nl = '\0';
        }
    }
    return pclose(p) == 0 ? 0 : 1;
}

static int pkg_config_has(const SysDep *d)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "pkg-config --exists %s 2>/dev/null", d->pkg);
    return system(cmd) == 0;
}

int dep_available(const SysDep *d)
{
    if (pkg_config_has(d)) return 1;
    /* no pkg-config entry — probe the header through the compiler */
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "echo '#include <%s>' | g++ -E -x c++ - >/dev/null 2>&1",
             d->header);
    return system(cmd) == 0;
}

static const char *fallback_flags(const SysDep *d)
{
#if defined(_WIN32)
    return d->flags_win;
#elif defined(__APPLE__)
    return d->flags_mac;
#else
    return d->flags_lin;
#endif
}

int dep_flags(const SysDep *d, char *buf, size_t n)
{
    char cmd[256], out[512];
    snprintf(cmd, sizeof(cmd),
             "pkg-config --cflags --libs %s 2>/dev/null", d->pkg);
    if (run_capture(cmd, out, sizeof(out)) == 0 && out[0]) {
        snprintf(buf, n, " %s", out);
        return 0;
    }
    snprintf(buf, n, " %s", fallback_flags(d));
    return 0;
}

void dep_version(const SysDep *d, char *buf, size_t n)
{
    char cmd[256];
    if (n) buf[0] = '\0';
    snprintf(cmd, sizeof(cmd),
             "pkg-config --modversion %s 2>/dev/null", d->pkg);
    run_capture(cmd, buf, n);
}

/* Linux distro id from /etc/os-release ("arch", "ubuntu", ...) */
static const char *linux_distro(void)
{
    static char id[64] = "";
    FILE *f = fopen("/etc/os-release", "r");
    if (!f) return id;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "ID=", 3) == 0) {
            char *v = line + 3;
            if (*v == '"') v++;
            size_t len = strcspn(v, "\"\n");
            if (len >= sizeof(id)) len = sizeof(id) - 1;
            memcpy(id, v, len); id[len] = '\0';
            break;
        }
    }
    fclose(f);
    return id;
}

void dep_print_hint(const SysDep *d)
{
    fprintf(stderr, "  '%s' (needed by @std.%s) is not installed.\n",
            d->lib, d->module);
#if defined(_WIN32)
    fprintf(stderr,
        "  install it with one of:\n"
        "    MSYS2:  pacman -S mingw-w64-x86_64-%s\n"
        "    vcpkg:  vcpkg install %s\n", d->pkg_pacman, d->pkg_apt);
#elif defined(__APPLE__)
    fprintf(stderr, "  install it with:  brew install %s\n", d->pkg_pacman);
#else
    {
        const char *id = linux_distro();
        if (strstr(id, "arch") || strstr(id, "cachyos") ||
            strstr(id, "manjaro") || strstr(id, "endeavour"))
            fprintf(stderr, "  install it with:  sudo pacman -S %s\n",
                    d->pkg_pacman);
        else if (strstr(id, "debian") || strstr(id, "ubuntu") ||
                 strstr(id, "mint") || strstr(id, "pop"))
            fprintf(stderr, "  install it with:  sudo apt install lib%s-dev\n",
                    d->pkg_apt);
        else if (strstr(id, "fedora") || strstr(id, "rhel"))
            fprintf(stderr, "  install it with:  sudo dnf install %s-devel\n",
                    d->lib);
        else if (strstr(id, "suse"))
            fprintf(stderr, "  install it with:  sudo zypper install lib%s-devel\n",
                    d->pkg_apt);
        else
            fprintf(stderr,
                "  install the %s development package, e.g.:\n"
                "    pacman -S %s   |   apt install lib%s-dev   |   "
                "dnf install %s-devel\n",
                d->lib, d->pkg_pacman, d->pkg_apt, d->lib);
    }
#endif
}

int deps_flags_for_cpp(const char *cpp_path, char *buf, size_t n)
{
    if (n) buf[0] = '\0';

    int needed[16] = {0};
    FILE *f = fopen(cpp_path, "r");
    if (!f) return 0;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        for (int i = 0; SYS_DEPS[i].module && i < 16; i++)
            if (!needed[i] && strstr(line, SYS_DEPS[i].header))
                needed[i] = 1;
    }
    fclose(f);

    int missing = 0;
    for (int i = 0; SYS_DEPS[i].module && i < 16; i++) {
        if (!needed[i]) continue;
        const SysDep *d = &SYS_DEPS[i];
        if (!dep_available(d)) {
            fprintf(stderr, "error: missing system dependency\n");
            dep_print_hint(d);
            missing = 1;
            continue;
        }
        char flags[600];
        if (dep_flags(d, flags, sizeof(flags)) == 0) {
            size_t used = strlen(buf);
            snprintf(buf + used, n - used, "%s", flags);
        }
    }
    return missing;
}
