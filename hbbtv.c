/*
 * hbbtv.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <vdr/remote.h>
#include "hbbtv.h"
#include "hbbtvurl.h"
#include "hbbtvmenu.h"
#include "status.h"
#include "hbbtvservice.h"
#include "hbbtvvideocontrol.h"
#include "browsercommunication.h"
#include "cefhbbtvpage.h"
#include "osdshm.h"

static const char *VERSION = "0.1.0";
static const char *DESCRIPTION = "URL finder for HbbTV";
static const char *MAINMENUENTRY = "HbbTV";

cHbbtvDeviceStatus *HbbtvDeviceStatus;

cPluginHbbtv::cPluginHbbtv(void) {
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
    HbbtvDeviceStatus = NULL;
    browserComm = NULL;
    osdDispatcher = new
    OsdDispatcher();
    showPlayer = false;
    lastWriteTime = std::time(nullptr);
}

cPluginHbbtv::~cPluginHbbtv() {
    // Clean up after yourself!
    DELETENULL(osdDispatcher);
}

const char *cPluginHbbtv::Version(void) { return VERSION; }

const char *cPluginHbbtv::Description(void) { return DESCRIPTION; }

const char *cPluginHbbtv::MainMenuEntry(void) { return MAINMENUENTRY; }

void cPluginHbbtv::WriteUrlsToFile() {
    char *urlFileName;
    asprintf(&urlFileName, "%s/hbbtv_urls.list", ConfigDirectory(Name()));
    const cStringList *allUrls = cHbbtvURLs::AllURLs();

    FILE *urlFile = fopen(urlFileName, "w");
    if (urlFile) {
        for (int i = 0; i < allUrls->Size(); i++) {
            fprintf(urlFile, "%s\n", (*allUrls)[i]);
        }
        fclose(urlFile);
    }

    free(urlFileName);
}

bool cPluginHbbtv::Start(void) {
    // Start any background activities the plugin shall perform.

    // read existing HbbTV URL list
    char *urlFileName;
    asprintf(&urlFileName, "%s/hbbtv_urls.list", ConfigDirectory(Name()));
    cStringList *allURLs = cHbbtvURLs::AllURLs();

    FILE *urlFile = fopen(urlFileName, "r");
    int bufferLength = 1024;
    char buffer[bufferLength];

    if (urlFile) {
        while(fgets(buffer, bufferLength, urlFile)) {
            strtok(buffer, "\n");

            if (allURLs->Find(buffer) == -1 && strlen(buffer) > 3) {
                allURLs->InsertUnique(strdup(buffer));
            }
        }
        fclose(urlFile);
    }

    free(urlFileName);


    HbbtvDeviceStatus = new cHbbtvDeviceStatus();

    browserComm = new BrowserCommunication();
    browserComm->Start();

    return true;
}

void cPluginHbbtv::Stop(void) {
    // Stop any background activities the plugin is performing.
    if (HbbtvDeviceStatus) DELETENULL(HbbtvDeviceStatus);
    if (browserComm) DELETENULL(browserComm);

    lastDisplayWidth = 0;
    lastDisplayHeight = 0;

    WriteUrlsToFile();
}

cOsdObject *cPluginHbbtv::MainMenuAction(void) {
    // Perform the action when selected from the main VDR menu.
    return osdDispatcher->get(MAINMENUENTRY, Name());
}

void cPluginHbbtv::MainThreadHook(void) {
    std::time_t current = std::time(nullptr);
    if (current - lastWriteTime > 5 * 60) {
        WriteUrlsToFile();
        lastWriteTime = current;
    }

    if (showPlayer) {
        showPlayer = false;

        if (!isHbbtvPlayerActivated) {
            auto video = new
            HbbtvVideoControl(new
            HbbtvVideoPlayer());
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
        }
    }
}

bool cPluginHbbtv::Service(const char *Id, void *Data) {
    if (strcmp(Id, "BrowserStatus-1.0") == 0) {
        if (Data) {
            BrowserStatus_v1_0 *status = (BrowserStatus_v1_0 *) Data;

            dsyslog("Received Status: %s", *status->message);

            if (strncmp(status->message, "PLAY_VIDEO:", 11) == 0) {
                // show video player
                showPlayer = true;
            } else if (strncmp(status->message, "STOP_VIDEO", 10) == 0) {
                // stop video player and show TV again
                cControl * current = cControl::Control();

                if (dynamic_cast<HbbtvVideoControl * >(current)) {
                    cControl::Shutdown();
                    cControl::Attach();
                }
            }
        }
        return true;
    }
    return false;
}

VDRPLUGINCREATOR(cPluginHbbtv);  // Don't touch this!
