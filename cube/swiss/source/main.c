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
#include "exi.h"
#include "httpd.h"
#include "config.h"
#include "gui/FrameBufferMagic.h"
#include "gui/IPLFontWrite.h"
#include "devices/deviceHandler.h"
#include "devices/fat/ata.h"
#include "aram/sidestep.h"
#include "devices/filemeta.h"

#define DEFAULT_FIFO_SIZE    (256*1024)//(64*1024) minimum

extern void __libogc_exit(int status);

GXRModeObj *vmode = NULL;				//Graphics Mode Object
void *gp_fifo = NULL;
u32 *xfb[2] = { NULL, NULL };   //Framebuffers
int whichfb = 0;       		 	    //Frame buffer toggle
u8 driveVersion[8];
file_handle* allFiles;   		//all the files in the current dir
int curMenuLocation = ON_FILLIST; //where are we on the screen?
int files = 0;                  //number of files in a directory
int curMenuSelection = 0;	      //menu selection
int curSelection = 0;		        //game selection
int needsDeviceChange = 0;
int needsRefresh = 0;
SwissSettings swissSettings;
char *knownExtensions[] = {".dol\0", ".iso\0", ".gcm\0", ".mp3\0", ".fzn\0", ".gci\0"};

int endsWith(char *str, char *end) {
	if(strlen(str) < strlen(end))
		return 0;
	int i;
	for(i = 0; i < strlen(end); i++)
		if(tolower((int)str[strlen(str)-i]) != tolower((int)end[strlen(end)-i]))
			return 0;
	return 1;
}

bool checkExtension(char *filename) {
	if(!swissSettings.hideUnknownFileTypes)
		return true;
	int i;
	for(i = 0; i < sizeof(knownExtensions)/sizeof(char*); i++) {
		if(endsWith(filename, knownExtensions[i])) {
			return true;
		}
	}
	return false;
}


char* getVideoStr(GXRModeObj *vmode) {
	switch(vmode->viTVMode) {
		case VI_TVMODE_NTSC_INT:     return NtscIntStr;    
		case VI_TVMODE_NTSC_DS:      return NtscDsStr;     
		case VI_TVMODE_NTSC_PROG:    return NtscProgStr;   
		case VI_TVMODE_PAL_INT:      return PalIntStr;     
		case VI_TVMODE_PAL_DS:       return PalDsStr;      
		case VI_TVMODE_PAL_PROG:     return PalProgStr;    
		case VI_TVMODE_MPAL_INT:     return MpalIntStr;    
		case VI_TVMODE_MPAL_DS:      return MpalDsStr;     
		case VI_TVMODE_MPAL_PROG:    return MpalProgStr;   
		case VI_TVMODE_EURGB60_INT:  return Eurgb60IntStr; 
		case VI_TVMODE_EURGB60_DS:   return Eurgb60DsStr;  
		case VI_TVMODE_EURGB60_PROG: return Eurgb60ProgStr;
		default:                     return UnkStr;
	}
}

/* get the preferred video mode */
GXRModeObj* swiss_get_videomode()
{
	// Swiss video mode force
	GXRModeObj *forcedMode = getModeFromSwissSetting(swissSettings.uiVMode);
	
	if(forcedMode != NULL) {
		return forcedMode;
	}
	
	__SYS_ReadROM(IPLInfo,256,0);	// Read IPL tag

	// Wii has no IPL tags for "PAL" so let libOGC figure out the video mode
	if(!is_gamecube()) {
		return VIDEO_GetPreferredMode(NULL); //Last mode used
	}
	else {	// Gamecube, determine based on IPL
		int retPAD = 0, retCnt = 10000;
		while(retPAD <= 0 && retCnt >= 0) { retPAD = PAD_ScanPads(); usleep(100); retCnt--; }
		// L Trigger held down ignores the fact that there's a component cable plugged in.
		if(VIDEO_HaveComponentCable() && !(PAD_ButtonsDown(0) & PAD_TRIGGER_L)) {
			if(strstr(IPLInfo,"MPAL")!=NULL) {
				swissSettings.sramVideo = 2;
				return &TVMpal480Prog; //Progressive 480p
			}
			else if((strstr(IPLInfo,"PAL")!=NULL)) {
				swissSettings.sramVideo = 1;
				return &TVPal576ProgScale; //Progressive 576p
			}
			else {
				swissSettings.sramVideo = 0;
				return &TVNtsc480Prog; //Progressive 480p
			}
		}
		else {
			//try to use the IPL region
			if(strstr(IPLInfo,"MPAL")!=NULL) {
				swissSettings.sramVideo = 2;
				return &TVMpal480IntDf;        //PAL-M
			}
			else if(strstr(IPLInfo,"PAL")!=NULL) {
				swissSettings.sramVideo = 1;
				return &TVPal576IntDfScale;         //PAL
			}
			else {
				swissSettings.sramVideo = 0;
				return &TVNtsc480IntDf;        //NTSC
			}
		}
	}
}

void load_config() {

	// Try to open up the config .ini in case it hasn't been opened already
	if(config_init()) {
		sprintf(txtbuffer,"Loaded %i entries from the config file",config_get_count());
		print_gecko("%s\r\n",txtbuffer);
		memcpy(&swissSettings, config_get_swiss_settings(), sizeof(SwissSettings));
	}
}

int comp(const void *a1, const void *b1)
{
	const file_handle* a = a1;
	const file_handle* b = b1;
	
	if(!a && b) return 1;
	if(a && !b) return -1;
	if(!a && !b) return 0;
	
	if((devices[DEVICE_CUR] == &__device_dvd) && ((dvdDiscTypeInt == GAMECUBE_DISC) || (dvdDiscTypeInt == MULTIDISC_DISC)))
	{
		if(a->size == DISC_SIZE && a->fileBase == 0)
			return -1;
		if(b->size == DISC_SIZE && b->fileBase == 0)
			return 1;
	}
	
	if(a->fileAttrib == IS_DIR && b->fileAttrib == IS_FILE)
		return -1;
	if(a->fileAttrib == IS_FILE && b->fileAttrib == IS_DIR)
		return 1;

	return strcasecmp(a->name, b->name);
}

void sortFiles(file_handle* dir, int num_files)
{
	if(num_files > 0) {
		qsort(&dir[0],num_files,sizeof(file_handle),comp);
	}
}

void free_files() {
	if(allFiles) {
		int i;
		for(i = 0; i < files; i++) {
			if(allFiles[i].meta) {
				if(allFiles[i].meta->banner) {
					free(allFiles[i].meta->banner);
					allFiles[i].meta->banner = NULL;
				}
				memset(allFiles[i].meta, 0, sizeof(file_meta));
				meta_free(allFiles[i].meta);
				allFiles[i].meta = NULL;
			}
		}
		free(allFiles);
		allFiles = NULL;
		files = 0;
	}
}

void scan_files() {
	free_files();
	// Read the directory/device TOC
	if(allFiles){ free(allFiles); allFiles = NULL; }
	print_gecko("Reading directory: %s\r\n",curFile.name);
	files = devices[DEVICE_CUR]->readDir(&curFile, &allFiles, -1);
	memcpy(&curDir, &curFile, sizeof(file_handle));
	sortFiles(allFiles, files);
	print_gecko("Found %i entries\r\n",files);
}

// Keep this list sorted
char *autoboot_dols[] = { "/boot.dol", "/boot2.dol" };
void load_auto_dol() {
	u8 rev_buf[sizeof(GITREVISION) - 1]; // Don't include the NUL termination in the comparison

	memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
	scan_files();
	for (int i = 0; i < files; i++) {
		for (int f = 0; f < (sizeof(autoboot_dols) / sizeof(char *)); f++) {
			if (endsWith(allFiles[i].name, autoboot_dols[f])) {
				// Official Swiss releases have the short commit hash appended to
				// the end of the DOL, compare it to our own to make sure we don't
				// bootloop the same version
				devices[DEVICE_CUR]->seekFile(&allFiles[i],
						allFiles[i].size - sizeof(rev_buf),
						DEVICE_HANDLER_SEEK_SET);
				devices[DEVICE_CUR]->readFile(&allFiles[i], rev_buf, sizeof(rev_buf));
				if (memcmp(GITREVISION, rev_buf, sizeof(rev_buf)) != 0) {
					// Emulate some of the menu's behavior to satisfy boot_dol
					curSelection = i;
					memcpy(&curFile, &allFiles[i], sizeof(file_handle));
					boot_dol();
				}

				// If we've made it this far, we've already found an autoboot DOL,
				// the first one (boot.dol) is not cancellable, but the rest of the
				// list is
				if (PAD_ButtonsHeld(0) & PAD_BUTTON_Y) {
					return;
				}
			}
		}
	}
}

void main_loop()
{ 
	
	while(PAD_ButtonsHeld(0) & PAD_BUTTON_A) { VIDEO_WaitVSync (); }
	// We don't care if a subsequent device is "default"
	if(needsDeviceChange) {
		free_files();
		if(devices[DEVICE_CUR]) {
			devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
		}
		devices[DEVICE_CUR] = NULL;
		needsDeviceChange = 0;
		needsRefresh = 1;
		curMenuLocation = ON_FILLIST;
		select_device(DEVICE_CUR);
		if(devices[DEVICE_CUR] != NULL) {
			memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
		}
		curMenuLocation = ON_OPTIONS;
	}
	if(devices[DEVICE_CUR] != NULL) {
		DrawFrameStart();
		DrawMessageBox(D_INFO,"Setting up device");
		DrawFrameFinish();
		// If the user selected a device, make sure it's ready before we browse the filesystem
		devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
		sdgecko_setSpeed(EXI_SPEED32MHZ);
		if(!devices[DEVICE_CUR]->init( devices[DEVICE_CUR]->initial )) {
			needsDeviceChange = 1;
			deviceHandler_setDeviceAvailable(devices[DEVICE_CUR], false);
			return;
		}
		deviceHandler_setDeviceAvailable(devices[DEVICE_CUR], true);	
	}
	else {
		curMenuLocation=ON_OPTIONS;
	}

	while(1) {
		if(devices[DEVICE_CUR] != NULL && needsRefresh) {
			curMenuLocation=ON_OPTIONS;
			curSelection=0; curMenuSelection=0;
			scan_files();
			if(files<1) { devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial); needsDeviceChange=1; break;}
			needsRefresh = 0;
			curMenuLocation=ON_FILLIST;
		}
		while(PAD_ButtonsHeld(0) & PAD_BUTTON_A) { VIDEO_WaitVSync (); }
		drawFiles(&allFiles, files);

		u16 btns = PAD_ButtonsHeld(0);
		if(curMenuLocation==ON_OPTIONS) {
			if(btns & PAD_BUTTON_LEFT){	curMenuSelection = (--curMenuSelection < 0) ? (MENU_MAX-1) : curMenuSelection;}
			else if(btns & PAD_BUTTON_RIGHT){curMenuSelection = (curMenuSelection + 1) % MENU_MAX;	}
		}
		if(devices[DEVICE_CUR] != NULL && ((btns & PAD_BUTTON_B)||(curMenuLocation==ON_FILLIST)))	{
			while(PAD_ButtonsHeld(0) & PAD_BUTTON_B){ VIDEO_WaitVSync (); }
			curMenuLocation=ON_FILLIST;
			renderFileBrowser(&allFiles, files);
		}
		else if(btns & PAD_BUTTON_A) {
			//handle menu event
			switch(curMenuSelection) {
				case 0:		// Device change
					needsDeviceChange = 1;  //Change from SD->DVD or vice versa
					break;
				case 1:		// Settings
					show_settings(NULL, NULL);
					break;
				case 2:		// Credits
					show_info();
					break;
				case 3:
					if(devices[DEVICE_CUR] != NULL) {
						memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
						if(devices[DEVICE_CUR] == &__device_wkf) { 
							wkfReinit(); devices[DEVICE_CUR]->deinit(devices[DEVICE_CUR]->initial);
						}
					}
					needsRefresh=1;
					break;
				case 4:
					__libogc_exit(0);
					break;
			}
			
		}
		while (!(!(PAD_ButtonsHeld(0) & PAD_BUTTON_B) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_A) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_RIGHT) && !(PAD_ButtonsHeld(0) & PAD_BUTTON_LEFT))) {
			VIDEO_WaitVSync();
		}
		if(needsDeviceChange) {
			break;
		}
	}
}

#include "menu.h"
/****************************************************************************
* Main
****************************************************************************/
void swiss_init(void) 
{
	*(volatile unsigned long*)0xcc00643c = 0x00000000; //allow 32mhz exi bus
	
	// Disable IPL modchips to allow access to IPL ROM fonts
	ipl_set_config(6); 
	usleep(1000); //wait for modchip to disable (overkill)
	DVD_Init();

	// Register all devices supported (order matters for boot devices)
	int i = 0;
	for(i = 0; i < MAX_DEVICES; i++)
		allDevices[i] = NULL;
	i = 0;
	allDevices[i++] = &__device_wkf;
	allDevices[i++] = &__device_wode;
	allDevices[i++] = &__device_sd_a;
	allDevices[i++] = &__device_sd_b;
	allDevices[i++] = &__device_card_a;
	allDevices[i++] = &__device_card_b;
	allDevices[i++] = &__device_dvd;
	allDevices[i++] = &__device_ide_a;
	allDevices[i++] = &__device_ide_b;
	allDevices[i++] = &__device_qoob;
	allDevices[i++] = &__device_smb;
	allDevices[i++] = &__device_sys;
	allDevices[i++] = &__device_usbgecko;
	allDevices[i++] = &__device_ftp;
	allDevices[i++] = NULL;

	// Set current devices
	devices[DEVICE_CUR] = NULL;
	devices[DEVICE_DEST] = NULL;
	devices[DEVICE_TEMP] = NULL;
	devices[DEVICE_CONFIG] = NULL;
	devices[DEVICE_PATCHES] = NULL;
	
	settings_init();	// Init settings for Swiss
	
	//	init_textures();
	
	drive_version(&driveVersion[0]);
	swissSettings.hasDVDDrive = *(u32*)&driveVersion[0] ? 1 : 0;
	
	if(!driveVersion[0]) {
		ShowAction("Initialise DVD .. (HOLD B if NO DVD Drive)");
		// Reset DVD if there was a modchip
		dvd_reset();	// low-level, basic
		dvd_read_id();
		if(!(PAD_ButtonsHeld(0) & PAD_BUTTON_B)) {
			dvd_set_streaming(*(char*)0x80000008);
		}
		drive_version(&driveVersion[0]);
		swissSettings.hasDVDDrive = *(u32*)&driveVersion[0] ? 1 : 0;
		CancelAction();
		if(!swissSettings.hasDVDDrive && !(PAD_ButtonsHeld(0) & PAD_BUTTON_B)) {
			ShowAction("No DVD Drive Detected !!");
			sleep(2);
			CancelAction();
		}
		CancelAction();
	}
	
	needsDeviceChange = 1;
	needsRefresh = 1;
	
	swissSettings.debugUSB = 1;	// TODO remove
	
	//debugging stuff
	if(swissSettings.debugUSB) {
		if(usb_isgeckoalive(1)) {
			usb_flush(1);
		}
		print_gecko("Arena Size: %iKb\r\n",(SYS_GetArena1Hi()-SYS_GetArena1Lo())/1024);
		print_gecko("DVD Drive Present? %s\r\n",swissSettings.hasDVDDrive?"Yes":"No");
		print_gecko("GIT Commit: %s\r\n", GITREVISION);
		print_gecko("GIT Revision: %s\r\n", GITVERSION);
	}
	
	// Go through all devices with FEAT_BOOT_DEVICE feature and set it as current if one is available
	ShowAction("Checking devices ...");
	for(i = 0; i < MAX_DEVICES; i++) {
		if(allDevices[i] != NULL && (allDevices[i]->features & FEAT_BOOT_DEVICE)) {
			print_gecko("Testing device %s\r\n", allDevices[i]->deviceName);
			if(allDevices[i]->test()) {
				deviceHandler_setDeviceAvailable(allDevices[i], true);
				devices[DEVICE_CUR] = allDevices[i];
				break;
			}
		}
	}
	CancelAction();
	
	if(devices[DEVICE_CUR] != NULL) {
		print_gecko("Detected %s\r\n", devices[DEVICE_CUR]->deviceName);
		devices[DEVICE_CUR]->init(devices[DEVICE_CUR]->initial);
		if(devices[DEVICE_CUR]->features & FEAT_AUTOLOAD_DOL) {
			load_auto_dol();
		}
		memcpy(&curFile, devices[DEVICE_CUR]->initial, sizeof(file_handle));
		needsDeviceChange = 0;
		// TODO: re-add if dvd && gcm type disc, show banner/boot screen
		// If this device can write, set it as the config device for now
	}

	// Scan here since some devices would already be initialised (faster)
	populateDeviceAvailability();
	
	// load config
	load_config();
	swissSettings.initNetworkAtStart=1;
	if(swissSettings.initNetworkAtStart) {
		// Start up the BBA if it exists
		ShowAction("Initialising Network");
		init_network();
		init_httpd_thread();
		CancelAction();
	}
	
	// DVD Motor off
	if(swissSettings.stopMotor && swissSettings.hasDVDDrive) {
		dvd_motor_off();
	}

	//while(1) {
	//	main_loop();
	//}
}

GXRModeObj *getModeFromSwissSetting(int uiVMode) {
	switch(uiVMode) {
		case 1:
			switch(swissSettings.sramVideo) {
				case 2:  return &TVMpal480IntDf;
				case 1:  return &TVEurgb60Hz480IntDf;
				default: return &TVNtsc480IntDf;
			}
		case 2:
			if(VIDEO_HaveComponentCable()) {
				switch(swissSettings.sramVideo) {
					case 2:  return &TVMpal480Prog;
					case 1:  return &TVEurgb60Hz480Prog;
					default: return &TVNtsc480Prog;
				}
			} else {
				switch(swissSettings.sramVideo) {
					case 2:  return &TVMpal480IntDf;
					case 1:  return &TVEurgb60Hz480IntDf;
					default: return &TVNtsc480IntDf;
				}
			}
		case 3:
			return &TVPal576IntDfScale;
		case 4:
			if(VIDEO_HaveComponentCable()) {
				return &TVPal576ProgScale;
			} else {
				return &TVPal576IntDfScale;
			}
	}
	return NULL;
}

// Checks if devices are available, prints name of device being detected for slow init devices
void populateDeviceAvailability() {
	ShowProgress("Detecting devices ...\nThis can be skipped by holding B next time", 0, MAX_DEVICES);
	if(PAD_ButtonsHeld(0) & PAD_BUTTON_B) {
		deviceHandler_setAllDevicesAvailable();
		return;
	}
	int i;
	for(i = 0; i < MAX_DEVICES; i++) {
		if(allDevices[i] != NULL && !deviceHandler_getDeviceAvailable(allDevices[i])) {
			print_gecko("Checking device availability for device %s\r\n", allDevices[i]->deviceName);
			deviceHandler_setDeviceAvailable(allDevices[i], allDevices[i]->test());
		}
		if(PAD_ButtonsHeld(0) & PAD_BUTTON_B) {
			deviceHandler_setAllDevicesAvailable();
			break;
		}
		ShowProgress("Detecting devices ...\nThis can be skipped by holding B next time", i, MAX_DEVICES);
	}
}
