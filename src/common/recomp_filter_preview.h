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

// The crop used for the bundled sample (assets/frame_preview_sample.png:
// Lenneth at a save point, 320x224).
#define RECOMP_FILTER_PREVIEW_CROP_X 130
#define RECOMP_FILTER_PREVIEW_CROP_Y 122
#define RECOMP_FILTER_PREVIEW_CROP_W 60
#define RECOMP_FILTER_PREVIEW_CROP_H 52

#ifdef __cplusplus
}
#endif

#endif
