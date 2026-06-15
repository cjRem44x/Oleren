#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <tuple>
#include <type_traits>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
template<typename F>
struct _OlrnDeferGuard {
    F fn;
    _OlrnDeferGuard(F f) : fn(std::move(f)) {}
    ~_OlrnDeferGuard() { fn(); }
    _OlrnDeferGuard(const _OlrnDeferGuard&) = delete;
    _OlrnDeferGuard& operator=(const _OlrnDeferGuard&) = delete;
};
/* ── olrn builtins ─────────────────────────────────────────────── */
static inline std::string _olrn_cin(const std::string& prompt) {
    std::cout << prompt;
    std::string _line;
    std::getline(std::cin, _line);
    return _line; }
template<typename T>
static inline T _olrn_rng(T lo, T hi) {
    static std::mt19937 _eng(std::random_device{}());
    if constexpr (std::is_integral_v<T>)
        return std::uniform_int_distribution<T>(lo, hi)(_eng);
    else
        return std::uniform_real_distribution<T>(lo, hi)(_eng); }
/* ── error handling ─────────────────────────────────────────────── */
struct _OlrnError { int32_t code; const char* msg; };
template<typename T>
struct _OlrnResult {
    bool _ok; T _val; _OlrnError _err;
    _OlrnResult(T v) : _ok(true), _val(std::move(v)), _err{0, nullptr} {}
    _OlrnResult(_OlrnError e) : _ok(false), _val{}, _err(e) {}
    bool is_ok() const { return _ok; }
    T value() const { return _val; }
    const _OlrnError& error() const { return _err; }
};
template<> struct _OlrnResult<void> {
    bool _ok; _OlrnError _err;
    _OlrnResult() : _ok(true), _err{0, nullptr} {}
    _OlrnResult(_OlrnError e) : _ok(false), _err(e) {}
    bool is_ok() const { return _ok; }
    const _OlrnError& error() const { return _err; }
};
/* casts: numeric/enum → plain static_cast; string → fallible parse,
   so @i32("42") yields _OlrnResult<int32_t> (use try/catch) */
template<typename T, typename F,
         typename std::enable_if<std::is_arithmetic<F>::value ||
                                 std::is_enum<F>::value, int>::type = 0>
static inline T _olrn_cast(F v) { return static_cast<T>(v); }
template<typename T>
static inline _OlrnResult<T> _olrn_cast(const std::string& s) {
    try {
        size_t pos = 0;
        if constexpr (std::is_floating_point_v<T>) {
            double d = std::stod(s, &pos);
            if (pos != s.size()) return _OlrnError{-1, "InvalidNumber"};
            return _OlrnResult<T>(static_cast<T>(d));
        } else if constexpr (std::is_unsigned_v<T>) {
            if (!s.empty() && s[0] == '-') return _OlrnError{-1, "InvalidNumber"};
            unsigned long long v = std::stoull(s, &pos);
            if (pos != s.size()) return _OlrnError{-1, "InvalidNumber"};
            return _OlrnResult<T>(static_cast<T>(v));
        } else {
            long long v = std::stoll(s, &pos);
            if (pos != s.size()) return _OlrnError{-1, "InvalidNumber"};
            return _OlrnResult<T>(static_cast<T>(v));
        }
    } catch (...) { return _OlrnError{-1, "InvalidNumber"}; }
}
template<typename T>
static inline _OlrnResult<T> _olrn_cast(const char* s) {
    return _olrn_cast<T>(std::string(s)); }
/* ── end olrn builtins ──────────────────────────────────────────── */
/* ── olrn stdlib ───────────────────────────────────────────────── */
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <filesystem>

/* std.time */
static inline int64_t _olrn_time_now() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count(); }
static inline int64_t _olrn_time_mono() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count(); }
static inline void _olrn_time_sleep(int64_t ns) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(ns)); }

/* std.math */
static inline double _olrn_sqrt(double x)          { return std::sqrt(x); }
static inline double _olrn_fabs(double x)          { return std::fabs(x); }
static inline double _olrn_sin(double x)           { return std::sin(x); }
static inline double _olrn_cos(double x)           { return std::cos(x); }
static inline double _olrn_tan(double x)           { return std::tan(x); }
static inline double _olrn_asin(double x)          { return std::asin(x); }
static inline double _olrn_acos(double x)          { return std::acos(x); }
static inline double _olrn_atan(double x)          { return std::atan(x); }
static inline double _olrn_atan2(double y, double x){ return std::atan2(y,x); }
static inline double _olrn_floor(double x)         { return std::floor(x); }
static inline double _olrn_ceil(double x)          { return std::ceil(x); }
static inline double _olrn_round(double x)         { return std::round(x); }
static inline double _olrn_pow(double b, double e) { return std::pow(b,e); }
static inline double _olrn_log(double x)           { return std::log(x); }
static inline double _olrn_log2(double x)          { return std::log2(x); }
static inline double _olrn_exp(double x)           { return std::exp(x); }

/* std.str */
static inline int64_t _olrn_str_len(const std::string& s) { return (int64_t)s.size(); }
static inline int64_t _olrn_str_parse_int(const std::string& s) { return std::stoll(s); }
static inline double  _olrn_str_parse_f64(const std::string& s) { return std::stod(s); }
static inline std::string _olrn_str_upper(std::string s) {
    std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return (char)std::toupper(c);});
    return s; }
static inline std::string _olrn_str_lower(std::string s) {
    std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return (char)std::tolower(c);});
    return s; }
static inline std::string _olrn_str_trim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(),s.end(),[](unsigned char c){return !std::isspace(c);}));
    s.erase(std::find_if(s.rbegin(),s.rend(),[](unsigned char c){return !std::isspace(c);}).base(), s.end());
    return s; }
static inline bool _olrn_str_starts(const std::string& s, const std::string& p) {
    return s.rfind(p,0)==0; }
static inline bool _olrn_str_ends(const std::string& s, const std::string& p) {
    return s.size()>=p.size() && s.compare(s.size()-p.size(),p.size(),p)==0; }
static inline bool _olrn_str_contains(const std::string& s, const std::string& p) {
    return s.find(p)!=std::string::npos; }

/* std.io */
static inline int64_t _olrn_io_open(const std::string& path, int32_t mode) {
    const char* modes[] = {"r", "w", "r+", "a"};
    if (mode < 0 || mode > 3) return 0;
    FILE* f = std::fopen(path.c_str(), modes[mode]);
    return (int64_t)(uintptr_t)f; }
static inline void _olrn_io_close(int64_t h) {
    if (h) std::fclose((FILE*)(uintptr_t)h); }
static inline std::vector<uint8_t> _olrn_io_read(int64_t h, int64_t n) {
    std::vector<uint8_t> buf((size_t)n);
    size_t r = std::fread(buf.data(), 1, (size_t)n, (FILE*)(uintptr_t)h);
    buf.resize(r); return buf; }
static inline void _olrn_io_write(int64_t h, const std::vector<uint8_t>& data) {
    std::fwrite(data.data(), 1, data.size(), (FILE*)(uintptr_t)h); }
static inline void _olrn_io_seek(int64_t h, int64_t pos, int32_t from) {
    int whence[] = {SEEK_SET, SEEK_CUR, SEEK_END};
    std::fseek((FILE*)(uintptr_t)h, (long)pos,
               whence[(from >= 0 && from <= 2) ? from : 0]); }
static inline int64_t _olrn_io_tell(int64_t h) {
    return (int64_t)std::ftell((FILE*)(uintptr_t)h); }
static inline std::string _olrn_io_readline(int64_t h) {
    FILE* f = (FILE*)(uintptr_t)h;
    std::string line; int c;
    while ((c = std::fgetc(f)) != EOF && c != '\n') line += (char)c;
    return line; }
static inline bool _olrn_io_eof(int64_t h) {
    return std::feof((FILE*)(uintptr_t)h) != 0; }

/* std.fs */
static inline bool _olrn_fs_exists(const std::string& p) {
    std::error_code ec; return std::filesystem::exists(p, ec); }
static inline bool _olrn_fs_is_dir(const std::string& p) {
    std::error_code ec; return std::filesystem::is_directory(p, ec); }
static inline bool _olrn_fs_is_file(const std::string& p) {
    std::error_code ec; return std::filesystem::is_regular_file(p, ec); }
static inline bool _olrn_fs_mkdir(const std::string& p) {
    std::error_code ec; return std::filesystem::create_directories(p, ec); }
static inline bool _olrn_fs_rm(const std::string& p) {
    std::error_code ec; return std::filesystem::remove(p, ec); }
static inline int64_t _olrn_fs_rm_all(const std::string& p) {
    std::error_code ec;
    return (int64_t)std::filesystem::remove_all(p, ec); }
static inline std::vector<std::string> _olrn_fs_ls(const std::string& p) {
    std::vector<std::string> out; std::error_code ec;
    for (auto& e : std::filesystem::directory_iterator(p, ec))
        out.push_back(e.path().filename().string());
    std::sort(out.begin(), out.end()); return out; }
static inline int64_t _olrn_fs_size(const std::string& p) {
    std::error_code ec;
    auto n = std::filesystem::file_size(p, ec);
    return ec ? -1 : (int64_t)n; }
static inline bool _olrn_fs_rename(const std::string& a, const std::string& b) {
    std::error_code ec; std::filesystem::rename(a, b, ec); return !ec; }
static inline bool _olrn_fs_copy(const std::string& a, const std::string& b) {
    std::error_code ec;
    std::filesystem::copy(a, b,
        std::filesystem::copy_options::overwrite_existing |
        std::filesystem::copy_options::recursive, ec);
    return !ec; }
static inline std::string _olrn_fs_cwd() {
    std::error_code ec;
    return std::filesystem::current_path(ec).string(); }

/* std.thread */
template<typename Fn>
static inline int64_t _olrn_thread_spawn(Fn fn) {
    auto* t = new std::thread(fn);
    return (int64_t)(uintptr_t)t; }
template<typename Fn>
static inline int64_t _olrn_thread_spawn_arg(Fn fn, int64_t arg) {
    auto* t = new std::thread(fn, arg);
    return (int64_t)(uintptr_t)t; }
static inline void _olrn_thread_join(int64_t h) {
    auto* t = (std::thread*)(uintptr_t)h;
    if (t && t->joinable()) t->join();
    delete t; }
static inline void _olrn_thread_detach(int64_t h) {
    auto* t = (std::thread*)(uintptr_t)h;
    if (t && t->joinable()) t->detach();
    delete t; }
static inline void _olrn_thread_yield() { std::this_thread::yield(); }
static inline int64_t _olrn_thread_id() {
    return (int64_t)std::hash<std::thread::id>{}(std::this_thread::get_id()); }
static inline int32_t _olrn_thread_cores() {
    return (int32_t)std::thread::hardware_concurrency(); }
/* std.thread mutex */
static inline int64_t _olrn_mutex_new() {
    return (int64_t)(uintptr_t)(new std::mutex()); }
static inline void _olrn_mutex_lock(int64_t h) {
    ((std::mutex*)(uintptr_t)h)->lock(); }
static inline void _olrn_mutex_unlock(int64_t h) {
    ((std::mutex*)(uintptr_t)h)->unlock(); }
static inline void _olrn_mutex_free(int64_t h) {
    delete (std::mutex*)(uintptr_t)h; }
/* std.thread atomic i32 */
static inline int64_t _olrn_atomic_new(int32_t v) {
    return (int64_t)(uintptr_t)(new std::atomic<int32_t>(v)); }
static inline int32_t _olrn_atomic_load(int64_t h) {
    return ((std::atomic<int32_t>*)(uintptr_t)h)->load(); }
static inline void _olrn_atomic_store(int64_t h, int32_t v) {
    ((std::atomic<int32_t>*)(uintptr_t)h)->store(v); }
static inline int32_t _olrn_atomic_add(int64_t h, int32_t v) {
    return ((std::atomic<int32_t>*)(uintptr_t)h)->fetch_add(v); }
static inline bool _olrn_atomic_cas(int64_t h, int32_t expected, int32_t desired) {
    return ((std::atomic<int32_t>*)(uintptr_t)h)->compare_exchange_strong(expected, desired); }
static inline void _olrn_atomic_free(int64_t h) {
    delete (std::atomic<int32_t>*)(uintptr_t)h; }
/* ── end olrn stdlib ─────────────────────────────────────────────── */
/* ── malkur gamedev library (SDL2 backend) ─────────────────────── */
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <cstring>

struct Color    { uint8_t r, g, b, a; };
struct Vec2     { float x, y; };
struct Rect     { float x, y, w, h; };
struct Camera2D { Vec2 target; Vec2 offset; float rotation; float zoom; };

/* ── embedded 8x8 bitmap font (ASCII 32-126) ────────────────────── */
static const uint8_t _mk_font8[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00}, /* ! */
    {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00}, /* " */
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, /* # */
    {0x18,0x7E,0xC0,0x7C,0x06,0xFC,0x18,0x00}, /* $ */
    {0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00}, /* % */
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, /* & */
    {0x06,0x0C,0x18,0x00,0x00,0x00,0x00,0x00}, /* ' */
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, /* ( */
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, /* ) */
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, /* * */
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, /* + */
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, /* , */
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, /* - */
    {0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00}, /* . */
    {0x03,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00}, /* / */
    {0x7C,0xC6,0xCE,0xD6,0xE6,0xC6,0x7C,0x00}, /* 0 */
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, /* 1 */
    {0x7C,0xC6,0x06,0x1C,0x30,0x66,0xFE,0x00}, /* 2 */
    {0xFC,0x06,0x06,0x3C,0x06,0x06,0xFC,0x00}, /* 3 */
    {0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x0C,0x00}, /* 4 */
    {0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00}, /* 5 */
    {0x3C,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00}, /* 6 */
    {0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00}, /* 7 */
    {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00}, /* 8 */
    {0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00}, /* 9 */
    {0x00,0x30,0x30,0x00,0x00,0x30,0x30,0x00}, /* : */
    {0x00,0x30,0x30,0x00,0x00,0x30,0x30,0x60}, /* ; */
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, /* < */
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, /* = */
    {0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00}, /* > */
    {0x7C,0xC6,0x06,0x0C,0x18,0x00,0x18,0x00}, /* ? */
    {0x7C,0xC6,0xDE,0xDE,0xDC,0xC0,0x7C,0x00}, /* @ */
    {0x38,0x6C,0xC6,0xC6,0xFE,0xC6,0xC6,0x00}, /* A */
    {0xFC,0xC6,0xC6,0xFC,0xC6,0xC6,0xFC,0x00}, /* B */
    {0x7C,0xC6,0xC0,0xC0,0xC0,0xC6,0x7C,0x00}, /* C */
    {0xF8,0xCC,0xC6,0xC6,0xC6,0xCC,0xF8,0x00}, /* D */
    {0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xFE,0x00}, /* E */
    {0xFE,0xC0,0xC0,0xF8,0xC0,0xC0,0xC0,0x00}, /* F */
    {0x7C,0xC6,0xC0,0xC0,0xCE,0xC6,0x7C,0x00}, /* G */
    {0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, /* H */
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00}, /* I */
    {0x1E,0x06,0x06,0x06,0xC6,0xC6,0x7C,0x00}, /* J */
    {0xC6,0xCC,0xD8,0xF0,0xD8,0xCC,0xC6,0x00}, /* K */
    {0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x00}, /* L */
    {0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00}, /* M */
    {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00}, /* N */
    {0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, /* O */
    {0xFC,0xC6,0xC6,0xFC,0xC0,0xC0,0xC0,0x00}, /* P */
    {0x7C,0xC6,0xC6,0xC6,0xD6,0xCC,0x76,0x00}, /* Q */
    {0xFC,0xC6,0xC6,0xFC,0xCC,0xC6,0xC6,0x00}, /* R */
    {0x7C,0xC6,0xC0,0x7C,0x06,0xC6,0x7C,0x00}, /* S */
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, /* T */
    {0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, /* U */
    {0xC6,0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x00}, /* V */
    {0xC6,0xC6,0xD6,0xD6,0xFE,0xEE,0xC6,0x00}, /* W */
    {0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00}, /* X */
    {0xC6,0xC6,0x6C,0x38,0x18,0x18,0x18,0x00}, /* Y */
    {0xFE,0x06,0x0C,0x18,0x30,0x60,0xFE,0x00}, /* Z */
    {0x78,0x60,0x60,0x60,0x60,0x60,0x78,0x00}, /* [ */
    {0xC0,0x60,0x30,0x18,0x0C,0x06,0x03,0x00}, /* \ */
    {0x78,0x18,0x18,0x18,0x18,0x18,0x78,0x00}, /* ] */
    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00}, /* ^ */
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, /* _ */
    {0x18,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, /* ` */
    {0x00,0x00,0x7C,0x06,0x7E,0xC6,0x7E,0x00}, /* a */
    {0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xFC,0x00}, /* b */
    {0x00,0x00,0x7C,0xC6,0xC0,0xC6,0x7C,0x00}, /* c */
    {0x06,0x06,0x7E,0xC6,0xC6,0xC6,0x7E,0x00}, /* d */
    {0x00,0x00,0x7C,0xC6,0xFE,0xC0,0x7C,0x00}, /* e */
    {0x1E,0x30,0x7C,0x30,0x30,0x30,0x30,0x00}, /* f */
    {0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x7C}, /* g */
    {0xC0,0xC0,0xFC,0xC6,0xC6,0xC6,0xC6,0x00}, /* h */
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, /* i */
    {0x06,0x00,0x06,0x06,0x06,0xC6,0xC6,0x7C}, /* j */
    {0xC0,0xC0,0xC6,0xCC,0xD8,0xCC,0xC6,0x00}, /* k */
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, /* l */
    {0x00,0x00,0xCC,0xFE,0xFE,0xD6,0xC6,0x00}, /* m */
    {0x00,0x00,0xFC,0xC6,0xC6,0xC6,0xC6,0x00}, /* n */
    {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0x00}, /* o */
    {0x00,0x00,0xFC,0xC6,0xC6,0xFC,0xC0,0xC0}, /* p */
    {0x00,0x00,0x7E,0xC6,0xC6,0x7E,0x06,0x06}, /* q */
    {0x00,0x00,0xDC,0xE6,0xC0,0xC0,0xC0,0x00}, /* r */
    {0x00,0x00,0x7C,0xC0,0x7C,0x06,0xFC,0x00}, /* s */
    {0x30,0x30,0xFE,0x30,0x30,0x30,0x1E,0x00}, /* t */
    {0x00,0x00,0xC6,0xC6,0xC6,0xC6,0x7E,0x00}, /* u */
    {0x00,0x00,0xC6,0xC6,0xC6,0x6C,0x38,0x00}, /* v */
    {0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x6C,0x00}, /* w */
    {0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00}, /* x */
    {0x00,0x00,0xC6,0xC6,0xC6,0x7E,0x06,0xFC}, /* y */
    {0x00,0x00,0xFE,0x0C,0x18,0x30,0xFE,0x00}, /* z */
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, /* { */
    {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, /* | */
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, /* } */
    {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00}, /* ~ */
};

/* ── global state ───────────────────────────────────────────────── */
static struct {
    SDL_Window*   win    = nullptr;
    SDL_Renderer* ren    = nullptr;
    bool          quit   = false;
    int           target_fps = 0;
    uint64_t      freq = 0, prev = 0;
    float         dt = 0.0f;
    int           fps = 0;
    uint8_t  key_cur[SDL_NUM_SCANCODES]  = {};
    uint8_t  key_prev[SDL_NUM_SCANCODES] = {};
    uint32_t mouse_cur = 0, mouse_prev = 0;
    float    mx = 0, my = 0, mdx = 0, mdy = 0, scroll = 0;
    char     last_char = 0;
    /* camera 2D */
    float cam_tx = 0.0f, cam_ty = 0.0f;
    float cam_ox = 0.0f, cam_oy = 0.0f;
    float cam_zoom = 1.0f;
    /* gamepad (slots 0-3) */
    SDL_GameController* pads[4]  = {};
    uint8_t pad_btn_cur[4][SDL_CONTROLLER_BUTTON_MAX]  = {};
    uint8_t pad_btn_prev[4][SDL_CONTROLLER_BUTTON_MAX] = {};
    int16_t pad_axis[4][SDL_CONTROLLER_AXIS_MAX]       = {};
    /* audio */
    bool audio_open = false;
} _mk;

/* ── camera coordinate helpers (identity when zoom=1, target/offset=0) */
static inline float _mk_cx(float x) { return (x - _mk.cam_tx) * _mk.cam_zoom + _mk.cam_ox; }
static inline float _mk_cy(float y) { return (y - _mk.cam_ty) * _mk.cam_zoom + _mk.cam_oy; }
static inline float _mk_cs(float s) { return s * _mk.cam_zoom; }

/* ── window & core loop ─────────────────────────────────────────── */
static inline bool _olrn_mk_init_window(int32_t w, int32_t h, const std::string& title) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) return false;
    _mk.win = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, w, h, 0);
    if (!_mk.win) { SDL_Quit(); return false; }
    _mk.ren = SDL_CreateRenderer(_mk.win, -1, 0);
    if (!_mk.ren) { SDL_DestroyWindow(_mk.win); SDL_Quit(); return false; }
    SDL_SetRenderDrawBlendMode(_mk.ren, SDL_BLENDMODE_BLEND);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);
    _mk.freq = SDL_GetPerformanceFrequency();
    _mk.prev = SDL_GetPerformanceCounter();
    for (int i = 0; i < SDL_NumJoysticks() && i < 4; i++)
        if (SDL_IsGameController(i)) _mk.pads[i] = SDL_GameControllerOpen(i);
    return true; }
static inline void _olrn_mk_close_window() {
    for (int i = 0; i < 4; i++)
        if (_mk.pads[i]) { SDL_GameControllerClose(_mk.pads[i]); _mk.pads[i] = nullptr; }
    if (_mk.ren) SDL_DestroyRenderer(_mk.ren);
    if (_mk.win) SDL_DestroyWindow(_mk.win);
    _mk.ren = nullptr; _mk.win = nullptr;
    if (_mk.audio_open) { Mix_CloseAudio(); Mix_Quit(); _mk.audio_open = false; }
    IMG_Quit();
    SDL_Quit(); }
static inline bool _olrn_mk_should_close() {
    memcpy(_mk.key_prev, _mk.key_cur, SDL_NUM_SCANCODES);
    _mk.mouse_prev = _mk.mouse_cur;
    _mk.scroll = 0; _mk.last_char = 0;
    for (int i = 0; i < 4; i++)
        if (_mk.pads[i]) memcpy(_mk.pad_btn_prev[i], _mk.pad_btn_cur[i], SDL_CONTROLLER_BUTTON_MAX);
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if      (e.type == SDL_QUIT)        _mk.quit = true;
        else if (e.type == SDL_MOUSEWHEEL)  _mk.scroll += (float)e.wheel.y;
        else if (e.type == SDL_TEXTINPUT)   _mk.last_char = e.text.text[0];
        else if (e.type == SDL_CONTROLLERDEVICEADDED) {
            int di = e.cdevice.which;
            if (SDL_IsGameController(di))
                for (int i = 0; i < 4; i++)
                    if (!_mk.pads[i]) { _mk.pads[i] = SDL_GameControllerOpen(di); break; }
        } else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
            SDL_JoystickID jid = e.cdevice.which;
            for (int i = 0; i < 4; i++) {
                if (!_mk.pads[i]) continue;
                if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(_mk.pads[i])) == jid) {
                    SDL_GameControllerClose(_mk.pads[i]); _mk.pads[i] = nullptr;
                    memset(_mk.pad_btn_cur[i],  0, SDL_CONTROLLER_BUTTON_MAX);
                    memset(_mk.pad_btn_prev[i], 0, SDL_CONTROLLER_BUTTON_MAX);
                    memset(_mk.pad_axis[i],     0, SDL_CONTROLLER_AXIS_MAX * sizeof(int16_t));
                }
            }
        }
    }
    int nk = 0; const Uint8* ks = SDL_GetKeyboardState(&nk);
    memcpy(_mk.key_cur, ks, nk < SDL_NUM_SCANCODES ? nk : SDL_NUM_SCANCODES);
    int mx2, my2; _mk.mouse_cur = SDL_GetMouseState(&mx2, &my2);
    _mk.mdx = (float)mx2 - _mk.mx; _mk.mdy = (float)my2 - _mk.my;
    _mk.mx = (float)mx2; _mk.my = (float)my2;
    for (int i = 0; i < 4; i++) {
        if (!_mk.pads[i]) continue;
        for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX; b++)
            _mk.pad_btn_cur[i][b] = SDL_GameControllerGetButton(_mk.pads[i], (SDL_GameControllerButton)b);
        for (int a = 0; a < SDL_CONTROLLER_AXIS_MAX; a++)
            _mk.pad_axis[i][a] = SDL_GameControllerGetAxis(_mk.pads[i], (SDL_GameControllerAxis)a);
    }
    return _mk.quit; }
static inline void _olrn_mk_begin_draw() { (void)0; }
static inline void _olrn_mk_end_draw() {
    SDL_RenderPresent(_mk.ren);
    if (_mk.target_fps > 0) {
        uint64_t tgt = _mk.freq / (uint64_t)_mk.target_fps;
        while (SDL_GetPerformanceCounter() - _mk.prev < tgt) SDL_Delay(1);
    }
    uint64_t now = SDL_GetPerformanceCounter();
    _mk.dt  = (float)(now - _mk.prev) / (float)_mk.freq;
    _mk.fps = _mk.dt > 0.0f ? (int)(1.0f / _mk.dt + 0.5f) : 0;
    _mk.prev = now; }
static inline void  _olrn_mk_clear_bg(Color c) {
    SDL_SetRenderDrawColor(_mk.ren, c.r, c.g, c.b, c.a);
    SDL_RenderClear(_mk.ren); }
static inline void    _olrn_mk_set_fps(int32_t n)  { _mk.target_fps = n; }
static inline float   _olrn_mk_dt()                { return _mk.dt; }
static inline int32_t _olrn_mk_fps()               { return _mk.fps; }
static inline void    _olrn_mk_set_vsync(bool on)  {
#if SDL_VERSION_ATLEAST(2, 0, 18)
    SDL_RenderSetVSync(_mk.ren, on ? 1 : 0);
#else
    (void)on;
#endif
}
static inline void    _olrn_mk_set_title(const std::string& t) {
    SDL_SetWindowTitle(_mk.win, t.c_str()); }
static inline void    _olrn_mk_fullscreen(bool on) {
    SDL_SetWindowFullscreen(_mk.win, on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0); }
static inline int32_t _olrn_mk_width()  { int w,h; SDL_GetWindowSize(_mk.win,&w,&h); return w; }
static inline int32_t _olrn_mk_height() { int w,h; SDL_GetWindowSize(_mk.win,&w,&h); return h; }

/* ── keyboard ───────────────────────────────────────────────────── */
static inline bool _olrn_mk_key_down(int32_t k)     { return _mk.key_cur[k] != 0; }
static inline bool _olrn_mk_key_pressed(int32_t k)  { return _mk.key_cur[k] && !_mk.key_prev[k]; }
static inline bool _olrn_mk_key_released(int32_t k) { return !_mk.key_cur[k] && _mk.key_prev[k]; }
static inline std::string _olrn_mk_key_char() {
    return _mk.last_char ? std::string(1, _mk.last_char) : std::string(); }

/* ── mouse ──────────────────────────────────────────────────────── */
static inline Vec2  _olrn_mk_mouse_pos()             { return Vec2{_mk.mx,_mk.my}; }
static inline Vec2  _olrn_mk_mouse_delta()           { return Vec2{_mk.mdx,_mk.mdy}; }
static inline float _olrn_mk_mouse_scroll()          { return _mk.scroll; }
static inline bool  _olrn_mk_mouse_btn(int32_t b)    { return (_mk.mouse_cur  & SDL_BUTTON(b)) != 0; }
static inline bool  _olrn_mk_mouse_btn_pressed(int32_t b) {
    return (_mk.mouse_cur & SDL_BUTTON(b)) && !(_mk.mouse_prev & SDL_BUTTON(b)); }
static inline bool  _olrn_mk_mouse_btn_released(int32_t b) {
    return !(_mk.mouse_cur & SDL_BUTTON(b)) && (_mk.mouse_prev & SDL_BUTTON(b)); }
static inline void  _olrn_mk_mouse_set_pos(Vec2 p) {
    SDL_WarpMouseInWindow(_mk.win, (int)p.x, (int)p.y); }
static inline void  _olrn_mk_mouse_hide() { SDL_ShowCursor(SDL_DISABLE); }
static inline void  _olrn_mk_mouse_show() { SDL_ShowCursor(SDL_ENABLE); }

/* ── gamepad ────────────────────────────────────────────────────── */
static inline bool  _olrn_mk_pad_connected(int32_t id) {
    return id >= 0 && id < 4 && _mk.pads[id] != nullptr; }
static inline bool  _olrn_mk_pad_btn(int32_t id, int32_t btn) {
    if (id < 0 || id >= 4 || btn < 0 || btn >= SDL_CONTROLLER_BUTTON_MAX) return false;
    return _mk.pad_btn_cur[id][btn] != 0; }
static inline bool  _olrn_mk_pad_btn_pressed(int32_t id, int32_t btn) {
    if (id < 0 || id >= 4 || btn < 0 || btn >= SDL_CONTROLLER_BUTTON_MAX) return false;
    return _mk.pad_btn_cur[id][btn] && !_mk.pad_btn_prev[id][btn]; }
static inline float _olrn_mk_pad_axis(int32_t id, int32_t axis) {
    if (id < 0 || id >= 4 || axis < 0 || axis >= SDL_CONTROLLER_AXIS_MAX) return 0.0f;
    int16_t v = _mk.pad_axis[id][axis];
    return v < 0 ? (float)v / 32768.0f : (float)v / 32767.0f; }

/* ── 2D drawing (all positions/sizes in world space; camera applied) */
static inline void _olrn_mk_draw_rect(float x, float y, float w, float h, Color c) {
    SDL_SetRenderDrawColor(_mk.ren, c.r, c.g, c.b, c.a);
    SDL_FRect r{_mk_cx(x), _mk_cy(y), _mk_cs(w), _mk_cs(h)};
    SDL_RenderFillRectF(_mk.ren, &r); }
static inline void _olrn_mk_draw_rect_lines(float x, float y, float w, float h, float t, Color c) {
    _olrn_mk_draw_rect(x,       y,           w, t, c);
    _olrn_mk_draw_rect(x,       y + h - t,   w, t, c);
    _olrn_mk_draw_rect(x,       y + t,        t, h - 2*t, c);
    _olrn_mk_draw_rect(x + w - t, y + t,      t, h - 2*t, c); }
static inline void _olrn_mk_draw_rect_rot(float x, float y, float w, float h,
                                           Vec2 origin, float rot_deg, Color c) {
    float rad = rot_deg * 3.14159265f / 180.0f;
    float ca = SDL_cosf(rad), sa = SDL_sinf(rad);
    float lx[4] = {-origin.x, w-origin.x, w-origin.x, -origin.x};
    float ly[4] = {-origin.y, -origin.y,   h-origin.y,  h-origin.y};
    SDL_Color sc{c.r, c.g, c.b, c.a};
    SDL_Vertex v[4];
    for (int i = 0; i < 4; i++) {
        float rx = lx[i]*ca - ly[i]*sa;
        float ry = lx[i]*sa + ly[i]*ca;
        v[i] = {{_mk_cx(x+rx), _mk_cy(y+ry)}, sc, {0,0}};
    }
    int idx[6] = {0,1,2,0,2,3};
    SDL_RenderGeometry(_mk.ren, nullptr, v, 4, idx, 6); }
static inline void _olrn_mk_draw_line(float x1, float y1, float x2, float y2, float t, Color c) {
    SDL_SetRenderDrawColor(_mk.ren, c.r, c.g, c.b, c.a);
    if (_mk_cs(t) <= 1.0f) {
        SDL_RenderDrawLineF(_mk.ren, _mk_cx(x1), _mk_cy(y1), _mk_cx(x2), _mk_cy(y2)); return;
    }
    float dx = x2-x1, dy = y2-y1;
    float len = SDL_sqrtf(dx*dx + dy*dy);
    if (len <= 0.0f) return;
    float ht = _mk_cs(t) * 0.5f;
    float px = -dy/len * ht, py = dx/len * ht;
    float sx1=_mk_cx(x1), sy1=_mk_cy(y1), sx2=_mk_cx(x2), sy2=_mk_cy(y2);
    SDL_Color sc{c.r, c.g, c.b, c.a};
    SDL_Vertex vv[4] = {
        {{sx1+px,sy1+py},sc,{0,0}}, {{sx1-px,sy1-py},sc,{0,0}},
        {{sx2-px,sy2-py},sc,{0,0}}, {{sx2+px,sy2+py},sc,{0,0}} };
    int idx[6] = {0,1,2,0,2,3};
    SDL_RenderGeometry(_mk.ren, nullptr, vv, 4, idx, 6); }
static inline void _olrn_mk_draw_fan(float cx, float cy, float r, int n, float rot, Color c) {
    if (n < 3) n = 3;
    if (n > 128) n = 128;
    float scx=_mk_cx(cx), scy=_mk_cy(cy), sr=_mk_cs(r);
    SDL_Color sc{c.r, c.g, c.b, c.a};
    SDL_Vertex vv[130]; int idx[128*3];
    vv[0] = {{scx, scy}, sc, {0,0}};
    for (int i = 0; i <= n; i++) {
        float a = rot + (float)i/(float)n * 6.28318530718f;
        vv[i+1] = {{scx + SDL_cosf(a)*sr, scy + SDL_sinf(a)*sr}, sc, {0,0}};
    }
    for (int i = 0; i < n; i++) { idx[i*3]=0; idx[i*3+1]=i+1; idx[i*3+2]=i+2; }
    SDL_RenderGeometry(_mk.ren, nullptr, vv, n+2, idx, n*3); }
static inline void _olrn_mk_draw_circle(float cx, float cy, float r, Color c) {
    int n = (int)(r * 0.8f); if (n < 12) n = 12; if (n > 64) n = 64;
    _olrn_mk_draw_fan(cx, cy, r, n, 0.0f, c); }
static inline void _olrn_mk_draw_circle_lines(float cx, float cy, float r, Color c) {
    SDL_SetRenderDrawColor(_mk.ren, c.r, c.g, c.b, c.a);
    int n = (int)(r * 0.8f); if (n < 12) n = 12; if (n > 64) n = 64;
    float scx=_mk_cx(cx), scy=_mk_cy(cy), sr=_mk_cs(r);
    float px = scx+sr, py = scy;
    for (int i = 1; i <= n; i++) {
        float a = (float)i/(float)n * 6.28318530718f;
        float nx = scx+SDL_cosf(a)*sr, ny = scy+SDL_sinf(a)*sr;
        SDL_RenderDrawLineF(_mk.ren, px, py, nx, ny);
        px = nx; py = ny;
    } }
static inline void _olrn_mk_draw_tri(Vec2 a, Vec2 b, Vec2 cc, Color c) {
    SDL_Color sc{c.r, c.g, c.b, c.a};
    SDL_Vertex vv[3] = {
        {{_mk_cx(a.x),_mk_cy(a.y)},sc,{0,0}},
        {{_mk_cx(b.x),_mk_cy(b.y)},sc,{0,0}},
        {{_mk_cx(cc.x),_mk_cy(cc.y)},sc,{0,0}} };
    SDL_RenderGeometry(_mk.ren, nullptr, vv, 3, nullptr, 0); }
static inline void _olrn_mk_draw_poly(Vec2 center, int32_t sides, float r, float rot, Color c) {
    _olrn_mk_draw_fan(center.x, center.y, r, sides, rot, c); }

/* ── textures ───────────────────────────────────────────────────── */
static inline int64_t _olrn_mk_load_texture(const std::string& path) {
    SDL_Surface* s = IMG_Load(path.c_str());
    if (!s) return 0;
    SDL_Texture* t = SDL_CreateTextureFromSurface(_mk.ren, s);
    SDL_FreeSurface(s);
    return (int64_t)(uintptr_t)t; }
static inline void _olrn_mk_unload_texture(int64_t t) {
    if (t) SDL_DestroyTexture((SDL_Texture*)(uintptr_t)t); }
static inline void _olrn_mk_draw_texture(int64_t t, float x, float y, Color tint) {
    SDL_Texture* tex = (SDL_Texture*)(uintptr_t)t;
    int w,h; SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(tex, tint.a);
    SDL_FRect dst{_mk_cx(x), _mk_cy(y), _mk_cs((float)w), _mk_cs((float)h)};
    SDL_RenderCopyF(_mk.ren, tex, nullptr, &dst); }
static inline void _olrn_mk_draw_texture_ex(int64_t t, Vec2 pos, float rot, float scale, Color tint) {
    SDL_Texture* tex = (SDL_Texture*)(uintptr_t)t;
    int w,h; SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
    SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(tex, tint.a);
    SDL_FRect dst{_mk_cx(pos.x), _mk_cy(pos.y), _mk_cs((float)w*scale), _mk_cs((float)h*scale)};
    SDL_RenderCopyExF(_mk.ren, tex, nullptr, &dst, rot, nullptr, SDL_FLIP_NONE); }
static inline void _olrn_mk_draw_texture_rect(int64_t t, Rect src, Rect dst, Color tint) {
    SDL_Texture* tex = (SDL_Texture*)(uintptr_t)t;
    SDL_SetTextureColorMod(tex, tint.r, tint.g, tint.b);
    SDL_SetTextureAlphaMod(tex, tint.a);
    SDL_Rect  srect{(int)src.x,(int)src.y,(int)src.w,(int)src.h};
    SDL_FRect drect{_mk_cx(dst.x),_mk_cy(dst.y),_mk_cs(dst.w),_mk_cs(dst.h)};
    SDL_RenderCopyF(_mk.ren, tex, &srect, &drect); }

/* ── camera 2D ──────────────────────────────────────────────────── */
static inline void _olrn_mk_begin_camera2d(Camera2D cam) {
    _mk.cam_tx = cam.target.x; _mk.cam_ty = cam.target.y;
    _mk.cam_ox = cam.offset.x; _mk.cam_oy = cam.offset.y;
    _mk.cam_zoom = cam.zoom > 0.0f ? cam.zoom : 1.0f; }
static inline void _olrn_mk_end_camera2d() {
    _mk.cam_tx = 0.0f; _mk.cam_ty = 0.0f;
    _mk.cam_ox = 0.0f; _mk.cam_oy = 0.0f;
    _mk.cam_zoom = 1.0f; }
static inline Vec2 _olrn_mk_screen_to_world2d(Vec2 pos, Camera2D cam) {
    float z = cam.zoom > 0.0f ? cam.zoom : 1.0f;
    return Vec2{(pos.x - cam.offset.x)/z + cam.target.x,
                (pos.y - cam.offset.y)/z + cam.target.y}; }
static inline Vec2 _olrn_mk_world_to_screen2d(Vec2 pos, Camera2D cam) {
    float z = cam.zoom > 0.0f ? cam.zoom : 1.0f;
    return Vec2{(pos.x - cam.target.x)*z + cam.offset.x,
                (pos.y - cam.target.y)*z + cam.offset.y}; }

/* ── text (embedded 8x8 bitmap font) ────────────────────────────── */
static inline void _olrn_mk_draw_text(const std::string& text, float x, float y,
                                       float size, Color c) {
    float scale = size / 8.0f;
    SDL_SetRenderDrawColor(_mk.ren, c.r, c.g, c.b, c.a);
    float gx = x;
    for (unsigned char ch : text) {
        if (ch < 32 || ch > 126) { gx += size; continue; }
        const uint8_t* g = _mk_font8[ch - 32];
        for (int row = 0; row < 8; row++) {
            uint8_t bits = g[row];
            if (!bits) continue;
            for (int col = 0; col < 8; col++) {
                if (!(bits & (0x80 >> col))) continue;
                float ps = _mk_cs(scale); if (ps < 1.0f) ps = 1.0f;
                SDL_FRect r{_mk_cx(gx + col*scale), _mk_cy(y + row*scale), ps, ps};
                SDL_RenderFillRectF(_mk.ren, &r);
            }
        }
        gx += size;
    } }
static inline Vec2 _olrn_mk_measure_text(const std::string& text, float size) {
    return Vec2{(float)text.size() * size, size}; }
/* ── audio (SDL_mixer) ─────────────────────────────────────────── */
static inline bool _olrn_mk_init_audio() {
    if (_mk.audio_open) return true;
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) return false;
    _mk.audio_open = true; return true; }
static inline void _olrn_mk_close_audio() {
    if (!_mk.audio_open) return;
    Mix_CloseAudio(); Mix_Quit(); _mk.audio_open = false; }
static inline int64_t _olrn_mk_load_sound(const std::string& path) {
    return (int64_t)(uintptr_t)Mix_LoadWAV(path.c_str()); }
static inline void _olrn_mk_unload_sound(int64_t s) {
    if (s) Mix_FreeChunk((Mix_Chunk*)(uintptr_t)s); }
static inline void _olrn_mk_play_sound(int64_t s) {
    if (s) Mix_PlayChannel(-1, (Mix_Chunk*)(uintptr_t)s, 0); }
static inline void _olrn_mk_stop_sounds() { Mix_HaltChannel(-1); }
static inline void _olrn_mk_sound_set_volume(int64_t s, float vol) {
    if (s) Mix_VolumeChunk((Mix_Chunk*)(uintptr_t)s,
                           (int)(vol * MIX_MAX_VOLUME)); }
static inline int64_t _olrn_mk_load_music(const std::string& path) {
    return (int64_t)(uintptr_t)Mix_LoadMUS(path.c_str()); }
static inline void _olrn_mk_unload_music(int64_t m) {
    if (m) Mix_FreeMusic((Mix_Music*)(uintptr_t)m); }
static inline void _olrn_mk_play_music(int64_t m, int32_t loops) {
    if (m) Mix_PlayMusic((Mix_Music*)(uintptr_t)m, loops); }
static inline void _olrn_mk_stop_music() { Mix_HaltMusic(); }
static inline void _olrn_mk_pause_music() { Mix_PauseMusic(); }
static inline void _olrn_mk_resume_music() { Mix_ResumeMusic(); }
static inline bool _olrn_mk_music_playing() {
    return Mix_PlayingMusic() != 0 && Mix_PausedMusic() == 0; }
static inline bool _olrn_mk_music_paused() { return Mix_PausedMusic() != 0; }
static inline void _olrn_mk_music_set_volume(float vol) {
    Mix_VolumeMusic((int)(vol * MIX_MAX_VOLUME)); }
/* ── end malkur ─────────────────────────────────────────────────── */
namespace _olrn_err_MalkurError {
    static constexpr _OlrnError InitFailed{0, "InitFailed"};
    static constexpr _OlrnError LoadFailed{1, "LoadFailed"};
    static constexpr _OlrnError AudioFailed{2, "AudioFailed"};
}

using File = int64_t;
using Texture = int64_t;
using Sound = int64_t;
using Music = int64_t;

namespace IOMode {
    static constexpr int32_t Read = 0;
    static constexpr int32_t Write = 1;
    static constexpr int32_t ReadWrite = 2;
    static constexpr int32_t Append = 3;
}
namespace SeekFrom {
    static constexpr int32_t Start = 0;
    static constexpr int32_t Current = 1;
    static constexpr int32_t End = 2;
}
namespace keys {
    static constexpr int32_t A = 4;
    static constexpr int32_t B = 5;
    static constexpr int32_t C = 6;
    static constexpr int32_t D = 7;
    static constexpr int32_t E = 8;
    static constexpr int32_t F = 9;
    static constexpr int32_t G = 10;
    static constexpr int32_t H = 11;
    static constexpr int32_t I = 12;
    static constexpr int32_t J = 13;
    static constexpr int32_t K = 14;
    static constexpr int32_t L = 15;
    static constexpr int32_t M = 16;
    static constexpr int32_t N = 17;
    static constexpr int32_t O = 18;
    static constexpr int32_t P = 19;
    static constexpr int32_t Q = 20;
    static constexpr int32_t R = 21;
    static constexpr int32_t S = 22;
    static constexpr int32_t T = 23;
    static constexpr int32_t U = 24;
    static constexpr int32_t V = 25;
    static constexpr int32_t W = 26;
    static constexpr int32_t X = 27;
    static constexpr int32_t Y = 28;
    static constexpr int32_t Z = 29;
    static constexpr int32_t N1 = 30;
    static constexpr int32_t N2 = 31;
    static constexpr int32_t N3 = 32;
    static constexpr int32_t N4 = 33;
    static constexpr int32_t N5 = 34;
    static constexpr int32_t N6 = 35;
    static constexpr int32_t N7 = 36;
    static constexpr int32_t N8 = 37;
    static constexpr int32_t N9 = 38;
    static constexpr int32_t N0 = 39;
    static constexpr int32_t ENTER = 40;
    static constexpr int32_t ESCAPE = 41;
    static constexpr int32_t BACKSPACE = 42;
    static constexpr int32_t TAB = 43;
    static constexpr int32_t SPACE = 44;
    static constexpr int32_t F1 = 58;
    static constexpr int32_t F2 = 59;
    static constexpr int32_t F3 = 60;
    static constexpr int32_t F4 = 61;
    static constexpr int32_t F5 = 62;
    static constexpr int32_t F6 = 63;
    static constexpr int32_t F7 = 64;
    static constexpr int32_t F8 = 65;
    static constexpr int32_t F9 = 66;
    static constexpr int32_t F10 = 67;
    static constexpr int32_t F11 = 68;
    static constexpr int32_t F12 = 69;
    static constexpr int32_t RIGHT = 79;
    static constexpr int32_t LEFT = 80;
    static constexpr int32_t DOWN = 81;
    static constexpr int32_t UP = 82;
    static constexpr int32_t LCTRL = 224;
    static constexpr int32_t LSHIFT = 225;
    static constexpr int32_t LALT = 226;
    static constexpr int32_t RCTRL = 228;
    static constexpr int32_t RSHIFT = 229;
    static constexpr int32_t RALT = 230;
}
namespace mbtn {
    static constexpr int32_t LEFT = 1;
    static constexpr int32_t MIDDLE = 2;
    static constexpr int32_t RIGHT = 3;
}
namespace pad_btn {
    static constexpr int32_t A = 0;
    static constexpr int32_t B = 1;
    static constexpr int32_t X = 2;
    static constexpr int32_t Y = 3;
    static constexpr int32_t BACK = 4;
    static constexpr int32_t GUIDE = 5;
    static constexpr int32_t START = 6;
    static constexpr int32_t LS = 7;
    static constexpr int32_t RS = 8;
    static constexpr int32_t LB = 9;
    static constexpr int32_t RB = 10;
    static constexpr int32_t UP = 11;
    static constexpr int32_t DOWN = 12;
    static constexpr int32_t LEFT = 13;
    static constexpr int32_t RIGHT = 14;
}
namespace pad_axis {
    static constexpr int32_t LEFTX = 0;
    static constexpr int32_t LEFTY = 1;
    static constexpr int32_t RIGHTX = 2;
    static constexpr int32_t RIGHTY = 3;
    static constexpr int32_t LT = 4;
    static constexpr int32_t RT = 5;
}

constexpr auto PI = 3.14159;
constexpr auto TAU = 6.28319;
constexpr auto E = 2.71828;
constexpr Color BLACK = Color{.r = (int64_t)0, .g = (int64_t)0, .b = (int64_t)0, .a = (int64_t)255};
constexpr Color WHITE = Color{.r = (int64_t)255, .g = (int64_t)255, .b = (int64_t)255, .a = (int64_t)255};
constexpr Color RED = Color{.r = (int64_t)230, .g = (int64_t)41, .b = (int64_t)55, .a = (int64_t)255};
constexpr Color GREEN = Color{.r = (int64_t)0, .g = (int64_t)228, .b = (int64_t)48, .a = (int64_t)255};
constexpr Color BLUE = Color{.r = (int64_t)0, .g = (int64_t)121, .b = (int64_t)241, .a = (int64_t)255};
constexpr Color YELLOW = Color{.r = (int64_t)253, .g = (int64_t)249, .b = (int64_t)0, .a = (int64_t)255};
constexpr Color ORANGE = Color{.r = (int64_t)255, .g = (int64_t)161, .b = (int64_t)0, .a = (int64_t)255};
constexpr Color PURPLE = Color{.r = (int64_t)200, .g = (int64_t)122, .b = (int64_t)255, .a = (int64_t)255};
constexpr Color PINK = Color{.r = (int64_t)255, .g = (int64_t)109, .b = (int64_t)194, .a = (int64_t)255};
constexpr Color GRAY = Color{.r = (int64_t)130, .g = (int64_t)130, .b = (int64_t)130, .a = (int64_t)255};
constexpr Color DARKGRAY = Color{.r = (int64_t)80, .g = (int64_t)80, .b = (int64_t)80, .a = (int64_t)255};
constexpr Color TRANSPARENT = Color{.r = (int64_t)0, .g = (int64_t)0, .b = (int64_t)0, .a = (int64_t)0};

template<typename T_f>
int64_t thread_spawn(T_f f)
{
    return _olrn_thread_spawn(f);
}

template<typename T_f>
int64_t thread_spawn_arg(T_f f, int64_t arg)
{
    return _olrn_thread_spawn_arg(f, arg);
}

void thread_join(int64_t t)
{
    _olrn_thread_join(t);
}

void thread_detach(int64_t t)
{
    _olrn_thread_detach(t);
}

void thread_yield()
{
    _olrn_thread_yield();
}

int64_t thread_id()
{
    return _olrn_thread_id();
}

int32_t thread_cores()
{
    return _olrn_thread_cores();
}

int64_t thread_mutex_new()
{
    return _olrn_mutex_new();
}

void thread_mutex_lock(int64_t m)
{
    _olrn_mutex_lock(m);
}

void thread_mutex_unlock(int64_t m)
{
    _olrn_mutex_unlock(m);
}

void thread_mutex_free(int64_t m)
{
    _olrn_mutex_free(m);
}

int64_t thread_atomic_new(int32_t val)
{
    return _olrn_atomic_new(val);
}

int32_t thread_atomic_load(int64_t a)
{
    return _olrn_atomic_load(a);
}

void thread_atomic_store(int64_t a, int32_t val)
{
    _olrn_atomic_store(a, val);
}

int32_t thread_atomic_add(int64_t a, int32_t val)
{
    return _olrn_atomic_add(a, val);
}

bool thread_atomic_cas(int64_t a, int32_t expected, int32_t desired)
{
    return _olrn_atomic_cas(a, expected, desired);
}

void thread_atomic_free(int64_t a)
{
    _olrn_atomic_free(a);
}

void log_debug(std::string msg)
{
    (((std::cout << "[DEBUG] ") << msg) << std::endl);
}

void log_info(std::string msg)
{
    (((std::cout << "[INFO]  ") << msg) << std::endl);
}

void log_warn(std::string msg)
{
    (((std::cout << "[WARN]  ") << msg) << std::endl);
}

void log_error(std::string msg)
{
    (((std::cout << "[ERROR] ") << msg) << std::endl);
}

void log_fatal(std::string msg)
{
    (((std::cout << "[FATAL] ") << msg) << std::endl);
        { std::cerr << "panic: " << msg << std::endl; std::abort(); }
}

int64_t str_len(std::string s)
{
    return _olrn_str_len(s);
}

std::string str_from_int(int64_t n)
{
    return std::to_string(n);
}

std::string str_from_f64(double x)
{
    return std::to_string(x);
}

int64_t str_parse_int(std::string s)
{
    return _olrn_str_parse_int(s);
}

double str_parse_f64(std::string s)
{
    return _olrn_str_parse_f64(s);
}

std::string str_to_upper(std::string s)
{
    return _olrn_str_upper(s);
}

std::string str_to_lower(std::string s)
{
    return _olrn_str_lower(s);
}

std::string str_trim(std::string s)
{
    return _olrn_str_trim(s);
}

bool str_starts_with(std::string s, std::string prefix)
{
    return _olrn_str_starts(s, prefix);
}

bool str_ends_with(std::string s, std::string suffix)
{
    return _olrn_str_ends(s, suffix);
}

bool str_contains(std::string s, std::string sub)
{
    return _olrn_str_contains(s, sub);
}

void mem_copy(uint8_t * dst, uint8_t * src, int64_t n)
{
    std::memcpy(dst, src, n);
}

void mem_set(uint8_t * ptr, uint8_t val, int64_t n)
{
    std::memset(ptr, val, n);
}

void mem_zero(uint8_t * ptr, int64_t n)
{
    std::memset(ptr, (int64_t)0, n);
}

int64_t mem_kb(int64_t n)
{
    return (n * (int64_t)1024);
}

int64_t mem_mb(int64_t n)
{
    return ((n * (int64_t)1024) * (int64_t)1024);
}

int64_t mem_gb(int64_t n)
{
    return (((n * (int64_t)1024) * (int64_t)1024) * (int64_t)1024);
}

double math_sqrt(double x)
{
    return _olrn_sqrt(x);
}

double math_abs(double x)
{
    return _olrn_fabs(x);
}

double math_sin(double x)
{
    return _olrn_sin(x);
}

double math_cos(double x)
{
    return _olrn_cos(x);
}

double math_tan(double x)
{
    return _olrn_tan(x);
}

double math_asin(double x)
{
    return _olrn_asin(x);
}

double math_acos(double x)
{
    return _olrn_acos(x);
}

double math_atan(double x)
{
    return _olrn_atan(x);
}

double math_atan2(double y, double x)
{
    return _olrn_atan2(y, x);
}

double math_floor(double x)
{
    return _olrn_floor(x);
}

double math_ceil(double x)
{
    return _olrn_ceil(x);
}

double math_round(double x)
{
    return _olrn_round(x);
}

double math_pow(double b, double e)
{
    return _olrn_pow(b, e);
}

double math_log(double x)
{
    return _olrn_log(x);
}

double math_log2(double x)
{
    return _olrn_log2(x);
}

double math_exp(double x)
{
    return _olrn_exp(x);
}

double math_clamp(double v, double lo, double hi)
{
    if ((v < lo)) {
        return lo;
    }
    if ((v > hi)) {
        return hi;
    }
    return v;
}

double math_lerp(double a, double b, double t)
{
    return (a + ((b - a) * t));
}

double math_min(double a, double b)
{
    if ((a < b)) {
        return a;
    }
    return b;
}

double math_max(double a, double b)
{
    if ((a > b)) {
        return a;
    }
    return b;
}

double math_deg_to_rad(double deg)
{
    return ((deg * PI) / 180.0);
}

double math_rad_to_deg(double rad)
{
    return ((rad * 180.0) / PI);
}

double math_sign(double x)
{
    if ((x > 0.0)) {
        return 1.0;
    }
    if ((x < 0.0)) {
        return -1.0;
    }
    return 0.0;
}

int64_t time_now()
{
    return _olrn_time_now();
}

int64_t time_mono()
{
    return _olrn_time_mono();
}

void time_sleep(double secs)
{
    _olrn_time_sleep(_olrn_cast<int64_t>((secs * 1e+09)));
}

double time_since(int64_t t0)
{
    return (_olrn_cast<double>((time_mono() - t0)) / 1e+09);
}

int64_t time_sw_start()
{
    return time_mono();
}

double time_sw_elapsed(int64_t start)
{
    return time_since(start);
}

double time_ns_to_ms(int64_t ns)
{
    return (_olrn_cast<double>(ns) / 1e+06);
}

double time_ns_to_sec(int64_t ns)
{
    return (_olrn_cast<double>(ns) / 1e+09);
}

bool fs_exists(std::string path)
{
    return _olrn_fs_exists(path);
}

bool fs_is_dir(std::string path)
{
    return _olrn_fs_is_dir(path);
}

bool fs_is_file(std::string path)
{
    return _olrn_fs_is_file(path);
}

bool fs_mkdir(std::string path)
{
    return _olrn_fs_mkdir(path);
}

bool fs_rm(std::string path)
{
    return _olrn_fs_rm(path);
}

int64_t fs_rm_all(std::string path)
{
    return _olrn_fs_rm_all(path);
}

std::vector<std::string> fs_ls(std::string path)
{
    return _olrn_fs_ls(path);
}

int64_t fs_size(std::string path)
{
    return _olrn_fs_size(path);
}

bool fs_rename(std::string from, std::string to)
{
    return _olrn_fs_rename(from, to);
}

bool fs_copy(std::string from, std::string to)
{
    return _olrn_fs_copy(from, to);
}

std::string fs_cwd()
{
    return _olrn_fs_cwd();
}

File io_open(std::string path, int32_t mode)
{
    return _olrn_io_open(path, mode);
}

void io_close(File f)
{
    _olrn_io_close(f);
}

std::vector<uint8_t> io_read(File f, int64_t n)
{
    return _olrn_io_read(f, n);
}

void io_write(File f, std::vector<uint8_t> data)
{
    _olrn_io_write(f, data);
}

void io_seek(File f, int64_t pos, int32_t from)
{
    _olrn_io_seek(f, pos, from);
}

int64_t io_tell(File f)
{
    return _olrn_io_tell(f);
}

std::string io_readline(File f)
{
    return _olrn_io_readline(f);
}

bool io_eof(File f)
{
    return _olrn_io_eof(f);
}

_OlrnResult<void> malkur_init_window(int32_t w, int32_t h, std::string title)
{
    const auto ok = _olrn_mk_init_window(w, h, title);
    if (!ok) {
        return _OlrnResult<void>(_olrn_err_MalkurError::InitFailed);
    }
    return _OlrnResult<void>();
}

void malkur_close_window()
{
    _olrn_mk_close_window();
}

bool malkur_should_close()
{
    return _olrn_mk_should_close();
}

void malkur_begin_draw()
{
    _olrn_mk_begin_draw();
}

void malkur_end_draw()
{
    _olrn_mk_end_draw();
}

void malkur_clear_bg(Color c)
{
    _olrn_mk_clear_bg(c);
}

void malkur_set_fps(int32_t target)
{
    _olrn_mk_set_fps(target);
}

void malkur_set_vsync(bool on)
{
    _olrn_mk_set_vsync(on);
}

void malkur_set_title(std::string title)
{
    _olrn_mk_set_title(title);
}

void malkur_fullscreen(bool on)
{
    _olrn_mk_fullscreen(on);
}

float malkur_dt()
{
    return _olrn_mk_dt();
}

int32_t malkur_fps()
{
    return _olrn_mk_fps();
}

int32_t malkur_width()
{
    return _olrn_mk_width();
}

int32_t malkur_height()
{
    return _olrn_mk_height();
}

bool malkur_key_down(int32_t k)
{
    return _olrn_mk_key_down(k);
}

bool malkur_key_pressed(int32_t k)
{
    return _olrn_mk_key_pressed(k);
}

bool malkur_key_released(int32_t k)
{
    return _olrn_mk_key_released(k);
}

std::string malkur_key_char()
{
    return _olrn_mk_key_char();
}

Vec2 malkur_mouse_pos()
{
    return _olrn_mk_mouse_pos();
}

Vec2 malkur_mouse_delta()
{
    return _olrn_mk_mouse_delta();
}

float malkur_mouse_scroll()
{
    return _olrn_mk_mouse_scroll();
}

bool malkur_mouse_btn(int32_t b)
{
    return _olrn_mk_mouse_btn(b);
}

bool malkur_mouse_btn_pressed(int32_t b)
{
    return _olrn_mk_mouse_btn_pressed(b);
}

bool malkur_mouse_btn_released(int32_t b)
{
    return _olrn_mk_mouse_btn_released(b);
}

void malkur_mouse_set_pos(Vec2 p)
{
    _olrn_mk_mouse_set_pos(p);
}

void malkur_mouse_hide()
{
    _olrn_mk_mouse_hide();
}

void malkur_mouse_show()
{
    _olrn_mk_mouse_show();
}

void malkur_draw_rect(float x, float y, float w, float h, Color c)
{
    _olrn_mk_draw_rect(x, y, w, h, c);
}

void malkur_draw_rect_lines(float x, float y, float w, float h, float thick, Color c)
{
    _olrn_mk_draw_rect_lines(x, y, w, h, thick, c);
}

void malkur_draw_circle(float cx, float cy, float r, Color c)
{
    _olrn_mk_draw_circle(cx, cy, r, c);
}

void malkur_draw_circle_lines(float cx, float cy, float r, Color c)
{
    _olrn_mk_draw_circle_lines(cx, cy, r, c);
}

void malkur_draw_line(float x1, float y1, float x2, float y2, float thick, Color c)
{
    _olrn_mk_draw_line(x1, y1, x2, y2, thick, c);
}

void malkur_draw_triangle(Vec2 v1, Vec2 v2, Vec2 v3, Color c)
{
    _olrn_mk_draw_tri(v1, v2, v3, c);
}

void malkur_draw_poly(Vec2 center, int32_t sides, float r, float rot, Color c)
{
    _olrn_mk_draw_poly(center, sides, r, rot, c);
}

_OlrnResult<Texture> malkur_load_texture(std::string path)
{
    const auto t = _olrn_mk_load_texture(path);
    if ((t == (int64_t)0)) {
        return _OlrnResult<Texture>(_olrn_err_MalkurError::LoadFailed);
    }
    return _OlrnResult<Texture>(t);
}

void malkur_unload_texture(Texture t)
{
    _olrn_mk_unload_texture(t);
}

void malkur_draw_texture(Texture t, float x, float y, Color tint)
{
    _olrn_mk_draw_texture(t, x, y, tint);
}

void malkur_draw_texture_ex(Texture t, Vec2 pos, float rot, float scale, Color tint)
{
    _olrn_mk_draw_texture_ex(t, pos, rot, scale, tint);
}

Color malkur_rgba(int32_t r, int32_t g, int32_t b, int32_t a)
{
    return Color{.r = _olrn_cast<uint8_t>(r), .g = _olrn_cast<uint8_t>(g), .b = _olrn_cast<uint8_t>(b), .a = _olrn_cast<uint8_t>(a)};
}

Color malkur_color_fade(Color c, float alpha)
{
    const auto a = math_clamp(_olrn_cast<double>(alpha), 0.0, 1.0);
    return Color{.r = c.r, .g = c.g, .b = c.b, .a = _olrn_cast<uint8_t>((_olrn_cast<double>(255.0) * a))};
}

Color malkur_color_lerp(Color a, Color b, float t)
{
    const auto k = math_clamp(_olrn_cast<double>(t), 0.0, 1.0);
    return Color{.r = _olrn_cast<uint8_t>((_olrn_cast<double>(a.r) + ((_olrn_cast<double>(b.r) - _olrn_cast<double>(a.r)) * k))), .g = _olrn_cast<uint8_t>((_olrn_cast<double>(a.g) + ((_olrn_cast<double>(b.g) - _olrn_cast<double>(a.g)) * k))), .b = _olrn_cast<uint8_t>((_olrn_cast<double>(a.b) + ((_olrn_cast<double>(b.b) - _olrn_cast<double>(a.b)) * k))), .a = _olrn_cast<uint8_t>((_olrn_cast<double>(a.a) + ((_olrn_cast<double>(b.a) - _olrn_cast<double>(a.a)) * k)))};
}

Vec2 malkur_vec2(float x, float y)
{
    return Vec2{.x = x, .y = y};
}

Vec2 malkur_vec2_zero()
{
    return Vec2{.x = 0.0, .y = 0.0};
}

Rect malkur_rect(float x, float y, float w, float h)
{
    return Rect{.x = x, .y = y, .w = w, .h = h};
}

Vec2 malkur_v2_add(Vec2 a, Vec2 b)
{
    return Vec2{.x = (a.x + b.x), .y = (a.y + b.y)};
}

Vec2 malkur_v2_sub(Vec2 a, Vec2 b)
{
    return Vec2{.x = (a.x - b.x), .y = (a.y - b.y)};
}

Vec2 malkur_v2_scale(Vec2 v, float s)
{
    return Vec2{.x = (v.x * s), .y = (v.y * s)};
}

float malkur_v2_dot(Vec2 a, Vec2 b)
{
    return ((a.x * b.x) + (a.y * b.y));
}

float malkur_v2_len(Vec2 v)
{
    return _olrn_cast<float>(math_sqrt(_olrn_cast<double>(((v.x * v.x) + (v.y * v.y)))));
}

float malkur_v2_dist(Vec2 a, Vec2 b)
{
    const auto dx = (a.x - b.x);
    const auto dy = (a.y - b.y);
    return _olrn_cast<float>(math_sqrt(_olrn_cast<double>(((dx * dx) + (dy * dy)))));
}

Vec2 malkur_v2_norm(Vec2 v)
{
    const auto l = malkur_v2_len(v);
    if ((l == 0.0)) {
        return Vec2{.x = 0.0, .y = 0.0};
    }
    return Vec2{.x = (v.x / l), .y = (v.y / l)};
}

Vec2 malkur_v2_lerp(Vec2 a, Vec2 b, float t)
{
    return Vec2{.x = (a.x + ((b.x - a.x) * t)), .y = (a.y + ((b.y - a.y) * t))};
}

bool malkur_check_rects(Rect a, Rect b)
{
    return ((((a.x < (b.x + b.w)) && ((a.x + a.w) > b.x)) && (a.y < (b.y + b.h))) && ((a.y + a.h) > b.y));
}

bool malkur_check_circles(Vec2 c1, float r1, Vec2 c2, float r2)
{
    return (malkur_v2_dist(c1, c2) <= (r1 + r2));
}

bool malkur_check_point_rect(Vec2 pt, Rect r)
{
    return ((((pt.x >= r.x) && (pt.x <= (r.x + r.w))) && (pt.y >= r.y)) && (pt.y <= (r.y + r.h)));
}

bool malkur_check_point_circle(Vec2 pt, Vec2 center, float r)
{
    return (malkur_v2_dist(pt, center) <= r);
}

Rect malkur_rect_intersect(Rect a, Rect b)
{
    const auto x1 = _olrn_cast<float>(math_max(_olrn_cast<double>(a.x), _olrn_cast<double>(b.x)));
    const auto y1 = _olrn_cast<float>(math_max(_olrn_cast<double>(a.y), _olrn_cast<double>(b.y)));
    const auto x2 = _olrn_cast<float>(math_min(_olrn_cast<double>((a.x + a.w)), _olrn_cast<double>((b.x + b.w))));
    const auto y2 = _olrn_cast<float>(math_min(_olrn_cast<double>((a.y + a.h)), _olrn_cast<double>((b.y + b.h))));
    if (((x2 <= x1) || (y2 <= y1))) {
        return Rect{.x = 0.0, .y = 0.0, .w = 0.0, .h = 0.0};
    }
    return Rect{.x = x1, .y = y1, .w = (x2 - x1), .h = (y2 - y1)};
}

bool malkur_pad_connected(int32_t id)
{
    return _olrn_mk_pad_connected(id);
}

bool malkur_pad_btn(int32_t id, int32_t btn)
{
    return _olrn_mk_pad_btn(id, btn);
}

bool malkur_pad_btn_pressed(int32_t id, int32_t btn)
{
    return _olrn_mk_pad_btn_pressed(id, btn);
}

float malkur_pad_axis(int32_t id, int32_t axis)
{
    return _olrn_mk_pad_axis(id, axis);
}

Camera2D malkur_camera2d(Vec2 target, Vec2 offset, float zoom)
{
    return Camera2D{.target = target, .offset = offset, .rotation = 0.0, .zoom = zoom};
}

void malkur_begin_camera2d(Camera2D cam)
{
    _olrn_mk_begin_camera2d(cam);
}

void malkur_end_camera2d()
{
    _olrn_mk_end_camera2d();
}

Vec2 malkur_screen_to_world2d(Vec2 pos, Camera2D cam)
{
    return _olrn_mk_screen_to_world2d(pos, cam);
}

Vec2 malkur_world_to_screen2d(Vec2 pos, Camera2D cam)
{
    return _olrn_mk_world_to_screen2d(pos, cam);
}

void malkur_draw_rect_rot(float x, float y, float w, float h, Vec2 origin, float rot, Color c)
{
    _olrn_mk_draw_rect_rot(x, y, w, h, origin, rot, c);
}

void malkur_draw_texture_rect(Texture t, Rect src, Rect dst, Color tint)
{
    _olrn_mk_draw_texture_rect(t, src, dst, tint);
}

void malkur_draw_text(std::string text, float x, float y, float size, Color c)
{
    _olrn_mk_draw_text(text, x, y, size, c);
}

Vec2 malkur_measure_text(std::string text, float size)
{
    return _olrn_mk_measure_text(text, size);
}

_OlrnResult<void> malkur_init_audio()
{
    const auto ok = _olrn_mk_init_audio();
    if (!ok) {
        return _OlrnResult<void>(_olrn_err_MalkurError::AudioFailed);
    }
    return _OlrnResult<void>();
}

void malkur_close_audio()
{
    _olrn_mk_close_audio();
}

_OlrnResult<Sound> malkur_load_sound(std::string path)
{
    const auto s = _olrn_mk_load_sound(path);
    if ((s == (int64_t)0)) {
        return _OlrnResult<Sound>(_olrn_err_MalkurError::LoadFailed);
    }
    return _OlrnResult<Sound>(s);
}

void malkur_unload_sound(Sound s)
{
    _olrn_mk_unload_sound(s);
}

void malkur_play_sound(Sound s)
{
    _olrn_mk_play_sound(s);
}

void malkur_stop_sounds()
{
    _olrn_mk_stop_sounds();
}

void malkur_sound_set_volume(Sound s, float vol)
{
    _olrn_mk_sound_set_volume(s, vol);
}

_OlrnResult<Music> malkur_load_music(std::string path)
{
    const auto m = _olrn_mk_load_music(path);
    if ((m == (int64_t)0)) {
        return _OlrnResult<Music>(_olrn_err_MalkurError::LoadFailed);
    }
    return _OlrnResult<Music>(m);
}

void malkur_unload_music(Music m)
{
    _olrn_mk_unload_music(m);
}

void malkur_play_music(Music m, int32_t loops)
{
    _olrn_mk_play_music(m, loops);
}

void malkur_stop_music()
{
    _olrn_mk_stop_music();
}

void malkur_pause_music()
{
    _olrn_mk_pause_music();
}

void malkur_resume_music()
{
    _olrn_mk_resume_music();
}

bool malkur_music_playing()
{
    return _olrn_mk_music_playing();
}

bool malkur_music_paused()
{
    return _olrn_mk_music_paused();
}

void malkur_music_set_volume(float vol)
{
    _olrn_mk_music_set_volume(vol);
}

Color malkur_hex(uint32_t val)
{
    const auto r = _olrn_cast<uint8_t>(((val >> (int64_t)24) & (int64_t)255));
    const auto g = _olrn_cast<uint8_t>(((val >> (int64_t)16) & (int64_t)255));
    const auto b = _olrn_cast<uint8_t>(((val >> (int64_t)8) & (int64_t)255));
    const auto a = _olrn_cast<uint8_t>((val & (int64_t)255));
    return Color{.r = r, .g = g, .b = b, .a = a};
}

static _OlrnResult<void> _olrn_main()
{
    { auto _r_0 = (malkur_init_window((int64_t)320, (int64_t)240, "audio test")); if (!_r_0.is_ok()) return _OlrnResult<void>(_r_0.error()); }
    _OlrnDeferGuard _defer_0([&]() {
        malkur_close_window();
    });
    { auto _r_1 = (malkur_init_audio()); if (!_r_1.is_ok()) return _OlrnResult<void>(_r_1.error()); }
    _OlrnDeferGuard _defer_1([&]() {
        malkur_close_audio();
    });
    { auto _r_2 = (malkur_init_audio()); if (!_r_2.is_ok()) return _OlrnResult<void>(_r_2.error()); }
    const auto playing = malkur_music_playing();
    const auto paused = malkur_music_paused();
    if (playing) {
                { std::cerr << "panic: " << "music_playing should be false at start" << std::endl; std::abort(); }
    }
    if (paused) {
                { std::cerr << "panic: " << "music_paused should be false at start" << std::endl; std::abort(); }
    }
    malkur_music_set_volume(0.5);
    malkur_stop_sounds();
    malkur_stop_music();
    malkur_pause_music();
    malkur_resume_music();
    const auto bad_tex = (__extension__({auto _r_3=(malkur_load_texture("nonexistent.png"));if(!_r_3.is_ok()){_OlrnError _=_r_3.error();/* @i64 */ ((int64_t)0);
}_r_3.value();}));
    if ((bad_tex != (int64_t)0)) {
                { std::cerr << "panic: " << "expected 0 for missing texture" << std::endl; std::abort(); }
    }
    const auto bad_snd = (__extension__({auto _r_4=(malkur_load_sound("nonexistent.wav"));if(!_r_4.is_ok()){_OlrnError _=_r_4.error();/* @i64 */ ((int64_t)0);
}_r_4.value();}));
    if ((bad_snd != (int64_t)0)) {
                { std::cerr << "panic: " << "expected 0 for missing sound" << std::endl; std::abort(); }
    }
    const auto bad_mus = (__extension__({auto _r_5=(malkur_load_music("nonexistent.ogg"));if(!_r_5.is_ok()){_OlrnError _=_r_5.error();/* @i64 */ ((int64_t)0);
}_r_5.value();}));
    if ((bad_mus != (int64_t)0)) {
                { std::cerr << "panic: " << "expected 0 for missing music" << std::endl; std::abort(); }
    }
    std::cout << "malkur audio+image test ok" << std::endl;
    return _OlrnResult<void>();
}

int main()
{
    auto _r = _olrn_main();
    if (!_r.is_ok()) {
        std::cerr << "error: " << _r.error().msg << std::endl;
        return 1;
    }
    return 0;
}

