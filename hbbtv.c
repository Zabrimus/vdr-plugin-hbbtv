/*
 * hbbtv.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/plugin.h>
#include <vdr/device.h>

#include "hbbtvmenu.h"
#include "status.h"

static const char *VERSION        = "0.1.0";
static const char *DESCRIPTION    = "URL finder for HbbTV";
static const char *MAINMENUENTRY  = "HbbTV URLs";


cHbbtvDeviceStatus *HbbtvDeviceStatus;

class cPluginHbbtv : public cPlugin
{
   // Add any member variables or functions you may need here.
   public:
      cPluginHbbtv(void);
      virtual ~cPluginHbbtv();
      virtual const char *Version(void) { return VERSION; }
      virtual const char *Description(void) { return DESCRIPTION; }
      virtual bool Start(void);
      virtual void Stop(void);
      virtual const char *MainMenuEntry(void) { return MAINMENUENTRY; }
      virtual cOsdObject *MainMenuAction(void);
};

cPluginHbbtv::cPluginHbbtv(void)
{
   // Initialize any member variables here.
   // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
   // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
   HbbtvDeviceStatus = NULL;
}


cPluginHbbtv::~cPluginHbbtv()
{
   // Clean up after yourself!
}


bool cPluginHbbtv::Start(void)
{
   // Start any background activities the plugin shall perform.
   HbbtvDeviceStatus = new cHbbtvDeviceStatus();

   return true;
}


void cPluginHbbtv::Stop(void)
{
   // Stop any background activities the plugin is performing.
   if (HbbtvDeviceStatus) DELETENULL(HbbtvDeviceStatus);
}


cOsdObject *cPluginHbbtv::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return new cHbbtvMenu(MAINMENUENTRY);
}

VDRPLUGINCREATOR(cPluginHbbtv);  // Don't touch this!
