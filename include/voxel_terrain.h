#ifndef VOXEL_TERRAIN_HEADER
#define VOXEL_TERRAIN_HEADER

#include "pd_api.h"
#include "bitmap.h"

typedef struct Vector3
{
	float x;
	float y;
	float z;
} Vector3;

typedef struct DitherMap
{
	uint8_t*		data;
	unsigned int	width;
	unsigned int	height;
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
	TerrainSample*	data;
	unsigned int	width;
	unsigned int	height;
} HeightMap;

HeightMap* voxel_terrain_newHeightMap(const Bitmap* heightmap, const Bitmap* colourmap);
DitherMap* voxel_terrain_newDitherMap(const Bitmap* colourmap);

void voxel_terrain_freeHeightMap(HeightMap* heightmap);
void voxel_terrain_freeDitherMap(DitherMap* heightmap);
void voxel_terrain_draw(uint8_t* bitmapData, size_t rowBytes, const DitherMap* dithermap, const HeightMap* heigtmap, Vector3 position, float yaw, float pitch, unsigned int near, unsigned int far, float scaleXZ, float scale, int width, int height);

#endif