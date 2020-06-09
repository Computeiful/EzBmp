#include "Image.h"

// We do not support some of the more strange (and rare) bmp features
// We only support the RGB (3-byte) BMP format

#define new(X) malloc(X) // You can use your own malloc(...) here

// Pack to remove pads
#pragma pack(push, 1)

union Head {
	struct {
		uint16_t id; 
		uint32_t fsize;
		uint32_t unused;
		uint32_t off;
	};
};

union Body {
	struct {
		uint32_t hsize; // Header length (always 40)
		uint32_t w; 
		uint32_t h;
		uint16_t p_unused; // Colour Plane (always 1)
		uint16_t bpp; 
		uint32_t comp;
		uint32_t bsize; 
		uint32_t wppm;
		uint32_t hppm;
		uint32_t c_unused;
		uint32_t i_unused;
	};
};

#pragma pack(pop)

union Image * Image(uint32_t w, uint32_t h) {
    union Image *a = new(sizeof(union Image));
    if(!a) return NULL;

    a->image = new(w * h * 3);
    if(!a->image) return NULL;

    a->w = w;
    a->h = h;
    return a;
}

void Image_Free(union Image *a) {
    if(a) free(a->image);
    free(a);
}

static inline size_t Image_IndexOf(const union Image *a, uint32_t x, uint32_t y) {
    return (((a->w * y) + x) * 3);
}

inline union Pix Image_Get(const union Image *a, uint32_t x, uint32_t y) {
    const size_t index = Image_IndexOf(a, x, y);
    union Pix p;
    p.u32 = 0xFF000000;

    for(size_t i = 0; i < 3; i++) {
        p.u8[i] = a->image[index + i];
    }

    return p;
}

inline void Image_Set(union Image *a, uint32_t x, uint32_t y, union Pix p) {
    const size_t index = Image_IndexOf(a, x, y);

    for(size_t i = 0; i < 3; i++) {
        a->image[index + i] = p.u8[i];
    }
}

union Image * Image_Load(const char *fname) {
    FILE *in = fopen(fname, "rb");
    if(!in) return NULL;
    uint8_t check;

    union Head head; // Header
    check = !(fread(&head, sizeof(union Head), 1, in) == 1);
    if(check) return NULL;

    union Body body; // DIB
    check = !(fread(&body, sizeof(union Body), 1, in) == 1);
    if(check) return NULL;

    // Image
    const uint16_t type = (body.bpp >> 3);
    check = !(type == 3); // We only support RGB BMP
    if(check) return NULL;

    const size_t wsize = (body.w * type);
    const size_t rsize = (((wsize << 3) + 31) >> 5) << 2;
    const uint8_t wpad = (rsize - wsize);

    union Image *a = Image(body.w, body.h);
    if(!a) return NULL;

    uint8_t *image = a->image + (wsize * (a->h - 1));

    for(uint32_t y = 0; y < body.h; y++) {
        check = !(fread(image, type, body.w, in) == body.w);
		if(check) return NULL;
		image -= wsize;
		check = fseek(in, wpad, SEEK_CUR);
		if(check) return NULL;
    }

    fclose(in);
    return a;
}

uint8_t Image_Save(union Image *a, const char *fname) {
    FILE *in = fopen(fname, "wb");
    if(!in) return 1;
    uint8_t check;

    // The "normal" bmp header
    union Head head;
    head.id = 0x4D42;
    head.unused = 0;
    head.off = (14 + 40);

    // Do the header later
    check = fseek(in, sizeof(union Head), SEEK_SET);
    if(check) return 1;

    // DIB
    union Body body;
    body.hsize = 40;
    body.w = (uint32_t) a->w;
	body.h = (uint32_t) a->h;
	body.p_unused = 1;
	body.bpp = (uint16_t) (8 * 3);
	body.comp = 0;
	body.bsize = 0; 
	body.wppm = 3200;
	body.hppm = 3200;
	body.c_unused = 0;
	body.i_unused = 0;

    // Do the DIB later
    check = fseek(in, sizeof(union Body), SEEK_CUR);
    if(check) return 1;

    // Image
    const size_t wsize = (body.w * 3);
	const size_t rsize = (((wsize << 3) + 31) >> 5) << 2;
	const uint8_t wpad = (rsize - wsize);

    body.bsize = (uint32_t) ftell(in); // Used below

	uint8_t *image = a->image + (wsize * (a->h - 1));

	for(uint32_t y = 0; y < a->h; y++) {
		check = !(fwrite(image, 3, a->w, in) == a->w);
		if(check) return 1;
		image -= wsize;
		for(uint8_t i = 0; i < wpad; i++) {
			check = (fputc(0, in) == EOF);
			if(check) return 1;
		}
	}

	for(uint8_t i = 0; i < 2; i++) {
		check = (fputc(0, in) == EOF);
		if(check) return 1;
	}

    // Header can now be done
    head.fsize = (uint32_t) ftell(in); // We now know the BMP length
    fseek(in, 0, SEEK_SET);
    check = !(fwrite(&head, sizeof(union Head), 1, in) == 1);
	if(check) return 1;

    // DIB can now be done
    body.bsize = (ftell(in) - body.bsize); // We now know the RGB data length
    fseek(in, sizeof(union Head), SEEK_SET); // TODO: maybe the seek not needed here
    check = !(fwrite(&body, sizeof(union Body), 1, in) == 1);
	if(check) return 1;

    fclose(in);
    return 0;
}