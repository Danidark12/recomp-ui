/* recomp_cheats.c - leitura/gravação de cheats.txt (ver recomp_cheats.h). */

#include "recomp_cheats.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <unistd.h>
#endif

static char s_path[4200];

const char* recomp_cheats_path(void) {
    if (s_path[0]) return s_path;
    char dir[4096] = "";
#if defined(_WIN32)
    wchar_t wbuf[2048];
    DWORD n = GetModuleFileNameW(NULL, wbuf, (DWORD)(sizeof(wbuf) / sizeof(wbuf[0])));
    if (n > 0 && n < sizeof(wbuf) / sizeof(wbuf[0])) {
        int len = WideCharToMultiByte(CP_UTF8, 0, wbuf, (int)n, dir,
                                      (int)sizeof(dir) - 1, NULL, NULL);
        if (len > 0) {
            dir[len] = 0;
            char* cut = strrchr(dir, '\\');
            char* cut2 = strrchr(dir, '/');
            if (cut2 > cut) cut = cut2;
            if (cut) cut[1] = 0; else dir[0] = 0;
        }
    }
#else
    ssize_t n = readlink("/proc/self/exe", dir, sizeof(dir) - 1);
    if (n > 0) {
        dir[n] = 0;
        char* cut = strrchr(dir, '/');
        if (cut) cut[1] = 0; else dir[0] = 0;
    } else {
        dir[0] = 0;
    }
#endif
    snprintf(s_path, sizeof(s_path), "%scheats.txt", dir);
    return s_path;
}

static FILE* open_utf8(const char* path, const char* mode) {
#if defined(_WIN32)
    wchar_t wpath[2048], wmode[8];
    if (!MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 2048)) return NULL;
    if (!MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, 8)) return NULL;
    return _wfopen(wpath, wmode);
#else
    return fopen(path, mode);
#endif
}

int recomp_cheat_code_type_supported(uint32_t addr_word) {
    switch (addr_word >> 24) {
    case 0x80: case 0x30:
    case 0xD0: case 0xD1: case 0xD2: case 0xD3:
    case 0xE0: case 0xE1: case 0xE2: case 0xE3:
    case 0x50:
        return 1;
    default:
        return 0;
    }
}

static void trim(char* s) {
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == ' ' || s[n - 1] == '\t'))
        s[--n] = 0;
    size_t i = 0;
    while (s[i] == ' ' || s[i] == '\t') i++;
    if (i) memmove(s, s + i, n - i + 1);
}

/* "texto pt | texto en | texto es" -> três campos (en/es opcionais) */
static void copy_field(const char* b, const char* e, char* out, size_t cap) {
    size_t n = (size_t)(e - b);
    if (n >= cap) n = cap - 1;
    memcpy(out, b, n);
    out[n] = 0;
    trim(out);
}

static void split_lang(const char* src, char* pt, size_t pt_cap, char* en, size_t en_cap,
                       char* es, size_t es_cap) {
    const char* end = src + strlen(src);
    const char* bar1 = strstr(src, " | ");
    const char* bar2 = bar1 ? strstr(bar1 + 3, " | ") : NULL;
    copy_field(src, bar1 ? bar1 : end, pt, pt_cap);
    if (bar1) copy_field(bar1 + 3, bar2 ? bar2 : end, en, en_cap); else en[0] = 0;
    if (bar2) copy_field(bar2 + 3, end, es, es_cap); else es[0] = 0;
}

static void append_note(char* dst, size_t cap, const char* add) {
    if (!add[0]) return;
    if (dst[0] && strlen(dst) + 2 < cap) strcat(dst, " ");
    if (strlen(dst) + strlen(add) + 1 < cap) strcat(dst, add);
}

static int parse_code(const char* s, uint32_t* a, uint16_t* v) {
    char hex[16];
    int n = 0;
    uint32_t w1 = 0, w2 = 0;
    /* aceita "XXXXXXXX YYYY" (com espaço) ou "XXXXXXXXYYYY" */
    for (const char* p = s; *p && n < 12; ++p) {
        if (*p == ' ' || *p == '\t' || *p == '-') continue;
        if (!isxdigit((unsigned char)*p)) return 0;
        hex[n++] = *p;
    }
    if (n != 12) return 0;
    for (int i = 0; i < 8; ++i) {
        char c = hex[i];
        w1 = (w1 << 4) | (uint32_t)(isdigit((unsigned char)c) ? c - '0' : (tolower((unsigned char)c) - 'a' + 10));
    }
    for (int i = 8; i < 12; ++i) {
        char c = hex[i];
        w2 = (w2 << 4) | (uint32_t)(isdigit((unsigned char)c) ? c - '0' : (tolower((unsigned char)c) - 'a' + 10));
    }
    *a = w1;
    *v = (uint16_t)w2;
    return 1;
}

int recomp_cheats_load(RecompCheatList* list) {
    if (!list) return 0;
    memset(list, 0, sizeof(*list));
    FILE* f = open_utf8(recomp_cheats_path(), "rb");
    if (!f) return 0;
    list->loaded = 1;
    char line[1024];
    RecompCheat* cur = NULL;
    int first = 1;
    while (fgets(line, sizeof(line), f)) {
        if (first) {   /* BOM do UTF-8 */
            first = 0;
            if ((unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB &&
                (unsigned char)line[2] == 0xBF)
                memmove(line, line + 3, strlen(line + 3) + 1);
        }
        char raw[1024];
        snprintf(raw, sizeof(raw), "%s", line);
        trim(line);
        if (!line[0]) continue;
        if (line[0] == '#') {
            if (!cur) {
                size_t used = strlen(list->header);
                trim(raw);
                if (used + strlen(raw) + 2 < sizeof(list->header)) {
                    strcat(list->header, raw);
                    strcat(list->header, "\n");
                }
            }
            continue;
        }
        if (line[0] == '[') {
            char* close = strchr(line, ']');
            if (!close || list->count >= RECOMP_CHEATS_MAX) { cur = NULL; continue; }
            cur = &list->cheats[list->count++];
            memset(cur, 0, sizeof(*cur));
            cur->supported = 1;
            for (char* p = line + 1; p < close; ++p)
                if (*p == 'x' || *p == 'X' || *p == '*') cur->enabled = 1;
            split_lang(close + 1, cur->name_pt, sizeof(cur->name_pt),
                       cur->name_en, sizeof(cur->name_en),
                       cur->name_es, sizeof(cur->name_es));
            continue;
        }
        if (!cur) continue;
        if (line[0] == '>') {
            char pt[RECOMP_CHEATS_NOTE], en[RECOMP_CHEATS_NOTE], es[RECOMP_CHEATS_NOTE];
            split_lang(line + 1, pt, sizeof(pt), en, sizeof(en), es, sizeof(es));
            append_note(cur->note_pt, sizeof(cur->note_pt), pt);
            append_note(cur->note_en, sizeof(cur->note_en), en);
            append_note(cur->note_es, sizeof(cur->note_es), es);
            continue;
        }
        uint32_t a;
        uint16_t v;
        if (parse_code(line, &a, &v)) {
            if (cur->code_count < RECOMP_CHEATS_MAX_CODES) {
                cur->addr[cur->code_count] = a;
                cur->value[cur->code_count] = v;
                cur->code_count++;
                /* a linha seguinte a um 50 é o par endereço/valor inicial */
                int after_serial = cur->code_count >= 2 &&
                                   (cur->addr[cur->code_count - 2] >> 24) == 0x50;
                if (!after_serial && !recomp_cheat_code_type_supported(a))
                    cur->supported = 0;
            } else {
                cur->supported = 0;
            }
        } else {
            cur->supported = 0;   /* linha que não é código nem comentário */
        }
    }
    fclose(f);
    for (int i = 0; i < list->count; ++i)
        if (list->cheats[i].code_count == 0) list->cheats[i].supported = 0;
    return 1;
}

int recomp_cheats_save(const RecompCheatList* list) {
    if (!list || !list->loaded) return 0;
    char tmp[4200];
    snprintf(tmp, sizeof(tmp), "%s.tmp", recomp_cheats_path());
    FILE* f = open_utf8(tmp, "wb");
    if (!f) return 0;
    fputs(list->header, f);
    for (int i = 0; i < list->count; ++i) {
        const RecompCheat* c = &list->cheats[i];
        fprintf(f, "\n[%s] %s", c->enabled ? "x" : " ", c->name_pt);
        if (c->name_en[0] || c->name_es[0]) fprintf(f, " | %s", c->name_en);
        if (c->name_es[0]) fprintf(f, " | %s", c->name_es);
        fputc('\n', f);
        if (c->note_pt[0] || c->note_en[0] || c->note_es[0]) {
            fprintf(f, "> %s", c->note_pt);
            if (c->note_en[0] || c->note_es[0]) fprintf(f, " | %s", c->note_en);
            if (c->note_es[0]) fprintf(f, " | %s", c->note_es);
            fputc('\n', f);
        }
        for (int k = 0; k < c->code_count; ++k)
            fprintf(f, "%08X %04X\n", (unsigned)c->addr[k], (unsigned)c->value[k]);
    }
    if (fclose(f) != 0) return 0;
#if defined(_WIN32)
    wchar_t wtmp[2048], wdst[2048];
    if (!MultiByteToWideChar(CP_UTF8, 0, tmp, -1, wtmp, 2048) ||
        !MultiByteToWideChar(CP_UTF8, 0, recomp_cheats_path(), -1, wdst, 2048))
        return 0;
    return MoveFileExW(wtmp, wdst, MOVEFILE_REPLACE_EXISTING) ? 1 : 0;
#else
    return rename(tmp, recomp_cheats_path()) == 0;
#endif
}

int recomp_cheat_lang_from_code(const char* code) {
    if (!code) return RECOMP_CHEAT_LANG_EN;
    if (code[0] == 'p' || code[0] == 'P') return RECOMP_CHEAT_LANG_PT;
    if ((code[0] == 'e' || code[0] == 'E') && (code[1] == 's' || code[1] == 'S'))
        return RECOMP_CHEAT_LANG_ES;
    return RECOMP_CHEAT_LANG_EN;
}

static const char* pick(const char* en, const char* pt, const char* es, int lang) {
    if (lang == RECOMP_CHEAT_LANG_ES && es[0]) return es;
    if (lang == RECOMP_CHEAT_LANG_PT && pt[0]) return pt;
    return en[0] ? en : pt;
}

/* ---- Filtro de tela ---------------------------------------------------- */
static void screen_filter_path(char* out, size_t cap) {
    const char* cp = recomp_cheats_path();          /* <pasta do exe>/cheats.txt */
    size_t n = strlen(cp);
    const size_t tail = strlen("cheats.txt");
    if (n >= tail) n -= tail;
    snprintf(out, cap, "%.*sscreen_filter.txt", (int)n, cp);
}

int recomp_screen_filter_load(void) {
    char path[4200];
    screen_filter_path(path, sizeof(path));
    FILE* f = open_utf8(path, "rb");
    if (!f) return 0;
    int v = 0;
    if (fscanf(f, "%d", &v) != 1) v = 0;
    fclose(f);
    return (v >= 0 && v < RECOMP_SCREEN_FILTER_COUNT) ? v : 0;
}

void recomp_screen_filter_save(int mode) {
    char path[4200];
    screen_filter_path(path, sizeof(path));
    FILE* f = open_utf8(path, "wb");
    if (!f) return;
    fprintf(f, "%d\n", (mode >= 0 && mode < RECOMP_SCREEN_FILTER_COUNT) ? mode : 0);
    fclose(f);
}

const char* recomp_screen_filter_name(int mode) {
    static const char* k[RECOMP_SCREEN_FILTER_COUNT] = {
        "None", "LCD grid", "LCD RGB", "CRT", "Soft CRT" };
    return (mode >= 0 && mode < RECOMP_SCREEN_FILTER_COUNT) ? k[mode] : k[0];
}

const char* recomp_screen_filter_name_upper(int mode, int lang) {
    static const char* en[RECOMP_SCREEN_FILTER_COUNT] = {
        "NONE", "LCD GRID", "LCD RGB", "CRT", "SOFT CRT" };
    static const char* pt[RECOMP_SCREEN_FILTER_COUNT] = {
        "NENHUM", "LCD (GRADE)", "LCD RGB", "CRT", "CRT SUAVE" };
    static const char* es[RECOMP_SCREEN_FILTER_COUNT] = {
        "NINGUNO", "LCD (REJILLA)", "LCD RGB", "CRT", "CRT SUAVE" };
    if (mode < 0 || mode >= RECOMP_SCREEN_FILTER_COUNT) mode = 0;
    return lang == 1 ? pt[mode] : lang == 2 ? es[mode] : en[mode];
}

const char* recomp_cheat_name(const RecompCheat* c, int lang) {
    if (!c) return "";
    return pick(c->name_en, c->name_pt, c->name_es, lang);
}

const char* recomp_cheat_note(const RecompCheat* c, int lang) {
    if (!c) return "";
    return pick(c->note_en, c->note_pt, c->note_es, lang);
}
