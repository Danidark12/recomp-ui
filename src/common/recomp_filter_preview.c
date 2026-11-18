// recomp_filter_preview.c — see recomp_filter_preview.h.
#include "recomp_filter_preview.h"

#include <math.h>
#include <stdlib.h>

typedef struct { float r, g, b; } V3;

typedef struct {
    const unsigned char* px;
    int w, h;
} Src;

static V3 tap(const Src* s, int x, int y) {
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x >= s->w) x = s->w - 1;
    if (y >= s->h) y = s->h - 1;
    const unsigned char* p = s->px + ((size_t)y * (size_t)s->w + (size_t)x) * 4u;
    V3 v = { p[0] / 255.0f, p[1] / 255.0f, p[2] / 255.0f };
    return v;
}

static V3 mix3(V3 a, V3 b, float t) {
    V3 v = { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
    return v;
}
static V3 add3(V3 a, V3 b) { V3 v = { a.r + b.r, a.g + b.g, a.b + b.b }; return v; }
static V3 mul3(V3 a, float k) { V3 v = { a.r * k, a.g * k, a.b * k }; return v; }
static float clampf(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }
static float smooth(float e0, float e1, float x) {
    float t = clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
static int same(V3 a, V3 b) {
    return fabsf(a.r - b.r) < 0.004f && fabsf(a.g - b.g) < 0.004f && fabsf(a.b - b.b) < 0.004f;
}
static float luma(V3 c) { return c.r * 14.352f + c.g * 28.176f + c.b * 5.472f; }

// Same rules as PSX_UPSCALE_FUNC. gx, gy: position in source pixels.
static V3 upscale(const Src* s, float gx, float gy, float sc, int mode) {
    const int cx = (int)floorf(gx), cy = (int)floorf(gy);
    const float fx = gx - cx, fy = gy - cy;
    if (mode == 1) {   // Sharp Bilinear
        const float band = 0.5f - 0.5f / sc;
        float dx = fx - 0.5f, dy = fy - 0.5f;
        float px = cx + (dx - clampf(dx, -band, band)) * sc;
        float py = cy + (dy - clampf(dy, -band, band)) * sc;
        int ix = (int)floorf(px), iy = (int)floorf(py);
        float wx = px - ix, wy = py - iy;
        V3 a = tap(s, ix, iy), b = tap(s, ix + 1, iy);
        V3 c = tap(s, ix, iy + 1), d = tap(s, ix + 1, iy + 1);
        return mix3(mix3(a, b, wx), mix3(c, d, wx), wy);
    }
    V3 E = tap(s, cx, cy);
    V3 A = tap(s, cx - 1, cy - 1), B = tap(s, cx, cy - 1), C = tap(s, cx + 1, cy - 1);
    V3 D = tap(s, cx - 1, cy), F = tap(s, cx + 1, cy);
    V3 G = tap(s, cx - 1, cy + 1), H = tap(s, cx, cy + 1), I = tap(s, cx + 1, cy + 1);
    if (mode == 2) {   // Scale2x
        V3 th = fx < 0.5f ? D : F, oh = fx < 0.5f ? F : D;
        V3 tv = fy < 0.5f ? B : H, ov = fy < 0.5f ? H : B;
        return (same(th, tv) && !same(tv, oh) && !same(th, ov)) ? th : E;
    }
    if (mode == 3) {   // Scale3x
        int db = same(D, B), bf = same(B, F), dh = same(D, H), hf = same(H, F);
        int sx = (int)floorf(fx * 3.0f), sy = (int)floorf(fy * 3.0f);
        if (sx > 2) sx = 2;
        if (sy > 2) sy = 2;
        switch (sx + sy * 3) {
        case 0: return (db && !bf && !dh) ? D : E;
        case 1: return ((db && !bf && !dh && !same(E, C)) || (bf && !db && !hf && !same(E, A))) ? B : E;
        case 2: return (bf && !db && !hf) ? F : E;
        case 3: return ((db && !bf && !dh && !same(E, G)) || (dh && !db && !hf && !same(E, A))) ? D : E;
        case 4: return E;
        case 5: return ((bf && !db && !hf && !same(E, I)) || (hf && !dh && !bf && !same(E, C))) ? F : E;
        case 6: return (dh && !db && !hf) ? D : E;
        case 7: return ((dh && !db && !hf && !same(E, I)) || (hf && !dh && !bf && !same(E, G))) ? H : E;
        default: return (hf && !dh && !bf) ? F : E;
        }
    }
    // xBR (level 2 rules, all four corners, highest coverage wins).
    V3 P[25];
    float Y[25];
    for (int j = -2; j <= 2; ++j)
        for (int i = -2; i <= 2; ++i) {
            int k = (i + 2) + (j + 2) * 5;
            if (abs(i) == 2 && abs(j) == 2) { P[k] = E; Y[k] = 0.0f; continue; }
            P[k] = tap(s, cx + i, cy + j);
            Y[k] = luma(P[k]);
        }
    const float delta = 0.5f / sc;
    float best = 0.0f;
    V3 bestc = E;
    for (int q = 0; q < 4; ++q) {
        const int sx = (q & 1) == 0 ? 1 : -1, sy = (q & 2) == 0 ? 1 : -1;
        const float mx = sx > 0 ? fx : 1.0f - fx, my = sy > 0 ? fy : 1.0f - fy;
#define XI(x, y) ((((x) * sx) + 2) + (((y) * sy) + 2) * 5)
        float e = Y[XI(0, 0)], f = Y[XI(1, 0)], h = Y[XI(0, 1)], i9 = Y[XI(1, 1)];
        float b = Y[XI(0, -1)], c = Y[XI(1, -1)], d = Y[XI(-1, 0)], g = Y[XI(-1, 1)];
        float f4 = Y[XI(2, 0)], i4 = Y[XI(2, 1)], h5 = Y[XI(0, 2)], i5 = Y[XI(1, 2)];
        if (e == f || e == h) continue;
        float wd1 = fabsf(e - c) + fabsf(e - g) + fabsf(i9 - h5) + fabsf(i9 - f4) + 4.0f * fabsf(h - f);
        float wd2 = fabsf(h - d) + fabsf(h - i5) + fabsf(f - i4) + fabsf(f - b) + 4.0f * fabsf(e - i9);
        int lv1 = (fabsf(f - b) >= 15.0f && fabsf(h - d) >= 15.0f) ||
                  (fabsf(e - i9) < 15.0f && fabsf(f - i4) >= 15.0f && fabsf(h - i5) >= 15.0f) ||
                  fabsf(e - g) < 15.0f || fabsf(e - c) < 15.0f;
        if (!(wd1 < wd2) || !lv1) continue;
        int left = (2.0f * fabsf(f - g) <= fabsf(h - c)) && e != g && d != g;
        int up   = (fabsf(f - g) >= 2.0f * fabsf(h - c)) && e != c && b != c;
        float cov = smooth(1.5f - delta, 1.5f + delta, mx + my);
        if (left) { float t = smooth(1.0f - delta, 1.0f + delta, my + 0.5f * mx); if (t > cov) cov = t; }
        if (up)   { float t = smooth(1.0f - delta, 1.0f + delta, mx + 0.5f * my); if (t > cov) cov = t; }
        V3 col = fabsf(e - f) <= fabsf(e - h) ? P[XI(1, 0)] : P[XI(0, 1)];
#undef XI
        if (cov > best) { best = cov; bestc = col; }
    }
    return mix3(E, bestc, best);
}

// Same as psx_filter (the mask) at sx == sy == sc. ox: output column (CRT mask).
static V3 mask(V3 rgb, float fx, float fy, float sc, int mode, int ox) {
    if (mode == 0) return rgb;
    const float gate = clampf(sc - 2.0f, 0.0f, 1.0f);
    if (gate <= 0.0f) return rgb;
    V3 o = rgb;
    if (mode == 1 || mode == 2) {
        float ex = (fx < 1.0f - fx ? fx : 1.0f - fx) * sc;
        float ey = (fy < 1.0f - fy ? fy : 1.0f - fy) * sc;
        float g = smooth(0.0f, 1.0f, ex < ey ? ex : ey);
        o = mul3(rgb, 0.55f + 0.45f * g);
        if (mode == 2) {
            float t = fx * 3.0f;
            float mr = 1.0f - clampf(fabsf(t - 0.5f), 0.0f, 1.0f);
            float mg = 1.0f - clampf(fabsf(t - 1.5f), 0.0f, 1.0f);
            float mb = 1.0f - clampf(fabsf(t - 2.5f), 0.0f, 1.0f);
            o.r *= 1.0f + ((0.45f + 0.9f * mr) - 1.0f) * 0.6f;
            o.g *= 1.0f + ((0.45f + 0.9f * mg) - 1.0f) * 0.6f;
            o.b *= 1.0f + ((0.45f + 0.9f * mb) - 1.0f) * 0.6f;
        }
    } else if (mode == 5) {
        float dx = fx - 0.5f, dy = fy - 0.5f;
        float r = sqrtf(dx * dx + dy * dy) * 2.0f;
        float aa = 2.0f / (sc > 1.0f ? sc : 1.0f);
        float m = 1.0f - smooth(0.86f - aa, 0.86f + aa, r);
        o = mul3(rgb, 0.38f + (1.12f - 0.38f) * m);
    } else {
        int k = ox % 3;
        V3 mk = k == 0 ? (V3){ 1.0f, 0.7f, 0.7f } : k == 1 ? (V3){ 0.7f, 1.0f, 0.7f } : (V3){ 0.7f, 0.7f, 1.0f };
        if (mode == 3) {
            float beam = 0.62f + 0.38f * sinf(3.14159265f * fy);
            o.r = rgb.r * mk.r * beam * 1.35f;
            o.g = rgb.g * mk.g * beam * 1.35f;
            o.b = rgb.b * mk.b * beam * 1.35f;
        } else {
            o.r = rgb.r * (1.0f + (mk.r - 1.0f) * 0.5f) * 1.12f;
            o.g = rgb.g * (1.0f + (mk.g - 1.0f) * 0.5f) * 1.12f;
            o.b = rgb.b * (1.0f + (mk.b - 1.0f) * 0.5f) * 1.12f;
        }
    }
    o.r = clampf(o.r, 0.0f, 1.0f); o.g = clampf(o.g, 0.0f, 1.0f); o.b = clampf(o.b, 0.0f, 1.0f);
    return mix3(rgb, o, gate);
}

static V3 bilinear(const Src* s, float gx, float gy) {
    const float px = gx - 0.5f, py = gy - 0.5f;
    const int ix = (int)floorf(px), iy = (int)floorf(py);
    const float wx = px - ix, wy = py - iy;
    V3 a = tap(s, ix, iy), b = tap(s, ix + 1, iy);
    V3 c = tap(s, ix, iy + 1), d = tap(s, ix + 1, iy + 1);
    return mix3(mix3(a, b, wx), mix3(c, d, wx), wy);
}

unsigned char* recomp_filter_preview_render(const unsigned char* src, int sw, int sh,
                                            int cx, int cy, int cw, int ch, int scale,
                                            int upscale_mode, int filter,
                                            int scanlines, float strength,
                                            int* ow, int* oh) {
    return recomp_filter_preview_render_ex(src, sw, sh, cx, cy, cw, ch, scale,
                                           upscale_mode, filter, scanlines, strength,
                                           0, ow, oh);
}

unsigned char* recomp_filter_preview_render_ex(const unsigned char* src, int sw, int sh,
                                               int cx, int cy, int cw, int ch, int scale,
                                               int upscale_mode, int filter,
                                               int scanlines, float strength, int smooth,
                                               int* ow, int* oh) {
    if (!src || sw <= 0 || sh <= 0 || cw <= 0 || ch <= 0 || scale <= 0) return NULL;
    const int w = cw * scale, h = ch * scale;
    unsigned char* out = (unsigned char*)malloc((size_t)w * (size_t)h * 4u);
    if (!out) return NULL;
    const Src s = { src, sw, sh };
    const float sc = (float)scale;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float gx = cx + (x + 0.5f) / sc, gy = cy + (y + 0.5f) / sc;
            const float fx = gx - floorf(gx), fy = gy - floorf(gy);
            V3 c = upscale_mode > 0 ? upscale(&s, gx, gy, sc, upscale_mode)
                 : smooth >= 2
                     ? mul3(add3(add3(bilinear(&s, gx - 0.35f, gy - 0.35f), bilinear(&s, gx + 0.35f, gy - 0.35f)),
                                 add3(bilinear(&s, gx - 0.35f, gy + 0.35f), bilinear(&s, gx + 0.35f, gy + 0.35f))), 0.25f)
                 : smooth ? bilinear(&s, gx, gy)
                          : tap(&s, (int)floorf(gx), (int)floorf(gy));
            if (scanlines) {
                const float gate = clampf(sc - 1.0f, 0.0f, 1.0f);
                const float beam = sinf(3.14159265f * fy);
                c = mul3(c, 1.0f - (strength * gate) * (1.0f - beam));
            }
            c = mask(c, fx, fy, sc, filter, x);
            unsigned char* d = out + ((size_t)y * (size_t)w + (size_t)x) * 4u;
            d[0] = (unsigned char)(clampf(c.r, 0.0f, 1.0f) * 255.0f + 0.5f);
            d[1] = (unsigned char)(clampf(c.g, 0.0f, 1.0f) * 255.0f + 0.5f);
            d[2] = (unsigned char)(clampf(c.b, 0.0f, 1.0f) * 255.0f + 0.5f);
            d[3] = 255;
        }
    }
    if (ow) *ow = w;
    if (oh) *oh = h;
    return out;
}

void recomp_filter_preview_free(unsigned char* pixels) {
    free(pixels);
}
