// recomp_bezel_preview.h — thumbnail of a frame (bezel) image as it looks in
// game: the frame stretched to the window, with a sample 4:3 game picture
// drawn over the middle at full height. Shared by the launcher and the
// in-game options menu so both previews match. Pure C, no GL, no decoder.
#ifndef RECOMP_BEZEL_PREVIEW_H
#define RECOMP_BEZEL_PREVIEW_H

#ifdef __cplusplus
extern "C" {
#endif

// bezel: RGBA8 bw x bh (required). sample: RGBA8 sw x sh, or NULL for a black
// picture. The result keeps the frame's aspect and fits inside max_w x max_h.
// Returns a malloc'd opaque RGBA8 buffer (*ow x *oh), or NULL on bad input;
// release it with recomp_bezel_preview_free().
unsigned char* recomp_bezel_preview_compose(const unsigned char* bezel, int bw, int bh,
                                            const unsigned char* sample, int sw, int sh,
                                            int max_w, int max_h, int* ow, int* oh);
void recomp_bezel_preview_free(unsigned char* pixels);

#ifdef __cplusplus
}
#endif

#endif
