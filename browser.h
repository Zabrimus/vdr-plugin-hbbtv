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
#include <vdr/tools.h>
#include <thread>
#include <vdr/osd.h>

// to enable much more debug data output to stderr, set this variable to true
extern bool DumpDebugData;
#define dbgbrowser(a...) if (DumpDebugData) fprintf(stderr, a)

class Browser {

public:
    Browser(std::string ipcCommandFile, std::string ipcStreamFile);
    ~Browser();

    bool loadPage(std::string url, int rootFontSize);

    bool hideBrowser();
    bool showBrowser();

    bool setHtmlMode()  { return sendCommand("MODE 1"); };
    bool setHbbtvMode() { return sendCommand("MODE 2"); };

    bool setBrowserSize(int width, int height);
    bool setZoomLevel(double zoom);
    bool setRootFontSize(int px);

    bool sendKeyEvent(cString key);

    void callRawJavascript(cString script);

    void startUpdate(int left, int top, int width, int height);
    void stopUpdate();
    void FlushOsd();

    cOsd* GetOsd() { return osd; }

private:
    std::thread *updateThread;
    cPixmap *pixmap;
    bool isRunning;

    cOsd* osd;
    static int osdWidth;
    static int osdHeight;

    int commandSocketId;
    int commandEndpointId;

    int streamSocketId;
    int streamEndpointId;

    bool sendCommand(const char* command);
    static void readStream(int width, cPixmap *destPixmap);
};


#endif // BROWSER_H
