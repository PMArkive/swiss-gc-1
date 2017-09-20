/****************************************************************************
 * libwiigui
 *
 * Tantric 2009
 *
 * gui_imagedata.cpp
 *
 * GUI class definitions
 ***************************************************************************/

#include "gui.h"

/**
 * Constructor for the GuiImageData class.
 */
GuiImageData::GuiImageData(u8 * i, int w, int h, int tplSize)
{
	// TPL open
	if(tplSize > 0) {
		data = (u8*)memalign(32, tplSize);
		memcpy(data, i, tplSize);
		tplFile = (TPLFile*) calloc(1, sizeof(TPLFile));
		TPL_OpenTPLFromMemory(tplFile, (void *)data, tplSize);
		gxTex = (GXTexObj*)calloc(1, sizeof(GXTexObj));
		TPL_GetTexture(tplFile, 0, gxTex);
	}
	else {
		data = i;
	}
	width = w;
	height = h;
}

/**
 * Destructor for the GuiImageData class.
 */
GuiImageData::~GuiImageData()
{
	if(tplFile) {
		TPL_CloseTPLFile(tplFile);
		free(tplFile);
		tplFile = NULL;
	}
	if(gxTex) {
		free(gxTex);
		gxTex = NULL;
	}
	if(data) {
		free(data);
		data = NULL;
	}
}

u8 * GuiImageData::GetImage()
{
	return data;
}

int GuiImageData::GetWidth()
{
	return width;
}

int GuiImageData::GetHeight()
{
	return height;
}

GXTexObj * GuiImageData::GetTex()
{
	return gxTex;
}
