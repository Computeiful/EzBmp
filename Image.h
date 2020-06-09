#ifndef IMAGE_H
#define IMAGE_H

    #include <stdlib.h>
    #include <stdint.h>
    #include <stddef.h>
    #include <stdio.h>

    union Pix {
        struct {
            uint8_t A; // Alpha - often unused
            uint8_t R, G, B; // Red, Blue and Green
        };
        uint8_t u8[4];
        uint32_t u32;
    };

    union Image {
        struct {
            uint8_t *image;
            size_t w, h;
	    };
    };

    union Image * Image(uint32_t, uint32_t);
    union Pix Image_Get(const union Image *, uint32_t, uint32_t);
    void Image_Set(union Image *, uint32_t, uint32_t, union Pix);
    union Image * Image_Load(const char *); // Returns NULL when you load a malformed or unsupported BMP
    uint8_t Image_Save(union Image *, const char *); // Returns 1 on an IO error
    void Image_Free(union Image *);

#endif