#include "bitmap.h"

#include <stdlib.h>
#include <stdio.h>

Bitmap* bitmap_loadFromFile(PlaydateAPI* pd, const char* path)
{
    SDFile* bitmapFile = pd->file->open(path, kFileRead);

    if (bitmapFile)
    {
        Bitmap* newBitmap = (Bitmap*)malloc(sizeof(Bitmap));

        if (newBitmap)
        {
            pd->file->read(bitmapFile, &newBitmap->fileHeader, sizeof(BitmapFileHeader));
            pd->file->read(bitmapFile, &newBitmap->infoHeader, sizeof(BitmapInfoHeader));

            size_t dataSize = newBitmap->infoHeader.biWidth * newBitmap->infoHeader.biHeight * sizeof(BitmapPixel);
            newBitmap->data = (BitmapPixel*)malloc(dataSize);

            if (newBitmap->data)
            {
                pd->file->seek(bitmapFile, newBitmap->fileHeader.bfOffBits, SEEK_SET);

                if (newBitmap->infoHeader.biBitCount == 24)
                {
                    for (int h = 0; h < newBitmap->infoHeader.biHeight; ++h)
                    {
                        for (unsigned int w = 0; w < newBitmap->infoHeader.biWidth; ++w)
                        {
                            // If the height is negative, the bitmap is top-down
                            size_t index = newBitmap->infoHeader.biHeight < 0 ? w + h * newBitmap->infoHeader.biWidth : w + (newBitmap->infoHeader.biHeight - h - 1) * newBitmap->infoHeader.biWidth;

                            pd->file->read(bitmapFile, &newBitmap->data[index], sizeof(BitmapPixel));
                        }

                        //pd->file->read(bitmapFile, newBitmap->data, dataSize);

                        const unsigned char padding = ((newBitmap->infoHeader.biWidth * sizeof(BitmapPixel)) % 4) * sizeof(unsigned char);
                        pd->file->seek(bitmapFile, padding, SEEK_CUR);
                    }
                }
            }
        }

        pd->file->close(bitmapFile);

        return newBitmap;
    }

    return NULL;
}

void bitmap_freeBitmap(Bitmap* bitmap)
{
    free(bitmap->data);
    free(bitmap);
}

BitmapPixel bitmap_getPixel(const Bitmap* bitmap, unsigned int x, unsigned int y)
{
    const unsigned int index = x + y * bitmap->infoHeader.biWidth;
    return bitmap->data[index];
}

BitmapPixel bitmap_lerpPixel(const BitmapPixel* lhs, const BitmapPixel* rhs, float factor)
{
    return (BitmapPixel)
    {
        .r = lhs->r + (rhs->r - lhs->r) * factor,
        .g = lhs->g + (rhs->g - lhs->g) * factor,
        .b = lhs->b + (rhs->b - lhs->b) * factor
    };
}

BitmapPixel bitmap_getPixelLinear(const Bitmap* bitmap, float x, float y)
{
    const unsigned int x0   = floorf(x);
    const unsigned int x1   = x0 >= bitmap->infoHeader.biWidth  - 1 ? x0 : x0 + 1u;
    const unsigned int y0   = floorf(y);
    const unsigned int y1   = y0 >= bitmap->infoHeader.biHeight - 1 ? y0 : y0 + 1u;

    const unsigned int row0 = y0 * bitmap->infoHeader.biWidth;
    const unsigned int row1 = y1 * bitmap->infoHeader.biWidth;

    const BitmapPixel* sample00 = &bitmap->data[x0 + row0];
    const BitmapPixel* sample10 = &bitmap->data[x1 + row0];
    const BitmapPixel* sample01 = &bitmap->data[x0 + row1];
    const BitmapPixel* sample11 = &bitmap->data[x1 + row1];

    const float u               = x - x0;
    const float v               = y - y0;

    const BitmapPixel samplex0 = bitmap_lerpPixel( sample00,  sample10, u);
    const BitmapPixel samplex1 = bitmap_lerpPixel( sample01,  sample11, u);
    const BitmapPixel sample   = bitmap_lerpPixel(&samplex0, &samplex1, v);

    return sample;
}

const struct bitmap_api bitmap = {

    .loadFromFile   = &bitmap_loadFromFile,
    .freeBitmap     = &bitmap_freeBitmap,

    .getPixel       = &bitmap_getPixel,
    .getPixelLinear = &bitmap_getPixelLinear
};