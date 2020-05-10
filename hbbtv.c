/*
 * hbbtv.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/remote.h>
#include "hbbtv.h"
#include "hbbtvmenu.h"
#include "status.h"
#include "hbbtvservice.h"
#include "hbbtvvideocontrol.h"
#include "browsercommunication.h"
#include "cefhbbtvpage.h"
#include "osdshm.h"

static const char *VERSION        = "0.1.0";
static const char *DESCRIPTION    = "URL finder for HbbTV";
static const char *MAINMENUENTRY  = "HbbTV URLs";

cHbbtvDeviceStatus *HbbtvDeviceStatus;

cPluginHbbtv::cPluginHbbtv(void)
{
   // Initialize any member variables here.
   // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
   // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
   HbbtvDeviceStatus = NULL;
   browserComm = NULL;
   osdDispatcher = new OsdDispatcher();
   showPlayer = false;
}


cPluginHbbtv::~cPluginHbbtv()
{
   // Clean up after yourself!
   DELETENULL(osdDispatcher);
}

const char* cPluginHbbtv::Version(void) { return VERSION; }
const char* cPluginHbbtv::Description(void) { return DESCRIPTION; }
const char* cPluginHbbtv::MainMenuEntry(void) { return MAINMENUENTRY; }

bool cPluginHbbtv::Start(void)
{
   // Start any background activities the plugin shall perform.
   HbbtvDeviceStatus = new cHbbtvDeviceStatus();

   browserComm = new BrowserCommunication();
   browserComm->Start();

   return true;
}

void cPluginHbbtv::Stop(void)
{
   // Stop any background activities the plugin is performing.
   if (HbbtvDeviceStatus) DELETENULL(HbbtvDeviceStatus);
   if (browserComm) DELETENULL(browserComm);

   lastDisplayWidth = 0;
   lastDisplayHeight = 0;
}


cOsdObject *cPluginHbbtv::MainMenuAction(void)
{
  // Perform the action when selected from the main VDR menu.
  return osdDispatcher->get(MAINMENUENTRY);
}

void cPluginHbbtv::MainThreadHook(void) {
    if (showPlayer) {
        showPlayer = false;

        if (!isHbbtvPlayerActivated) {
            auto video = new HbbtvVideoControl(new HbbtvVideoPlayer());
            cControl::Launch(video);
            video->Attach();
        }
    }

    if (hbbtvPage != nullptr) {
        static int osdState = 0;
        if (cOsdProvider::OsdSizeChanged(osdState)) {
            int newWidth;
            int newHeight;
            double ph;
            cDevice::PrimaryDevice()->GetOsdSize(newWidth, newHeight, ph);

            if (newWidth != lastDisplayWidth || newHeight != lastDisplayHeight) {
                dsyslog("Old Size: %dx%d, New Size. %dx%d", lastDisplayWidth, lastDisplayHeight, newWidth, newHeight);
                lastDisplayWidth = newWidth;
                lastDisplayHeight = newHeight;

                dsyslog("MainThreadHook => Display()");
                hbbtvPage->Display();
            }

            // osdImage->TriggerOsdResize();
        }
    }
}

bool cPluginHbbtv::Service(const char *Id, void *Data)
{
    if (strcmp(Id, "BrowserStatus-1.0") == 0) {
        if (Data) {
            BrowserStatus_v1_0 *status = (BrowserStatus_v1_0*)Data;

            dsyslog("Received Status: %s", *status->message);

            if (strncmp(status->message, "PLAY_VIDEO:", 11) == 0) {
                showPlayer = true;
            }
        }
        return true;
    }
    return false;
}

VDRPLUGINCREATOR(cPluginHbbtv);  // Don't touch this!
