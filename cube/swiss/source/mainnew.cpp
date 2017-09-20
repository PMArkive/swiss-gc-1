/****************************************************************************
 * Swiss main.cpp
 * emu_kidid 2017
 ***************************************************************************/

#include <stdio.h>
#include <gccore.h>		/*** Wrapper to include common libogc headers ***/
#include <ogcsys.h>		/*** Needed for console support ***/
#include <ogc/color.h>
#include <ogc/exi.h>
#include <ogc/lwp.h>
#include <ogc/usbgecko.h>
#include <ogc/video_types.h>
#include <sdcard/card_cmn.h>
#include <ogc/machine/processor.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>
#include <sdcard/gcsd.h>
#include "main.h"
#include "settings.h"
#include "info.h"
#include "swiss.h"
#include "bba.h"
#include "dvd.h"
#include "wkf.h"
#include "httpd.h"
#include "config.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "devices/deviceHandler.h"
#include "devices/fat/ata.h"
#include "aram/sidestep.h"
#include "devices/filemeta.h"

#include "FreeTypeGX.h"
#include "video.h"
#include "menu.h"
#include "input.h"
#include "filelist.h"
#include "demo.h"

struct SSettings Settings;
int ExitRequested = 0;

void ExitApp()
{
	// TODO
	ShutoffRumble();
	StopGX();
	exit(0);
}

void
DefaultSettings()
{
	//Settings.LoadMethod = METHOD_AUTO;
	//Settings.SaveMethod = METHOD_AUTO;
	//sprintf (Settings.Folder1,"libwiigui/first folder");
	//sprintf (Settings.Folder2,"libwiigui/second folder");
	//sprintf (Settings.Folder3,"libwiigui/third folder");
	//Settings.AutoLoad = 1;
	//Settings.AutoSave = 1;
}
extern "C" {
	extern void swiss_init(void);
};
 
int
main(int argc, char *argv[])
{
	SetupPads(); // Initialize input
	
	InitVideo(); // Initialize video
	InitFreeType((u8*)font_ttf, font_ttf_size); // Initialize font system
	InitGUIThreads(); // Initialize GUI

	DefaultSettings();
	InitMainWindow();
	swiss_init();
	
	
	MainMenu(MENU_SETTINGS);
}
