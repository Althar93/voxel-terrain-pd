#include "voxel_terrain.h"

#include <stdlib.h>
#include <stdio.h>

HeightMap* voxel_terrain_newHeightMap(const Bitmap* heightmap, const Bitmap* colourMap, int scale)
{
    HeightMap* newHeightmap = (HeightMap*)malloc(sizeof(HeightMap));

    const float gamma = 1.0f;

    if (newHeightmap)
    {
        newHeightmap->width   = scale * heightmap->infoHeader.biWidth;
        newHeightmap->height  = scale * heightmap->infoHeader.biHeight;
        newHeightmap->data    = (TerrainSample*)malloc(sizeof(TerrainSample) * newHeightmap->width * newHeightmap->height);

        if (newHeightmap->data)
        {
            for (unsigned int y = 0; y < newHeightmap->height; ++y)
            {
                for (unsigned int x = 0; x < newHeightmap->width; ++x)
                {
                    // Index
                    const unsigned int dstIndex = x + y * newHeightmap->height;

                    const float xSource = (x / (float)newHeightmap->width)  * heightmap->infoHeader.biWidth;
                    const float ySource = (y / (float)newHeightmap->height) * heightmap->infoHeader.biHeight;

                    // Height
                    {
                        const BitmapPixel rgb   = bitmap.getPixelLinear(heightmap, xSource, ySource); 
                        const float height      = (rgb.r / 255.0f);
        
                        newHeightmap->data[dstIndex].height = (uint8_t)roundf(height * 255.0f);
                    }

                    // Colour
                    {
                        const BitmapPixel rgb = bitmap.getPixelLinear(colourMap, xSource, ySource);
                        const float luminance = powf(rgb.r / 255.0f, gamma) * 0.2126f + powf(rgb.g / 255.0f, gamma) * 0.7152f + powf(rgb.b / 255.0f, gamma) * 0.0722f;

                        newHeightmap->data[dstIndex].luminance = (uint8_t)roundf(luminance * 255.0f);
                    }
                }
            }
        }
    }

    return newHeightmap;
}

void voxel_terrain_freeHeightMap(HeightMap* heightmap)
{
    free(heightmap->data);
    free(heightmap);
}

DitherMap* voxel_terrain_newDitherMap(const Bitmap* colourmap)
{
    DitherMap* newDithermap = (DitherMap*)malloc(sizeof(DitherMap));

    if (newDithermap)
    {
        newDithermap->width  = colourmap->infoHeader.biWidth;
        newDithermap->height = colourmap->infoHeader.biHeight;
        newDithermap->data   = (uint8_t*)malloc(sizeof(uint8_t) * newDithermap->width * newDithermap->height);

        if (newDithermap->data)
        {
            for (unsigned int y = 0; y < newDithermap->height; ++y)
            {
                for (unsigned int x = 0; x < newDithermap->width; ++x)
                {
                    // Index
                    const unsigned int index    = x + y * newDithermap->height;
                    newDithermap->data[index]   = bitmap.getPixel(colourmap, x, y).r;
                }
            }
        }
    }

    return newDithermap;
}

void voxel_terrain_freeDitherMap(DitherMap* dithermap)
{
    free(dithermap->data);
    free(dithermap);
}

TerrainSample voxel_terrain_getSample(const HeightMap* heightmap, int x, int y)
{
    // Wrap around
    x = x % heightmap->width;
    y = y % heightmap->height;

    const unsigned int index = x + y * heightmap->height;
    return heightmap->data[index];
}

TerrainSample voxel_terrain_lerpSample(const TerrainSample* lhs, const TerrainSample* rhs, const float factor)
{
    return (TerrainSample)
    {
        .height    = lhs->height    + (uint8_t)((rhs->height    - lhs->height)    * factor),
        .luminance = lhs->luminance + (uint8_t)((rhs->luminance - lhs->luminance) * factor)
    };
}

TerrainSample voxel_terrain_getSampleLinear(const HeightMap* heightmap, const float x, const float y)
{
    int x0 = (int)floorf(x);
    int x1 = x0 + 1;
    int y0 = (int)floorf(y);
    int y1 = y0 + 1;

    const float u = x - x0;
    const float v = y - y0;

    // Wrap around
    x0 = x0 % heightmap->width;
    y0 = y0 % heightmap->height;
    x1 = x1 % heightmap->width;
    y1 = y1 % heightmap->height;

    const unsigned int row0         = y0 * heightmap->width;
    const unsigned int row1         = y1 * heightmap->width;

    const TerrainSample* sample00   = &heightmap->data[x0 + row0];
    const TerrainSample* sample10   = &heightmap->data[x1 + row0];
    const TerrainSample* sample01   = &heightmap->data[x0 + row1];
    const TerrainSample* sample11   = &heightmap->data[x1 + row1];

    const TerrainSample samplex0    = voxel_terrain_lerpSample( sample00,  sample10, u);
    const TerrainSample samplex1    = voxel_terrain_lerpSample( sample01,  sample11, u);
    const TerrainSample sample      = voxel_terrain_lerpSample(&samplex0, &samplex1, v);

    return sample;
}

LCDSolidColor voxel_terrain_dither(const DitherMap* dithermap, const unsigned int x, const unsigned int y, const uint8_t luminance)
{
    const int xDither               = x % dithermap->width;
    const int yDither               = y % dithermap->height;
    const unsigned char ditherMask  = dithermap->data[xDither + yDither * dithermap->width];

    return (LCDSolidColor)(luminance >= ditherMask);
}

static inline void voxel_terrain_setPixel(uint8_t* bitmapData, const uint16_t rowBytes, const unsigned int x, const unsigned int y, const uint8_t value)
{
    // Based off : https://graphics.stanford.edu/~seander/bithacks.html#ConditionalSetOrClearBitsWithoutBranching
    const uint16_t dstIndex = (x >> 3) + (y * rowBytes);
    const uint8_t mask      = (1 << (uint8_t)(7 - (x % 8)));
    bitmapData[dstIndex]    = (bitmapData[dstIndex] & ~mask) | (-value & mask);
}

static inline void voxel_terrain_drawDither(uint8_t* bitmapData, const uint16_t rowBytes, const DitherMap* dithermap, const unsigned int x, const unsigned int y, const uint8_t luminance)
{
    const uint8_t xDither       = x % 32u;                  // (uint8_t)dithermap->width;
    const uint8_t yDither       = y % 32u;                  // (uint8_t)dithermap->height;
    const uint8_t srcIndex      = xDither + yDither * 32u;  // (uint8_t)dithermap->width;
    const uint8_t ditherMask    = dithermap->data[srcIndex];

    voxel_terrain_setPixel(bitmapData, rowBytes, x, y, luminance >= ditherMask);
}

// Number of samples - adjust to balance quality vs performance
#define LINE_WIDTH  (8u)
#define DEPTH       (2 * 96u)

#define ROLL_ENABLED (1)

// Based off : https://github.com/s-macke/VoxelSpace
void voxel_terrain_draw(
    uint8_t* bitmapData, 
    const uint16_t rowBytes,
    const DitherMap* dithermap, 
    const HeightMap* heightmap, 
    const Vector3* position, 
    const float yaw, 
    const float pitch, 
    const float roll, 
    const uint16_t near,
    const uint16_t far,
    const float scaleXZ,
    const float scale,
    const int width,
    const int height)
{
    // Precompute horizon & cos
    const int horizon               = (int)roundf((1.0f + pitch) * (0.5f * height));
    const float cosPhi              = cosf(yaw);
    const float sinPhi              = sinf(yaw);

    // Precomputed dx/dz factors
    const float dxFactor            = ( 2.0f * cosPhi) / (float)width;
    const float dzFactor            = (-2.0f * sinPhi) / (float)width;
    const float dz                  = 1.0f / DEPTH;

    // Precompute half width for roll
    const float halfWidth           = width / 2.0f;
    const float rcpHalfWidth        = 1.0f / halfWidth;
    const int positionY             = (position->y * 255);

    // Precompute z values, scales, offsets and factors
    float zScales[DEPTH];
    int   zOffsets[DEPTH];
    uint8_t zFades[DEPTH];
    float zPositionX[DEPTH];
    float zPositionZ[DEPTH];
    float zDX[DEPTH];
    float zDZ[DEPTH];
    int   zMaxHeight[DEPTH];

    for (unsigned int z = 0; z < DEPTH; ++z)
    {
        const float zFactor = dz * z;
        const float zValue  = near + (far - near) * (zFactor * zFactor);

        zScales[z]      = scale / (zValue * 255.0f);
        zOffsets[z]     = (int)(horizon - zScales[z] * positionY);
        zFades[z]       = (uint8_t)(255 * (1.0f - powf(zFactor, 8.0f)));

        zPositionX[z]   = scaleXZ * ((-cosPhi * zValue - sinPhi * zValue) + position->x);
        zPositionZ[z]   = scaleXZ * (( sinPhi * zValue - cosPhi * zValue) + position->z);

        zDX[z]          = scaleXZ * dxFactor * zValue;
        zDZ[z]          = scaleXZ * dzFactor * zValue;

        zMaxHeight[z]   = CLAMP(height - (int)(255 * zScales[z] + zOffsets[z]), 0, height - 1);

        // When roll is enabled, we need the offset to be relative to '0'
        #if ROLL_ENABLED
            zOffsets[z] -= horizon;
        #endif
    }

    // From left to right
    for (unsigned int x = 0u; x < (unsigned int)width; x += LINE_WIDTH)
    {
        // Start off at min height
        uint8_t minHeight = height;

        // Scan front to back + skip early if the theoretical max is occluded
        for (unsigned int z = 0u; z < DEPTH && (zMaxHeight[z] < minHeight) && (minHeight > 0) ; ++z)
        {
            // Sample coordinates
            const int sampleX = (int)(x * zDX[z] + zPositionX[z]);
            const int sampleZ = (int)(x * zDZ[z] + zPositionZ[z]);

            // Sample terrain
            const TerrainSample sample = voxel_terrain_getSample(heightmap, sampleX, sampleZ);

            // Fade luminance
            const uint8_t fadeLuminance = 255u;
            const uint8_t luminance     = (uint8_t)((fadeLuminance * (255u - zFades[z]) + sample.luminance * zFades[z]) / 255u);

            if (luminance != fadeLuminance)
            {
                #if ROLL_ENABLED
                    const float relativeX       = (x - halfWidth) * rcpHalfWidth;
                    const int shiftedHorizon    = (int)(relativeX * roll + horizon);
                    const int zRollOffset       = (int)(shiftedHorizon + zOffsets[z]);
                    const int heightOnScreen    = (int)(sample.height * zScales[z] + zRollOffset);
                #else
                    const int heightOnScreen    = (int)(sample.height * zScales[z] + zOffsets[z]);
                #endif

                if (heightOnScreen > 0)
                {
                    // Compute upper and lower bounds of the vertical line
                    const uint8_t top = CLAMP(height - heightOnScreen, 0, height - 1);
                    const uint8_t bot = CLAMP(MIN(minHeight, height), top, height);

                    // Draw rectangle with dithering
                    for (uint8_t y = top; y < bot; ++y)
                    {
                        // Compute the offset for this row
                        const uint16_t rowOffset = y * rowBytes;

                        for (uint16_t u = 0u; u < LINE_WIDTH; ++u)
                        {
                            // Optimisation : pass '0' to the rowBytes since we have already offset the bitmap based on the active row
                            voxel_terrain_drawDither(bitmapData + rowOffset, 0u, dithermap, x + u, y, luminance);
                        }
                    }

                    minHeight = MIN(minHeight, top);
                }
            }
        }
    }
}