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

const struct bitmap_api bitmap = {

	.loadFromFile	= &bitmap_loadFromFile,
	.freeBitmap		= &bitmap_freeBitmap,

	.getPixel       = &bitmap_getPixel
};