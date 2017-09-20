/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * menu.h
 * Menu flow routines - handles all menu logic
 ***************************************************************************/
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#ifndef _MENU_H_
#define _MENU_H_

#include <ogcsys.h>

EXTERNC void InitGUIThreads();
EXTERNC void InitMainWindow();
EXTERNC void MainMenu (int menuitem);
EXTERNC void ErrorPrompt(const char * msg);
EXTERNC int ErrorPromptRetry(const char * msg);
EXTERNC void InfoPrompt(const char * msg);
EXTERNC void ShowAction (const char *msg);
EXTERNC void CancelAction();
EXTERNC void ShowProgress (const char *msg, int done, int total);

enum
{
	MENU_EXIT = -1,
	MENU_NONE,
	MENU_SETTINGS,
	MENU_SETTINGS_FILE,
	MENU_BROWSE_DEVICE
};

#endif
