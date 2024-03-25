#include "voxel_terrain.h"

#include <stdlib.h>
#include <stdio.h>

#define MIN(A, B)           (A < B ? A : B)
#define MAX(A, B)           (A > B ? A : B)
#define CLAMP(A, B, C)      (A < B ? B : (A > C ? C : A))

HeightMap* voxel_terrain_newHeightMap(const Bitmap* heightmap, const Bitmap* colourMap)
{
    HeightMap* newHeightmap = (HeightMap*)malloc(sizeof(HeightMap));

    if (newHeightmap)
    {
        newHeightmap->width   = heightmap->infoHeader.biWidth;
        newHeightmap->height  = heightmap->infoHeader.biHeight;
        newHeightmap->data    = (TerrainSample*)malloc(sizeof(TerrainSample) * newHeightmap->width * newHeightmap->height);

        if (newHeightmap->data)
        {
            for (unsigned int y = 0; y < newHeightmap->height; ++y)
            {
                for (unsigned int x = 0; x < newHeightmap->width; ++x)
                {
                    // Index
                    const unsigned int index = x + y * newHeightmap->height;

                    // Height
                    {
                        const BitmapPixel rgb   = bitmap.getPixel(heightmap, x, y);
                        const float height      = (rgb.r / 255.0f);
        
                        newHeightmap->data[index].height = (uint8_t)roundf(height * 255.0f);
                    }

                    // Colour
                    {
                        const BitmapPixel rgb = bitmap.getPixel(colourMap, x, y);
                        const float luminance = powf(rgb.r / 255.0f, 2.2f) * 0.2126f + powf(rgb.g / 255.0f, 2.2f) * 0.7152f + powf(rgb.b / 255.0f, 2.2f) * 0.0722f;

                        newHeightmap->data[index].luminance = (uint8_t)roundf(luminance * 255.0f);
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

TerrainSample voxel_terrain_lerpSample(const TerrainSample* lhs, const TerrainSample* rhs, float factor)
{
    return (TerrainSample)
    {
        .height    = lhs->height    + (rhs->height    - lhs->height)    * factor,
        .luminance = lhs->luminance + (rhs->luminance - lhs->luminance) * factor
    };
}

TerrainSample voxel_terrain_getSampleLinear(const HeightMap* heightmap, float x, float y)
{
    int x0 = floorf(x);
    int x1 = x0 + 1;
    int y0 = floorf(y);
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

LCDSolidColor voxel_terrain_dither(const DitherMap* dithermap, unsigned int x, unsigned y, uint8_t luminance)
{
    const int xDither               = x % dithermap->width;
    const int yDither               = y % dithermap->height;
    const unsigned char ditherMask = dithermap->data[xDither + yDither * dithermap->width];

    return (LCDSolidColor)(luminance >= ditherMask);
}

static inline void voxel_terrain_setPixel(uint8_t* bitmapData, uint16_t rowBytes, unsigned int x, unsigned int y, uint8_t value)
{
    // Based off : https://graphics.stanford.edu/~seander/bithacks.html#ConditionalSetOrClearBitsWithoutBranching
    const uint16_t dstIndex = (x >> 3) + (y * rowBytes);
    const uint8_t mask      = (1 << (uint8_t)(7 - (x % 8)));
    bitmapData[dstIndex]    = (bitmapData[dstIndex] & ~mask) | (-value & mask);
}

static inline void voxel_terrain_drawDither(uint8_t* bitmapData, uint16_t rowBytes, const DitherMap* dithermap, unsigned int x, unsigned int y, uint8_t luminance)
{
    const uint8_t xDither       = x % (uint8_t)dithermap->width;
    const uint8_t yDither       = y % (uint8_t)dithermap->height;
    const uint8_t srcIndex      = xDither + yDither * (uint8_t)dithermap->width;
    const uint8_t ditherMask    = dithermap->data[srcIndex];

    voxel_terrain_setPixel(bitmapData, rowBytes, x, y, luminance >= ditherMask);
}

// Number of samples - adjust to balance quality vs performance
#define LINE_WIDTH  (8u)
#define DEPTH       (2 * 96u)

// Based off : https://github.com/s-macke/VoxelSpace
void voxel_terrain_draw(uint8_t* bitmapData, size_t rowBytes, const DitherMap* dithermap, const HeightMap* heightmap, Vector3 position, float yaw, float pitch, unsigned int near, unsigned int far, float scaleXZ, float scale, int width, int height)
{
    // Precompute horizon & cos
    const int horizon               = (int)roundf((1.0f + pitch) * (0.5f * height));
    const float cosPhi              = cosf(yaw);
    const float sinPhi              = sinf(yaw);

    // Precomputed dx/dz factors
    const float dxFactor            = ( 2.0f * cosPhi) / (float)width;
    const float dzFactor            = (-2.0f * sinPhi) / (float)width;
    const float dz                  = 1.0f / DEPTH;

    // Precompute z values, scales, offsets and factors
    float zScales[DEPTH];
    int   zOffsets[DEPTH];
    float zFades[DEPTH];
    float zPositionX[DEPTH];
    float zPositionZ[DEPTH];
    float zDX[DEPTH];
    float zDZ[DEPTH];
    int   zMaxHeight[DEPTH];
    {
        for (unsigned int index = 0; index < DEPTH; ++index)
        {
            const float zFactor = dz * index;
            const float zValue  = near + (far - near) * (zFactor * zFactor);

            zScales[index]      = scale / (zValue * 255.0f);
            zOffsets[index]     = (int)(horizon - zScales[index] * (position.y * 255.0f));
            zFades[index]       = 1.0f - powf(zFactor, 4.0f);

            zPositionX[index]   = scaleXZ * ((-cosPhi * zValue - sinPhi * zValue) + position.x);
            zPositionZ[index]   = scaleXZ * (( sinPhi * zValue - cosPhi * zValue) + position.z);

            zDX[index]          = scaleXZ * dxFactor * zValue;
            zDZ[index]          = scaleXZ * dzFactor * zValue;

            zMaxHeight[index]   = CLAMP(height - (int)(255 * zScales[index] + zOffsets[index]), 0, height - 1);
        }
    }

    // From left to right
    for (unsigned int x = 0u; x < (unsigned int)width; x += LINE_WIDTH)
    {
        // Start off at min height
        uint8_t minHeight = height;

        // Scan front to back + skip early if the theoretical max is occluded
        for (unsigned int z = 0u; z < DEPTH && (zMaxHeight[z] <= minHeight); ++z)
        {
            // Sample coordinates
            const float sampleX = x * zDX[z] + zPositionX[z];
            const float sampleZ = x * zDZ[z] + zPositionZ[z];

            //const TerrainSample sample = voxel_terrain_getSample(heightmap, current.x, current.z);
            const TerrainSample sample = voxel_terrain_getSampleLinear(heightmap, sampleX, sampleZ);
            //const TerrainSample sample = zFactors[z] > 0.75f ? voxel_terrain_getSample(heightmap, sampleX, sampleZ) : voxel_terrain_getSampleLinear(heightmap, sampleX, sampleZ);
                
            // Fade luminance
            const unsigned char fadeLuminance = 255u;
            const unsigned char luminance     = fadeLuminance + (sample.luminance - fadeLuminance) * zFades[z];

            if (luminance != fadeLuminance)
            {
                const int heightOnScreen = (int)(sample.height * zScales[z] + zOffsets[z]);

                if (heightOnScreen > 0)
                {
                    // Compute upper and lower bounds of the vertical line
                    const unsigned int top = CLAMP(height - heightOnScreen, 0, height - 1);
                    const unsigned int bot = CLAMP(MIN(minHeight, height), top, height);

                    // Draw rectangle with dithering
                    for (unsigned int u = 0u; u < LINE_WIDTH; ++u)
                    {
                        for (unsigned int y = top; y < bot; ++y)
                        {
                            voxel_terrain_drawDither(bitmapData, rowBytes, dithermap, x + u, y, luminance);
                        }
                    }

                    minHeight = MIN(minHeight, top);
                }
            }
        }
    }
}