#ifndef BITMAP_HEADER
#define BITMAP_HEADER

#include "pd_api.h"

#pragma pack(push, 1)

typedef struct BitmapFileHeader
{
    unsigned short  bfType;
    unsigned int    bfSize;
    unsigned short  bfReserved1;
    unsigned short  bfReserved2;
    unsigned int    bfOffBits;
} BitmapFileHeader;

typedef struct BitmapInfoHeader
{
    unsigned int    biSize;
    unsigned int    biWidth;
    int             biHeight;
    unsigned short  biPlanes;
    unsigned short  biBitCount;
    unsigned int    biCompression;
    unsigned int    biSizeImage;
    unsigned int    biXPelsPerMeter;
    unsigned int    biYPelsPerMeter;
    unsigned int    biClrUsed;
    unsigned int    biClrImportant;
} BitmapInfoHeader;

typedef struct RGBQuad
{
    unsigned char   rgbBlue;
    unsigned char   rgbGreen;
    unsigned char   rgbRed;
    unsigned char   rgbReserved;
} RGBQuad;

typedef struct BitmapPixel
{
    unsigned char b;
    unsigned char g;
    unsigned char r;
} BitmapPixel;

typedef struct Bitmap
{
    BitmapFileHeader fileHeader;
    BitmapInfoHeader infoHeader;
    BitmapPixel*     data;
} Bitmap;

struct bitmap_api
{
    Bitmap* (*loadFromFile)(PlaydateAPI* pd, const char* path);
    BitmapPixel (*getPixel)(const Bitmap* bitmap, unsigned int x, unsigned int y);

    void (*freeBitmap)(Bitmap*);
};

// Convenience
extern const struct bitmap_api bitmap;

#pragma pack(pop)

#endif