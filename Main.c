#include "Image.h"

int main(void) {
    union Image *a = Image_Load("A.bmp");
    printf("%08X\n", Image_Get(a, 0, 0).u32);
    Image_Save(a, "B.bmp");
    Image_Free(a);
}