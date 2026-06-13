#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "ast/ast.h"
#include "sema/check.h"
#include "deps/deps.h"
#include "codegen/codegen.h"

#ifdef _WIN32
#include <direct.h>
#define olrn_mkdir(p) _mkdir(p)
#else
#define olrn_mkdir(p) mkdir((p), 0755)
#endif

/* ── file helpers ─────────────────────────────────────────────── */

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return NULL; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

static char *resolve_path(const char *base_path, const char *rel)
{
    if (rel[0] == '/') return strdup(rel);
    const char *last_slash = strrchr(base_path, '/');
    if (!last_slash) return strdup(rel);
    size_t dir_len = (size_t)(last_slash - base_path) + 1;
    char *out = malloc(dir_len + strlen(rel) + 1);
    memcpy(out, base_path, dir_len);
    strcpy(out + dir_len, rel);
    return out;
}

static AstNode *parse_file(const char *path, char **src_out)
{
    char *src = read_file(path);
    if (!src) return NULL;
    *src_out = src;
    Lexer lex; lexer_init(&lex, src);
    Parser p;  parser_init(&p, &lex);
    AstNode *prog = parser_parse_program(&p);
    if (p.had_error) {
        fprintf(stderr, "error in file: %s\n", path);
        ast_free(prog); free(src); *src_out = NULL;
        return NULL;
    }
    return prog;
}

/* ── stdlib loading ───────────────────────────────────────────── */

static char *find_stdlib(void)
{
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n > 0) {
        exe[n] = '\0';
        char *slash = strrchr(exe, '/');
        if (slash) {
            *slash = '\0';
            char *path = malloc(strlen(exe) + 16);
            sprintf(path, "%s/stdlib", exe);
            if (access(path, F_OK) == 0) return path;
            free(path);
        }
    }
    if (access("stdlib", F_OK) == 0) return strdup("stdlib");
    return NULL;
}

static const char *STD_MODULES[] = {
    "io", "fs", "time", "math", "mem", "str", "log", "thread", NULL
};

/* true if any import binds the malkur module (mk = @std.malkur) */
static int imports_use_malkur(AstNode *program)
{
    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (imp->import_decl.is_lib && imp->import_decl.module &&
            strcmp(imp->import_decl.module, "malkur") == 0)
            return 1;
    }
    return 0;
}

static int imports_use_pelentar(AstNode *program)
{
    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (imp->import_decl.is_lib && imp->import_decl.module &&
            strcmp(imp->import_decl.module, "pelentar") == 0)
            return 1;
    }
    return 0;
}

static void load_stdlib_module(AstNode *prog, const char *stdlib_path,
                               const char *lib, const char *mod,
                               char **srcs, int *count)
{
    char path[4096];
    snprintf(path, sizeof(path), "%s/%s/%s.olrn", stdlib_path, lib, mod);
    char *src = NULL;
    AstNode *imported = parse_file(path, &src);
    if (!imported) return;
    NodeList tmp = prog->program.decls;
    memset(&prog->program.decls, 0, sizeof(NodeList));
    for (int j = 0; j < imported->program.decls.count; j++)
        node_list_push(&prog->program.decls, imported->program.decls.items[j]);
    for (int j = 0; j < tmp.count; j++)
        node_list_push(&prog->program.decls, tmp.items[j]);
    free(tmp.items);
    imported->program.decls.count = 0;
    ast_free(imported);
    srcs[(*count)++] = src;
}

static int merge_imports(AstNode *program, const char *host_path,
                         char **extra_srcs, int *extra_count)
{
    /* any @std entry (whole-lib import or module bind) loads the stdlib once.
       malkur only loads when explicitly bound (mk = @std.malkur), since it
       drags in SDL2; it loads first so it lands after std.math in decl order
       (no fn prototypes are emitted — callees must precede callers). */
    int wants_std = 0;
    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (imp->import_decl.is_lib &&
            strcmp(imp->import_decl.source, "std") == 0)
            wants_std = 1;
    }
    if (wants_std) {
        char *stdlib_path = find_stdlib();
        if (!stdlib_path) {
            fprintf(stderr, "warning: stdlib not found — @std unavailable\n");
        } else {
            if (imports_use_malkur(program))
                load_stdlib_module(program, stdlib_path, "std",
                                   "malkur", extra_srcs, extra_count);
            if (imports_use_pelentar(program))
                load_stdlib_module(program, stdlib_path, "std",
                                   "pelentar", extra_srcs, extra_count);
            for (int m = 0; STD_MODULES[m]; m++)
                load_stdlib_module(program, stdlib_path, "std",
                                   STD_MODULES[m], extra_srcs, extra_count);
            free(stdlib_path);
        }
    }
    for (int i = 0; i < program->program.imports.count; i++) {
        AstNode *imp = program->program.imports.items[i];
        if (imp->import_decl.is_lib) continue;
        char *resolved = resolve_path(host_path, imp->import_decl.source);
        char *src = NULL;
        AstNode *imported = parse_file(resolved, &src);
        free(resolved);
        if (!imported) return 0;
        NodeList tmp = program->program.decls;
        memset(&program->program.decls, 0, sizeof(NodeList));
        for (int j = 0; j < imported->program.decls.count; j++)
            node_list_push(&program->program.decls, imported->program.decls.items[j]);
        for (int j = 0; j < tmp.count; j++)
            node_list_push(&program->program.decls, tmp.items[j]);
        free(tmp.items);
        imported->program.decls.count = 0;
        ast_free(imported);
        extra_srcs[(*extra_count)++] = src;
    }
    return 1;
}

/* ── build progress tree ──────────────────────────────────────── */

/* Zig-style step tree, printed by 'olrn build' / 'olrn run' as each
   pipeline stage completes (sac/emit stay quiet). */
static int g_tree = 0;

static void tree_step(int last, const char *label, const char *detail)
{
    if (!g_tree) return;
    printf("%s %-8s %s\n", last ? "└─" : "├─", label, detail);
    fflush(stdout);
}

/* ── core compilation pipeline ────────────────────────────────── */

/* compile a generated .cpp to a binary; system-library deps (e.g.
   SDL2 for malkur) are detected from the file and resolved via
   pkg-config or platform fallbacks. Returns 0 on success. */
static int gxx_compile(const char *cpp_path, const char *output)
{
    char dep_flags[768];
    if (deps_flags_for_cpp(cpp_path, dep_flags, sizeof(dep_flags))) {
        fprintf(stderr, "build aborted — run 'olrn deps' for a full report\n");
        return 1;
    }

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "g++ -std=c++17 -O2 \"%s\" -o \"%s\"%s 2>&1", cpp_path, output,
             dep_flags);
    return (system(cmd) == 0) ? 0 : 1;
}

/* Parse olrn_path, merge imports, emit C++ to out. Returns 0 on success. */
static int compile_to_out(const char *olrn_path, FILE *out)
{
    char *src = NULL;
    AstNode *program = parse_file(olrn_path, &src);
    if (!program) return 1;
    tree_step(0, "parse", olrn_path);

    char *extra_srcs[64]; int extra_count = 0;
    if (!merge_imports(program, olrn_path, extra_srcs, &extra_count)) {
        ast_free(program); free(src); return 1;
    }
    if (g_tree) {
        char d[32];
        snprintf(d, sizeof(d), "%d module%s", extra_count,
                 extra_count == 1 ? "" : "s");
        tree_step(0, "imports", extra_count ? d : "none");
    }

    if (check_program(program)) {
        ast_free(program); free(src);
        for (int i = 0; i < extra_count; i++) free(extra_srcs[i]);
        return 1;
    }
    tree_step(0, "sema", "ok");

    Codegen cg;
    codegen_init(&cg, out);
    codegen_emit(&cg, program);
    fflush(out);

    ast_free(program); free(src);
    for (int i = 0; i < extra_count; i++) free(extra_srcs[i]);
    return 0;
}

/* Compile one or more .olrn files to a native binary via g++.
   inputs[0] is the main file; inputs[1..n-1] are prepended as implicit imports.
   Returns 0 on success. */
static int compile_to_binary(const char **inputs, int input_count,
                             const char *output)
{
    /* write merged C++ to a temp file */
    char tmp_cpp[64];
    snprintf(tmp_cpp, sizeof(tmp_cpp), "/tmp/olrn_%d.cpp", (int)getpid());
    FILE *cpp_file = fopen(tmp_cpp, "w");
    if (!cpp_file) { perror(tmp_cpp); return 1; }

    char *main_src = NULL;
    AstNode *program = parse_file(inputs[0], &main_src);
    if (!program) { fclose(cpp_file); remove(tmp_cpp); return 1; }
    tree_step(0, "parse", inputs[0]);

    char *extra_srcs[128]; int extra_count = 0;
    if (!merge_imports(program, inputs[0], extra_srcs, &extra_count)) {
        ast_free(program); free(main_src);
        fclose(cpp_file); remove(tmp_cpp); return 1;
    }
    if (g_tree) {
        char d[32];
        snprintf(d, sizeof(d), "%d module%s", extra_count,
                 extra_count == 1 ? "" : "s");
        tree_step(0, "imports", extra_count ? d : "none");
    }

    /* prepend any additional .olrn files as implicit imports */
    for (int i = 1; i < input_count; i++) {
        char *fsrc = NULL;
        AstNode *imported = parse_file(inputs[i], &fsrc);
        if (!imported) {
            ast_free(program); free(main_src);
            for (int j = 0; j < extra_count; j++) free(extra_srcs[j]);
            fclose(cpp_file); remove(tmp_cpp); return 1;
        }
        NodeList tmp = program->program.decls;
        memset(&program->program.decls, 0, sizeof(NodeList));
        for (int j = 0; j < imported->program.decls.count; j++)
            node_list_push(&program->program.decls, imported->program.decls.items[j]);
        for (int j = 0; j < tmp.count; j++)
            node_list_push(&program->program.decls, tmp.items[j]);
        free(tmp.items);
        imported->program.decls.count = 0;
        ast_free(imported);
        extra_srcs[extra_count++] = fsrc;
    }

    if (check_program(program)) {
        ast_free(program); free(main_src);
        for (int i = 0; i < extra_count; i++) free(extra_srcs[i]);
        fclose(cpp_file); remove(tmp_cpp); return 1;
    }
    tree_step(0, "sema", "ok");

    Codegen cg;
    codegen_init(&cg, cpp_file);
    codegen_emit(&cg, program);
    fflush(cpp_file);
    fclose(cpp_file);
    tree_step(0, "codegen", tmp_cpp);

    ast_free(program); free(main_src);
    for (int i = 0; i < extra_count; i++) free(extra_srcs[i]);

    tree_step(1, "g++", output);
    int rc = gxx_compile(tmp_cpp, output);
    remove(tmp_cpp);
    return rc;
}

/* ── olrn view ────────────────────────────────────────────────── */

static void write_viewer_cpp(FILE *fp, const char *media_path)
{
    /* escape the path for use inside a C string literal */
    char esc[4096]; int ei = 0;
    for (const char *p = media_path; *p && ei < (int)sizeof(esc)-3; p++) {
        if (*p == '"' || *p == '\\') esc[ei++] = '\\';
        esc[ei++] = *p;
    }
    esc[ei] = '\0';

    fprintf(fp,
"// generated by olrn view\n"
"#include <SDL2/SDL.h>\n"
"#include <SDL2/SDL_image.h>\n"
"#include <cstdio>\n"
"#include <cstring>\n"
"#include <cstdlib>\n"
"#include <cmath>\n"
"#include <vector>\n"
"#include <string>\n"
"#include <algorithm>\n"
"\n"
"static const char *MEDIA_PATH = \"%s\";\n"
"\n"
"// ── enhance ────────────────────────────────────────────────────\n"
"static void box_blur(uint8_t *dst, const uint8_t *src, int w, int h) {\n"
"    for (int y=0;y<h;y++) for (int x=0;x<w;x++) {\n"
"        int r=0,g=0,b=0,n=0;\n"
"        for (int dy=-1;dy<=1;dy++) for (int dx=-1;dx<=1;dx++) {\n"
"            int nx=x+dx, ny=y+dy;\n"
"            if (nx<0||nx>=w||ny<0||ny>=h) continue;\n"
"            const uint8_t *q=src+(ny*w+nx)*4;\n"
"            r+=q[0]; g+=q[1]; b+=q[2]; n++;\n"
"        }\n"
"        uint8_t *d=dst+(y*w+x)*4;\n"
"        d[0]=(uint8_t)(r/n); d[1]=(uint8_t)(g/n); d[2]=(uint8_t)(b/n);\n"
"        d[3]=src[(y*w+x)*4+3];\n"
"    }\n"
"}\n"
"\n"
"static uint8_t scurve(uint8_t v) {\n"
"    float f = v / 255.f;\n"
"    if (f < .5f) f = 4.f*f*f*f*.5f + f*.10f;\n"
"    else         f = 1.f - (4.f*(1.f-f)*(1.f-f)*(1.f-f)*.5f + (1.f-f)*.10f);\n"
"    f = f<0.f?0.f:(f>1.f?1.f:f);\n"
"    return (uint8_t)(f*255.f);\n"
"}\n"
"\n"
"static void apply_enhance(uint8_t *pix, int w, int h) {\n"
"    std::vector<uint8_t> blur((size_t)w*h*4);\n"
"    box_blur(blur.data(), pix, w, h);\n"
"    for (int i=0;i<w*h;i++) {\n"
"        for (int c=0;c<3;c++) {\n"
"            int s = (int)pix[i*4+c] + (int)(((int)pix[i*4+c]-(int)blur[i*4+c])*1.5f);\n"
"            s = s<0?0:(s>255?255:s);\n"
"            pix[i*4+c] = scurve((uint8_t)s);\n"
"        }\n"
"    }\n"
"}\n"
"\n"
"// ── render ──────────────────────────────────────────────────────\n"
"static void render_tex(SDL_Renderer *ren, SDL_Texture *tex,\n"
"                        int tw, int th, float zoom, float px, float py) {\n"
"    int ww,wh; SDL_GetRendererOutputSize(ren,&ww,&wh);\n"
"    float dw=tw*zoom, dh=th*zoom;\n"
"    SDL_FRect dst{ww*.5f-dw*.5f+px, wh*.5f-dh*.5f+py, dw, dh};\n"
"    SDL_SetRenderDrawColor(ren,20,20,20,255);\n"
"    SDL_RenderClear(ren);\n"
"    SDL_RenderCopyF(ren,tex,NULL,&dst);\n"
"    SDL_RenderPresent(ren);\n"
"}\n"
"\n"
"static void update_title(SDL_Window *win, float zoom, bool enhance, bool paused) {\n"
"    char buf[768];\n"
"    snprintf(buf,sizeof(buf),\"olrn view — %%s  [%%.0f%%%%]%%-7s%%-10s\",\n"
"        MEDIA_PATH, zoom*100.f,\n"
"        enhance ? \"  [E on]\" : \"\",\n"
"        paused  ? \"  [paused]\" : \"\");\n"
"    SDL_SetWindowTitle(win,buf);\n"
"}\n"
"\n"
"// ── image / gif viewer ──────────────────────────────────────────\n"
"static int view_image(SDL_Window *win, SDL_Renderer *ren, bool is_gif) {\n"
"    IMG_Animation *anim = NULL;\n"
"    SDL_Surface   *img  = NULL;\n"
"    if (is_gif) anim = IMG_LoadAnimation(MEDIA_PATH);\n"
"    if (!anim)  img  = IMG_Load(MEDIA_PATH);\n"
"    if (!anim && !img) {\n"
"        fprintf(stderr,\"view: %%s\\n\",IMG_GetError()); return 1;\n"
"    }\n"
"    int tw = anim ? anim->w : img->w;\n"
"    int th = anim ? anim->h : img->h;\n"
"    int nframes = anim ? anim->count : 1;\n"
"\n"
"    std::vector<std::vector<uint8_t>> orig(nframes), cur(nframes);\n"
"    for (int i=0;i<nframes;i++) {\n"
"        SDL_Surface *s = anim ? anim->frames[i] : img;\n"
"        SDL_Surface *r = SDL_ConvertSurfaceFormat(s,SDL_PIXELFORMAT_RGBA32,0);\n"
"        orig[i].resize((size_t)tw*th*4);\n"
"        SDL_LockSurface(r);\n"
"        memcpy(orig[i].data(),r->pixels,(size_t)tw*th*4);\n"
"        SDL_UnlockSurface(r);\n"
"        cur[i] = orig[i];\n"
"        SDL_FreeSurface(r);\n"
"    }\n"
"    if (img) SDL_FreeSurface(img);\n"
"\n"
"    SDL_Texture *tex = SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGBA32,\n"
"                                          SDL_TEXTUREACCESS_STREAMING,tw,th);\n"
"    SDL_SetTextureBlendMode(tex,SDL_BLENDMODE_BLEND);\n"
"\n"
"    float zoom=1.f, px=0.f, py=0.f;\n"
"    bool enhance=false, dragging=false, running=true;\n"
"    int drag_sx=0, drag_sy=0;\n"
"    float drag_px0=0.f, drag_py0=0.f;\n"
"    int frame=0;\n"
"    Uint32 last_frame_t=SDL_GetTicks();\n"
"    update_title(win,zoom,enhance,false);\n"
"\n"
"    while (running) {\n"
"        SDL_Event ev;\n"
"        while (SDL_PollEvent(&ev)) {\n"
"            if (ev.type==SDL_QUIT) running=false;\n"
"            else if (ev.type==SDL_KEYDOWN) {\n"
"                switch (ev.key.keysym.sym) {\n"
"                case SDLK_ESCAPE: case SDLK_q: running=false; break;\n"
"                case SDLK_e:\n"
"                    enhance=!enhance;\n"
"                    for (int i=0;i<nframes;i++) {\n"
"                        cur[i]=orig[i];\n"
"                        if (enhance) apply_enhance(cur[i].data(),tw,th);\n"
"                    }\n"
"                    update_title(win,zoom,enhance,false);\n"
"                    break;\n"
"                case SDLK_EQUALS: case SDLK_PLUS:\n"
"                    zoom*=1.1f; update_title(win,zoom,enhance,false); break;\n"
"                case SDLK_MINUS:\n"
"                    zoom/=1.1f; update_title(win,zoom,enhance,false); break;\n"
"                case SDLK_0:\n"
"                    zoom=1.f; px=0.f; py=0.f;\n"
"                    update_title(win,zoom,enhance,false); break;\n"
"                default: break;\n"
"                }\n"
"            }\n"
"            else if (ev.type==SDL_MOUSEWHEEL) {\n"
"                zoom *= ev.wheel.y>0 ? 1.1f : 1.f/1.1f;\n"
"                update_title(win,zoom,enhance,false);\n"
"            }\n"
"            else if (ev.type==SDL_MOUSEBUTTONDOWN&&ev.button.button==SDL_BUTTON_LEFT) {\n"
"                dragging=true; drag_sx=ev.button.x; drag_sy=ev.button.y;\n"
"                drag_px0=px; drag_py0=py;\n"
"            }\n"
"            else if (ev.type==SDL_MOUSEBUTTONUP&&ev.button.button==SDL_BUTTON_LEFT)\n"
"                dragging=false;\n"
"            else if (ev.type==SDL_MOUSEMOTION&&dragging) {\n"
"                px=drag_px0+(ev.motion.x-drag_sx);\n"
"                py=drag_py0+(ev.motion.y-drag_sy);\n"
"            }\n"
"        }\n"
"        if (nframes>1) {\n"
"            int delay = anim ? anim->delays[frame] : 100;\n"
"            if (delay<10) delay=100;\n"
"            if ((int)(SDL_GetTicks()-last_frame_t) >= delay) {\n"
"                frame=(frame+1)%%nframes;\n"
"                last_frame_t=SDL_GetTicks();\n"
"            }\n"
"        }\n"
"        SDL_UpdateTexture(tex,NULL,cur[frame].data(),tw*4);\n"
"        render_tex(ren,tex,tw,th,zoom,px,py);\n"
"        SDL_Delay(16);\n"
"    }\n"
"    SDL_DestroyTexture(tex);\n"
"    if (anim) IMG_FreeAnimation(anim);\n"
"    return 0;\n"
"}\n"
"\n"
"// ── video viewer ────────────────────────────────────────────────\n"
"static int view_video(SDL_Window *win, SDL_Renderer *ren) {\n"
"    char probe[2048];\n"
"    snprintf(probe,sizeof(probe),\n"
"        \"ffprobe -v quiet -select_streams v:0 \"\n"
"        \"-show_entries stream=width,height,r_frame_rate \"\n"
"        \"-of default=noprint_wrappers=1 \\\"%%s\\\" 2>/dev/null\", MEDIA_PATH);\n"
"    FILE *pf=popen(probe,\"r\");\n"
"    if (!pf) { fprintf(stderr,\"view: ffprobe not found — install ffmpeg\\n\"); return 1; }\n"
"    int vw=0,vh=0; double fps=30.0;\n"
"    char line[256];\n"
"    while (fgets(line,sizeof(line),pf)) {\n"
"        if (!strncmp(line,\"width=\",6))  vw=atoi(line+6);\n"
"        else if (!strncmp(line,\"height=\",7)) vh=atoi(line+7);\n"
"        else if (!strncmp(line,\"r_frame_rate=\",13)) {\n"
"            int n=30,d=1; sscanf(line+13,\"%%d/%%d\",&n,&d);\n"
"            fps=d>0?(double)n/d:30.0;\n"
"        }\n"
"    }\n"
"    pclose(pf);\n"
"    if (!vw||!vh) { fprintf(stderr,\"view: could not probe video dimensions\\n\"); return 1; }\n"
"\n"
"    char fcmd[2048];\n"
"    snprintf(fcmd,sizeof(fcmd),\n"
"        \"ffmpeg -i \\\"%%s\\\" -f rawvideo -pix_fmt rgba pipe:1 2>/dev/null\",\n"
"        MEDIA_PATH);\n"
"    FILE *fp=popen(fcmd,\"r\");\n"
"    if (!fp) { fprintf(stderr,\"view: ffmpeg not found — install ffmpeg\\n\"); return 1; }\n"
"\n"
"    SDL_Texture *tex=SDL_CreateTexture(ren,SDL_PIXELFORMAT_RGBA32,\n"
"                                        SDL_TEXTUREACCESS_STREAMING,vw,vh);\n"
"    std::vector<uint8_t> buf((size_t)vw*vh*4);\n"
"    float zoom=1.f,px=0.f,py=0.f;\n"
"    bool dragging=false,running=true,paused=false;\n"
"    int drag_sx=0,drag_sy=0;\n"
"    float drag_px0=0.f,drag_py0=0.f;\n"
"    Uint32 frame_ms=(Uint32)(1000.0/fps);\n"
"    Uint32 last_t=SDL_GetTicks();\n"
"    update_title(win,zoom,false,false);\n"
"\n"
"    while (running) {\n"
"        SDL_Event ev;\n"
"        while (SDL_PollEvent(&ev)) {\n"
"            if (ev.type==SDL_QUIT) running=false;\n"
"            else if (ev.type==SDL_KEYDOWN) {\n"
"                switch (ev.key.keysym.sym) {\n"
"                case SDLK_ESCAPE: case SDLK_q: running=false; break;\n"
"                case SDLK_SPACE:\n"
"                    paused=!paused; update_title(win,zoom,false,paused); break;\n"
"                case SDLK_EQUALS: case SDLK_PLUS:\n"
"                    zoom*=1.1f; update_title(win,zoom,false,paused); break;\n"
"                case SDLK_MINUS:\n"
"                    zoom/=1.1f; update_title(win,zoom,false,paused); break;\n"
"                case SDLK_0:\n"
"                    zoom=1.f; px=0.f; py=0.f;\n"
"                    update_title(win,zoom,false,paused); break;\n"
"                default: break;\n"
"                }\n"
"            }\n"
"            else if (ev.type==SDL_MOUSEWHEEL) {\n"
"                zoom *= ev.wheel.y>0 ? 1.1f : 1.f/1.1f;\n"
"                update_title(win,zoom,false,paused);\n"
"            }\n"
"            else if (ev.type==SDL_MOUSEBUTTONDOWN&&ev.button.button==SDL_BUTTON_LEFT) {\n"
"                dragging=true; drag_sx=ev.button.x; drag_sy=ev.button.y;\n"
"                drag_px0=px; drag_py0=py;\n"
"            }\n"
"            else if (ev.type==SDL_MOUSEBUTTONUP&&ev.button.button==SDL_BUTTON_LEFT)\n"
"                dragging=false;\n"
"            else if (ev.type==SDL_MOUSEMOTION&&dragging) {\n"
"                px=drag_px0+(ev.motion.x-drag_sx);\n"
"                py=drag_py0+(ev.motion.y-drag_sy);\n"
"            }\n"
"        }\n"
"        if (!paused) {\n"
"            Uint32 now=SDL_GetTicks();\n"
"            if (now-last_t>=frame_ms) {\n"
"                size_t want=(size_t)vw*vh*4;\n"
"                if (fread(buf.data(),1,want,fp)!=want) running=false;\n"
"                else SDL_UpdateTexture(tex,NULL,buf.data(),vw*4);\n"
"                last_t=now;\n"
"            }\n"
"        }\n"
"        render_tex(ren,tex,vw,vh,zoom,px,py);\n"
"        SDL_Delay(8);\n"
"    }\n"
"    pclose(fp);\n"
"    SDL_DestroyTexture(tex);\n"
"    return 0;\n"
"}\n"
"\n"
"int main() {\n"
"    if (SDL_Init(SDL_INIT_VIDEO) < 0) {\n"
"        fprintf(stderr,\"SDL_Init: %%s\\n\",SDL_GetError()); return 1;\n"
"    }\n"
"    IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG|IMG_INIT_WEBP);\n"
"    SDL_Window *win = SDL_CreateWindow(\"olrn view\",\n"
"        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,\n"
"        1280,720, SDL_WINDOW_RESIZABLE);\n"
"    if (!win) {\n"
"        fprintf(stderr,\"SDL_CreateWindow: %%s\\n\",SDL_GetError()); return 1;\n"
"    }\n"
"    SDL_Renderer *ren = SDL_CreateRenderer(win,-1,\n"
"        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);\n"
"    if (!ren) {\n"
"        fprintf(stderr,\"SDL_CreateRenderer: %%s\\n\",SDL_GetError()); return 1;\n"
"    }\n"
"\n"
"    std::string path=MEDIA_PATH;\n"
"    size_t dot=path.rfind('.');\n"
"    std::string ext = dot!=std::string::npos ? path.substr(dot+1) : \"\";\n"
"    for (auto &c:ext) c=(char)std::tolower((unsigned char)c);\n"
"\n"
"    int rc;\n"
"    if (ext==\"mp4\"||ext==\"mkv\"||ext==\"avi\"||ext==\"mov\"||\n"
"        ext==\"webm\"||ext==\"flv\"||ext==\"wmv\")\n"
"        rc = view_video(win,ren);\n"
"    else\n"
"        rc = view_image(win,ren, ext==\"gif\");\n"
"\n"
"    SDL_DestroyRenderer(ren);\n"
"    SDL_DestroyWindow(win);\n"
"    IMG_Quit();\n"
"    SDL_Quit();\n"
"    return rc;\n"
"}\n",
    esc);
}

static int cmd_view(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: olrn view <image|gif|video>\n");
        return 1;
    }
    const char *media_path = argv[2];

    char tmp_cpp[64], tmp_bin[64];
    snprintf(tmp_cpp, sizeof(tmp_cpp), "/tmp/olrn_view_%d.cpp", (int)getpid());
    snprintf(tmp_bin, sizeof(tmp_bin), "/tmp/olrn_view_%d",     (int)getpid());

    FILE *fp = fopen(tmp_cpp, "w");
    if (!fp) { perror(tmp_cpp); return 1; }
    write_viewer_cpp(fp, media_path);
    fclose(fp);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "g++ -std=c++17 -O2 \"%s\" -o \"%s\" "
        "$(sdl2-config --cflags --libs) -lSDL2_image 2>&1",
        tmp_cpp, tmp_bin);
    int rc = system(cmd);
    remove(tmp_cpp);
    if (rc != 0) {
        fprintf(stderr, "view: compile failed — is SDL2 + SDL2_image installed?\n");
        fprintf(stderr, "  Arch/CachyOS:  sudo pacman -S sdl2 sdl2_image\n");
        fprintf(stderr, "  Debian/Ubuntu: sudo apt install libsdl2-dev libsdl2-image-dev\n");
        remove(tmp_bin);
        return 1;
    }

    snprintf(cmd, sizeof(cmd), "\"%s\"", tmp_bin);
    rc = system(cmd);
    remove(tmp_bin);
    return rc;
}

/* ── commands ─────────────────────────────────────────────────── */

static void print_help(void)
{
    printf("olrn %s — the Oleren compiler\n\n", OLRN_VERSION);
    printf("usage:\n");
    printf("  olrn <file.olrn>               emit C++ to stdout\n");
    printf("  olrn emit <file.olrn>          emit C++ to stdout\n");
    printf("  olrn build-src <file.olrn>     emit C++ to olrn_out/<file>.cpp (project) or ./<file>.cpp\n");
    printf("  olrn build-out <file.cpp>      compile C++ to bin/<name> (project) or ./<name>\n");
    printf("  olrn check <file.olrn>         parse and check for errors\n");
    printf("  olrn sac <file(s)> [-o=name]   compile to native binary\n");
    printf("  olrn build                     build project (main.olrn → binary)\n");
    printf("  olrn run                       build and run project\n");
    printf("  olrn init                      scaffold a new project in cwd\n");
    printf("  olrn deps [file.olrn]          check system libs (SDL2, ...) + install hints\n");
    printf("  olrn view <file>               open image, GIF, or video in SDL2 viewer\n");
    printf("  olrn version                   print version\n");
    printf("  olrn help                      print this help\n");
}

/* olrn init — scaffold in the current directory */
/* current directory name — used as the project/binary name */
static const char *project_name(void)
{
    static char cwd[512];
    if (!getcwd(cwd, sizeof(cwd))) { perror("getcwd"); return NULL; }
    const char *name = strrchr(cwd, '/');
    return name ? name + 1 : cwd;
}

#define PROJECT_MAIN "src/main/olrn/main.olrn"

/* write a file only if it doesn't exist yet; returns 0 on success */
static int write_if_missing(const char *path, const char *fmt,
                            const char *arg)
{
    if (access(path, F_OK) == 0) return 0;
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); return 1; }
    fprintf(f, fmt, arg);
    fclose(f);
    printf("  %s\n", path);
    return 0;
}

static int cmd_init(int argc, char **argv)
{
    (void)argc; (void)argv;

    const char *name = project_name();
    if (!name) return 1;

    olrn_mkdir("bin");
    olrn_mkdir("src");
    olrn_mkdir("src/main");
    olrn_mkdir("src/main/olrn");
    olrn_mkdir("olrn_out");

    printf("initialized project: %s\n", name);
    if (write_if_missing(PROJECT_MAIN,
        "fn main() -> void\n"
        "{\n"
        "    @pl(\"Hello from %s!\")\n"
        "}\n", name)) return 1;
    if (write_if_missing("olrn_pkg.toml",
        "[project]\n"
        "name = \"%s\"\n"
        "version = \"0.2.0\"\n"
        "\n"
        "# external dependencies land here once the package manager exists:\n"
        "# [deps]\n", name)) return 1;
    if (write_if_missing("README.md",
        "# %s\n"
        "\n"
        "An Oleren project.\n"
        "\n"
        "```sh\n"
        "olrn build   # compile src/main/olrn/main.olrn -> bin/\n"
        "olrn run     # build then execute\n"
        "```\n", name)) return 1;
    return 0;
}

/* olrn deps [file.olrn] — report system-library deps + install hints.
   With no file, uses the project entry point; with no project, checks
   every dep the stdlib knows about (preflight mode). */
static int cmd_deps(int argc, char **argv)
{
    const char *entry = NULL;
    if (argc >= 3)                              entry = argv[2];
    else if (access(PROJECT_MAIN, F_OK) == 0)   entry = PROJECT_MAIN;
    else if (access("main.olrn", F_OK) == 0)    entry = "main.olrn";

    const SysDep *needed[16];
    int n = 0;
    if (entry) {
        char *src = NULL;
        AstNode *program = parse_file(entry, &src);
        if (!program) return 1;
        for (int i = 0; i < program->program.imports.count; i++) {
            AstNode *imp = program->program.imports.items[i];
            if (!imp->import_decl.is_lib || !imp->import_decl.module)
                continue;
            const SysDep *batch[16];
            int got = deps_for_module(imp->import_decl.module, batch, 16);
            for (int j = 0; j < got && n < 16; j++) needed[n++] = batch[j];
        }
        ast_free(program); free(src);
        printf("system deps for %s\n", entry);
        if (n == 0) { printf("└─ none required\n"); return 0; }
    } else {
        printf("system deps (no project here — checking all known)\n");
        for (int i = 0; SYS_DEPS[i].module && n < 16; i++)
            needed[n++] = &SYS_DEPS[i];
    }

    int missing = 0;
    for (int i = 0; i < n; i++) {
        const SysDep *d = needed[i];
        const char *branch = (i == n - 1) ? "└─" : "├─";
        if (dep_available(d)) {
            char ver[64];
            dep_version(d, ver, sizeof(ver));
            printf("%s %s (@std.%s)  found%s%s\n",
                   branch, d->lib, d->module, ver[0] ? " " : "", ver);
        } else {
            printf("%s %s (@std.%s)  MISSING\n", branch, d->lib, d->module);
            dep_print_hint(d);
            missing = 1;
        }
    }
    return missing;
}

/* olrn check <file.olrn> */
static int cmd_check(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: olrn check <file.olrn>\n");
        return 1;
    }
    const char *path = argv[2];
    char *src = NULL;
    AstNode *program = parse_file(path, &src);
    if (!program) return 1;

    /* also resolve imports to surface import-level errors */
    char *extra_srcs[64]; int extra_count = 0;
    int ok = merge_imports(program, path, extra_srcs, &extra_count);
    if (ok && check_program(program)) ok = 0;

    ast_free(program); free(src);
    for (int i = 0; i < extra_count; i++) free(extra_srcs[i]);

    if (!ok) return 1;
    printf("%s: ok\n", path);
    return 0;
}

/* olrn emit <file.olrn>  — C++ to stdout */
static int cmd_emit(int argc, char **argv)
{
    const char *path = (argc >= 3) ? argv[2] : argv[1];
    return compile_to_out(path, stdout);
}

/* olrn build-src <file.olrn>  — C++ to olrn_out/<basename>.cpp (project)
   or <basename>.cpp (flat) */
static int cmd_build_src(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: olrn build-src <file.olrn>\n");
        return 1;
    }
    const char *in_path = argv[2];

    const char *base = strrchr(in_path, '/');
    base = base ? base + 1 : in_path;
    char stem[256];
    snprintf(stem, sizeof(stem), "%s", base);
    char *dot = strrchr(stem, '.');
    if (dot) strcpy(dot, ".cpp");
    else strcat(stem, ".cpp");

    char out_path[512];
    if (access(PROJECT_MAIN, F_OK) == 0) {
        olrn_mkdir("olrn_out");
        snprintf(out_path, sizeof(out_path), "olrn_out/%s", stem);
    } else {
        snprintf(out_path, sizeof(out_path), "%s", stem);
    }

    FILE *f = fopen(out_path, "w");
    if (!f) { perror(out_path); return 1; }
    int rc = compile_to_out(in_path, f);
    fclose(f);
    if (rc != 0) { remove(out_path); return 1; }
    printf("wrote: %s\n", out_path);
    return 0;
}

/* olrn build-out <file.cpp> [-o=name]  — compile existing C++ to binary.
   In a project, defaults to bin/<stem>; flat mode defaults to ./<stem>. */
static int cmd_build_out(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: olrn build-out <file.cpp> [-o=name]\n");
        return 1;
    }

    const char *cpp_path = NULL;
    const char *output   = NULL;

    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "-o=", 3) == 0)
            output = argv[i] + 3;
        else
            cpp_path = argv[i];
    }

    if (!cpp_path) {
        fprintf(stderr, "build-out: no input file\n");
        return 1;
    }

    char derived[512];
    if (!output) {
        const char *base = strrchr(cpp_path, '/');
        base = base ? base + 1 : cpp_path;
        char stem[256];
        snprintf(stem, sizeof(stem), "%s", base);
        char *dot = strrchr(stem, '.');
        if (dot) *dot = '\0';
        if (access(PROJECT_MAIN, F_OK) == 0) {
            olrn_mkdir("bin");
            snprintf(derived, sizeof(derived), "bin/%s", stem);
        } else {
            snprintf(derived, sizeof(derived), "%s", stem);
        }
        output = derived;
    }

    int rc = gxx_compile(cpp_path, output);
    if (rc == 0) printf("built: %s\n", output);
    return rc;
}

/* olrn sac <file(s)> [-o=name] */
static int cmd_sac(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: olrn sac <file.olrn> [more.olrn ...] [-o=name]\n");
        return 1;
    }

    const char *inputs[64];
    int input_count = 0;
    const char *output = NULL;

    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "-o=", 3) == 0)
            output = argv[i] + 3;
        else
            inputs[input_count++] = argv[i];
    }

    if (input_count == 0) {
        fprintf(stderr, "sac: no input files\n");
        return 1;
    }

    /* derive output name from first input if not given */
    char derived[256];
    if (!output) {
        const char *base = strrchr(inputs[0], '/');
        base = base ? base + 1 : inputs[0];
        snprintf(derived, sizeof(derived), "%s", base);
        char *dot = strrchr(derived, '.');
        if (dot) *dot = '\0';
        output = derived;
    }

    int rc = compile_to_binary(inputs, input_count, output);
    if (rc == 0) printf("built: %s\n", output);
    return rc;
}

/* olrn build — compile the project entry point.
   Project layout: src/main/olrn/main.olrn → olrn_out/<name>.cpp →
   bin/<name>. A bare main.olrn in cwd still works (flat mode) and
   builds to ./<name>. */
static int cmd_build(void)
{
    const char *name = project_name();
    if (!name) return 1;

    g_tree = 1;
    printf("%s\n", name);

    if (access(PROJECT_MAIN, F_OK) == 0) {
        olrn_mkdir("olrn_out");
        olrn_mkdir("bin");

        char cpp_path[600], bin_path[600];
        snprintf(cpp_path, sizeof(cpp_path), "olrn_out/%s.cpp", name);
        snprintf(bin_path, sizeof(bin_path), "bin/%s", name);

        FILE *out = fopen(cpp_path, "w");
        if (!out) { perror(cpp_path); return 1; }
        int rc = compile_to_out(PROJECT_MAIN, out);
        fclose(out);
        if (rc != 0) { remove(cpp_path); return 1; }
        tree_step(0, "codegen", cpp_path);

        tree_step(1, "g++", bin_path);
        rc = gxx_compile(cpp_path, bin_path);
        if (rc == 0) { printf("built: %s\n", bin_path); fflush(stdout); }
        return rc;
    }

    if (access("main.olrn", F_OK) == 0) {
        /* flat mode — single file next to you, binary in cwd */
        const char *inputs[] = { "main.olrn" };
        int rc = compile_to_binary(inputs, 1, name);
        if (rc == 0) { printf("built: %s\n", name); fflush(stdout); }
        return rc;
    }

    fprintf(stderr, "build: no %s or main.olrn found — run 'olrn init'\n",
            PROJECT_MAIN);
    return 1;
}

/* olrn run  — build then execute */
static int cmd_run(void)
{
    const char *name = project_name();
    if (!name) return 1;

    char cmd[768];
    if (access(PROJECT_MAIN, F_OK) == 0)
        snprintf(cmd, sizeof(cmd), "./bin/%s", name);
    else
        snprintf(cmd, sizeof(cmd), "./%s", name);

    if (access(cmd + 2, F_OK) != 0) {
        fprintf(stderr, "error: '%s' not found — run 'olrn build' first\n",
                cmd + 2);
        return 1;
    }
    return system(cmd);
}

/* ── entry point ──────────────────────────────────────────────── */

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_help();
        return 1;
    }

    const char *sub = argv[1];

    if (strcmp(sub, "version") == 0 || strcmp(sub, "-V") == 0) {
        printf("olrn %s\n", OLRN_VERSION);
        return 0;
    }
    if (strcmp(sub, "help") == 0 || strcmp(sub, "-h") == 0) {
        print_help();
        return 0;
    }
    if (strcmp(sub, "init")      == 0) return cmd_init(argc, argv);
    if (strcmp(sub, "deps")      == 0) return cmd_deps(argc, argv);
    if (strcmp(sub, "check")     == 0) return cmd_check(argc, argv);
    if (strcmp(sub, "emit")      == 0) return cmd_emit(argc, argv);
    if (strcmp(sub, "build-src") == 0) return cmd_build_src(argc, argv);
    if (strcmp(sub, "build-out") == 0) return cmd_build_out(argc, argv);
    if (strcmp(sub, "sac")       == 0) return cmd_sac(argc, argv);
    if (strcmp(sub, "build")     == 0) return cmd_build();
    if (strcmp(sub, "run")       == 0) return cmd_run();
    if (strcmp(sub, "view")      == 0) return cmd_view(argc, argv);

    if (sub[0] == '-') {
        fprintf(stderr, "unknown option '%s' — commands take no dashes "
                        "(try 'olrn help')\n", sub);
        return 1;
    }

    /* bare file path — emit C++ to stdout (backward-compatible) */
    return compile_to_out(sub, stdout);
}
