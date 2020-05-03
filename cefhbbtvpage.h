/**
 *  VDR HbbTV Plugin
 *
 *  cefhbbtvpage.h
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#ifndef CEFHBBTVPAGE_H
#define CEFHBBTVPAGE_H

#include <string>
#include <thread>
#include <mutex>
#include <vdr/tools.h>
#include <vdr/osd.h>
#include <vdr/osdbase.h>
#include "browsercommunication.h"

typedef struct OsdStruct {
    char    message[20];
    int     width;
    int     height;
} OsdStruct;

class CefHbbtvPage : public cOsdObject {

private:
    std::string hbbtvUrl;

    cPixmap *pixmap;
    cOsd* osd;

    std::mutex shm_mutex;
    std::mutex show_mutex;

public:
    CefHbbtvPage();
    ~CefHbbtvPage() override;
    void Show() override;

    bool loadPage(std::string url);

    bool hideBrowser();
    bool showBrowser();

    bool setHtmlMode()  { return browserComm->SendToBrowser("MODE 1"); };
    bool setHbbtvMode() { return browserComm->SendToBrowser("MODE 2"); };

    bool sendKeyEvent(cString key);

    cOsd* GetOsd() { return osd; }
    void readOsdUpdate(OsdStruct* osdUpdate);

    eOSState ProcessKey(eKeys Key) override;
};

extern CefHbbtvPage *hbbtvPage;

#endif // CEFHBBTVPAGE_H
