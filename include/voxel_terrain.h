#ifndef VOXEL_TERRAIN_HEADER
#define VOXEL_TERRAIN_HEADER

#include "pd_api.h"
#include "bitmap.h"

#define ABS(A)              (A < 0 ? -A : A)
#define MIN(A, B)           (A < B ? A : B)
#define MAX(A, B)           (A > B ? A : B)
#define CLAMP(A, B, C)      (A < B ? B : (A > C ? C : A))
#define LERP(A, B, F)       (A + (B - A) * F)

typedef struct Vector3
{
    float x;
    float y;
    float z;
} Vector3;

typedef struct DitherMap
{
    uint8_t*        data;
    unsigned int    width;
    unsigned int    height;
} DitherMap;

typedef struct TerrainSample
{
    union
    {
        struct
        {
            uint8_t height;
            uint8_t luminance;
        };

        uint8_t data[2];
    };

} TerrainSample;

typedef struct HeightMap
{
    TerrainSample*  data;
    unsigned int    width;
    unsigned int    height;
} HeightMap;

HeightMap* voxel_terrain_newHeightMap(const Bitmap* heightmap, const Bitmap* colourmap, int scale);
DitherMap* voxel_terrain_newDitherMap(const Bitmap* colourmap);

void voxel_terrain_freeHeightMap(HeightMap* heightmap);
void voxel_terrain_freeDitherMap(DitherMap* heightmap);
void voxel_terrain_draw(
    uint8_t* bitmapData, 
    const uint16_t rowBytes,
    const DitherMap* dithermap, 
    const HeightMap* heigtmap, 
    const Vector3* position, 
    const float yaw, 
    const float pitch, 
    const float roll, 
    const uint16_t near,
    const uint16_t far,
    const float scaleXZ, 
    float scale, 
    const int width, 
    const int height);

#endif