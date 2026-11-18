// recomp_filter_preview.h — CPU rendering of the screen filters for the
// launcher and in-game menu previews. Mirrors the runtime's present shaders
// (psxrecomp gpu_gl_renderer.c: PSX_UPSCALE_FUNC / psx_filter /
// psx_scanline) on a small crop of a sample game picture, so the preview
// shows what a combination looks like without needing the game running.
#ifndef RECOMP_FILTER_PREVIEW_H
#define RECOMP_FILTER_PREVIEW_H

#ifdef __cplusplus
extern "C" {
#endif

// src: RGBA8 sw x sh. The crop (cx, cy, cw, ch) is drawn at `scale` output
// pixels per source pixel. upscale / filter use the recomp_cheats.h
// numbering; scanlines 0/1 with strength 0..1. Returns a malloc'd opaque
// RGBA8 image of (cw*scale) x (ch*scale), or NULL; free with
// recomp_filter_preview_free().
unsigned char* recomp_filter_preview_render(const unsigned char* src, int sw, int sh,
                                            int cx, int cy, int cw, int ch, int scale,
                                            int upscale, int filter,
                                            int scanlines, float strength,
                                            int* ow, int* oh);
void recomp_filter_preview_free(unsigned char* pixels);

// Same, plus `smooth`: with no upscaler, 0 samples nearest, 1 bilinear (one of
// Texture filtering = Bilinear / Screen smoothing on), 2 bilinear plus a light
// blur (both on). An upscaler, when chosen, replaces this sampling.
unsigned char* recomp_filter_preview_render_ex(const unsigned char* src, int sw, int sh,
                                               int cx, int cy, int cw, int ch, int scale,
                                               int upscale, int filter,
                                               int scanlines, float strength, int smooth,
                                               int* ow, int* oh);

// Screen-model colour hook. The colour simulation (Raw / CRT / Composite /
// Trinitron) lives in the host runtime; it registers a function that rewrites
// an RGBA8 buffer in place for a given model. Without a hook the preview keeps
// the raw colours.
typedef void (*RecompPreviewColorFn)(int screen_kind, unsigned char* rgba, int pixels);
void recomp_filter_preview_set_color_hook(RecompPreviewColorFn fn);
// Copy of the sample with the screen model applied (malloc'd; NULL on failure
// or when screen_kind is 0 / no hook — then use the original).
unsigned char* recomp_filter_preview_colorize(const unsigned char* rgba, int w, int h,
                                              int screen_kind);

// The crop used for the bundled sample (assets/frame_preview_sample.png:
// Lenneth on Asgard Hill, 320x224).
#define RECOMP_FILTER_PREVIEW_CROP_X 118
#define RECOMP_FILTER_PREVIEW_CROP_Y 122
#define RECOMP_FILTER_PREVIEW_CROP_W 72
#define RECOMP_FILTER_PREVIEW_CROP_H 66

#ifdef __cplusplus
}
#endif

#endif
