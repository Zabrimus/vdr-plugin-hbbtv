/**
 *  VDR Skin Plugin which uses CEF OSR as rendering engine
 *
 *  browser.h
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#ifndef BROWSER_H
#define BROWSER_H

#include <string>
#include <thread>
#include <mutex>
#include <sys/shm.h>
#include <vdr/tools.h>
#include <vdr/osd.h>
#include "browsercommunication.h"

// to enable much more debug data output to stderr, set this variable to true
extern bool DumpDebugData;
#define dbgbrowser(a...) if (DumpDebugData) fprintf(stderr, a)

class Browser {
    friend BrowserCommunication;

    public:
        Browser();
        ~Browser();

        bool loadPage(std::string url, int rootFontSize);

        bool hideBrowser();
        bool showBrowser();

        bool setHtmlMode()  { return browserComm->SendToBrowser("MODE 1"); };
        bool setHbbtvMode() { return browserComm->SendToBrowser("MODE 2"); };

        bool setBrowserSize(int width, int height);
        bool setZoomLevel(double zoom);
        bool setRootFontSize(int px);

        bool sendKeyEvent(cString key);

        void callRawJavascript(cString script);

        void createOsd(int left, int top, int width, int height);
        void FlushOsd();

        cOsd* GetOsd() { return osd; }

    private:
        cPixmap *pixmap;

        cOsd* osd;
        int osdWidth;
        int osdHeight;

        int shmid;
        uint8_t *shmp;
        std::mutex shm_mutex;

        void readOsdUpdate(int socketId);
};

extern Browser *browser;

#endif // BROWSER_H
