//
//  main.c
//  Extension
//
//  Created by Dave Hayden on 7/30/14.
//  Copyright (c) 2014 Panic, Inc. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include "pd_api.h"

#include "voxel_terrain.h"

static int update(void* userdata);
const char* fontpath = "/System/Fonts/Asheville-Sans-14-Bold.pft";
LCDFont* font = NULL;

DitherMap* ditherMap;
HeightMap* heightmap;

int frameCounter;

static int cleanup(PlaydateAPI* pd)
{
	voxel_terrain_freeHeightMap(heightmap);
	voxel_terrain_freeDitherMap(ditherMap);

	return 0;
}

#ifdef _WINDLL
__declspec(dllexport)
#endif
int eventHandler(PlaydateAPI* pd, PDSystemEvent event, uint32_t arg)
{
	(void)arg; // arg is currently only used for event = kEventKeyPressed

	if ( event == kEventInit )
	{
		const char* err;
		font = pd->graphics->loadFont(fontpath, &err);
		
		if (font == NULL)
		{
			pd->system->error("%s:%i Couldn't load font %s: %s", __FILE__, __LINE__, fontpath, err);
		}

		// Note: If you set an update callback in the kEventInit handler, the system assumes the game is pure C and doesn't run any Lua code in the game
		pd->system->setUpdateCallback(update, pd);

		// Enable accelerometer
		pd->system->setPeripheralsEnabled(kAccelerometer);

		// Maximum speed
		pd->display->setRefreshRate(50.0f);
	}

	if (event == kEventTerminate)
	{
		cleanup(pd);
	}
	
	return 0;
}

#define TEXT_WIDTH 76
#define TEXT_HEIGHT 16

// State
#define STATE_INIT   0
#define STATE_UPDATE 1
int state = STATE_INIT;

Vector3 viewPosition;
float p = 0.0f;
float y = 0.0f;

static int initUpdate(PlaydateAPI* pd)
{
	Bitmap* ditherBitmap = bitmap.loadFromFile(pd, "images/bayer16tile2.bmp");
	Bitmap* heightBitmap = bitmap.loadFromFile(pd, "images/D21.bmp");
	Bitmap* colourBitmap = bitmap.loadFromFile(pd, "images/C21.bmp");

	heightmap = voxel_terrain_newHeightMap(heightBitmap, colourBitmap);
	ditherMap = voxel_terrain_newDitherMap(ditherBitmap);

	bitmap.freeBitmap(heightBitmap);
	bitmap.freeBitmap(colourBitmap);
	bitmap.freeBitmap(ditherBitmap);

	viewPosition = (Vector3)
	{
		.x = heightmap->width / 2.0f,
		.y = 0.5f,
		.z = heightmap->height / 2.0f
	};

	y = 0.0f;
	p = 0.0f;

	frameCounter = 0;

	return STATE_UPDATE;
}

static int mainUpdate(PlaydateAPI* pd)
{
	pd->graphics->setFont(font);

	PDButtons current, pushed, released;
	pd->system->getButtonState(&current, &pushed, &released);

	// Blit direct to display buffer vs. use bitmap
	{
		// Test voxel rendering
		{
			frameCounter++;

			unsigned int near   = 1;
			unsigned int far    = (heightmap->height) * 2;

			pd->graphics->clear(kColorWhite);

			// Pass NULL instead to draw lines
			uint8_t* data = pd->graphics->getFrame();

			voxel_terrain_draw(data, LCD_ROWSIZE, ditherMap, heightmap, viewPosition, y, p, near, far, 0.25f, 20000.0f, LCD_COLUMNS, LCD_ROWS);
		}

		//uint8_t* data = pd->graphics->getFrame();

		char* buffer2;
		pd->system->formatString(&buffer2, "Position x=%i y=%i z=%i", (int)viewPosition.x, (int)viewPosition.y, (int)viewPosition.z);

		pd->graphics->setDrawMode(kDrawModeFillWhite);
		pd->graphics->drawText(buffer2, strlen(buffer2), kASCIIEncoding, 0, 16);
	}

	// Coarse dt
	float dt = pd->system->getElapsedTime();
	pd->system->resetElapsedTime();

	float xzSpeed = 50.0f;
	float ySpeed  = 1.0f;

	// Input
	if (1)
	{
		const float cosPhi  = cosf(y);
		const float sinPhi  = sinf(y);

		//Vector3 translation = (Vector3)
		//{
		//	-sinPhi* xzSpeed * dt,
		//	0.0f,
		//	cosPhi * xzSpeed * dt
		//};

		if (current & kButtonLeft)
		{
			y += ySpeed * dt;
			//viewPosition.x -= 1 * xzSpeed * dt;
		}
		
		if (current & kButtonRight)
		{
			y -= ySpeed * dt;
			//viewPosition.x += 1 * xzSpeed * dt;
		}

		if (current & kButtonUp)
		{
			viewPosition.x -= sinPhi * xzSpeed * dt;
			viewPosition.z -= cosPhi * xzSpeed * dt;
		}

		if (current & kButtonDown)
		{
			viewPosition.x += sinPhi * xzSpeed * dt;
			viewPosition.z += cosPhi * xzSpeed * dt;
		}

		if (current & kButtonB)
		{
			viewPosition.y -= 1 * ySpeed * dt;

			if (viewPosition.y < 0.0f)
			{
				viewPosition.y = 0.0f;
			}
		}

		if (current & kButtonA)
		{
			viewPosition.y += 1 * ySpeed * dt;
		}

		p += pd->system->getCrankChange() / 360.0f;
	}

	pd->graphics->setDrawMode(kDrawModeFillWhite);
	pd->system->drawFPS(0, 0);

	return STATE_UPDATE;
}

static int update(void* userdata)
{
	PlaydateAPI* pd = userdata;

	switch (state)
	{
	case STATE_INIT:
		state = initUpdate(pd);
		break;
	case STATE_UPDATE:
		state = mainUpdate(pd);
		break;
	};

	return 1;
}
