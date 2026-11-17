// recomp_bezel_preview.c — see recomp_bezel_preview.h.
#include "recomp_bezel_preview.h"

#include <stdlib.h>

// Area-average the source rectangle that maps onto destination pixel (dx,dy)
// of a dw x dh image, writing one RGBA pixel.
static void box_sample(const unsigned char* src, int sw, int sh, int dw, int dh,
                       int dx, int dy, unsigned char out[4]) {
    int x0 = (int)((long long)dx * sw / dw);
    int x1 = (int)(((long long)(dx + 1) * sw + dw - 1) / dw);
    int y0 = (int)((long long)dy * sh / dh);
    int y1 = (int)(((long long)(dy + 1) * sh + dh - 1) / dh);
    if (x1 <= x0) x1 = x0 + 1;
    if (y1 <= y0) y1 = y0 + 1;
    if (x1 > sw) x1 = sw;
    if (y1 > sh) y1 = sh;
    unsigned long long acc[4] = { 0, 0, 0, 0 };
    unsigned long long n = 0;
    for (int y = y0; y < y1; ++y) {
        const unsigned char* p = src + ((size_t)y * (size_t)sw + (size_t)x0) * 4u;
        for (int x = x0; x < x1; ++x, p += 4) {
            // Premultiply so transparent texels do not bleed their colour.
            acc[0] += (unsigned long long)p[0] * p[3];
            acc[1] += (unsigned long long)p[1] * p[3];
            acc[2] += (unsigned long long)p[2] * p[3];
            acc[3] += p[3];
            ++n;
        }
    }
    if (n == 0 || acc[3] == 0) { out[0] = out[1] = out[2] = out[3] = 0; return; }
    out[0] = (unsigned char)(acc[0] / acc[3]);
    out[1] = (unsigned char)(acc[1] / acc[3]);
    out[2] = (unsigned char)(acc[2] / acc[3]);
    out[3] = (unsigned char)(acc[3] / n);
}

unsigned char* recomp_bezel_preview_compose(const unsigned char* bezel, int bw, int bh,
                                            const unsigned char* sample, int sw, int sh,
                                            int max_w, int max_h, int* ow, int* oh) {
    if (!bezel || bw <= 0 || bh <= 0 || max_w <= 0 || max_h <= 0) return NULL;
    if (sample && (sw <= 0 || sh <= 0)) sample = NULL;
    int w = max_w;
    int h = (int)((long long)bh * w / bw);
    if (h > max_h) { h = max_h; w = (int)((long long)bw * h / bh); }
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    unsigned char* out = (unsigned char*)malloc((size_t)w * (size_t)h * 4u);
    if (!out) return NULL;

    // The game picture: 4:3 at full height, centred (narrower frames clip it).
    int pw = h * 4 / 3;
    if (pw > w) pw = w;
    const int px0 = (w - pw) / 2;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            unsigned char* d = out + ((size_t)y * (size_t)w + (size_t)x) * 4u;
            unsigned char c[4];
            if (x >= px0 && x < px0 + pw) {
                if (sample) {
                    box_sample(sample, sw, sh, pw, h, x - px0, y, c);
                    d[0] = c[0]; d[1] = c[1]; d[2] = c[2];
                } else {
                    d[0] = d[1] = d[2] = 0;
                }
            } else {
                // Frame over black, as in game (the bars are black behind it).
                box_sample(bezel, bw, bh, w, h, x, y, c);
                d[0] = (unsigned char)(c[0] * c[3] / 255);
                d[1] = (unsigned char)(c[1] * c[3] / 255);
                d[2] = (unsigned char)(c[2] * c[3] / 255);
            }
            d[3] = 255;
        }
    }
    if (ow) *ow = w;
    if (oh) *oh = h;
    return out;
}

void recomp_bezel_preview_free(unsigned char* pixels) {
    free(pixels);
}
