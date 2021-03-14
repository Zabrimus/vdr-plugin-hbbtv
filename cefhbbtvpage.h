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
#include "hbbtv.h"

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

    int disp_width;
    int disp_height;
    bool resizeOsd;

public:
    CefHbbtvPage();
    ~CefHbbtvPage() override;
    void Show() override;
    void Display();
    void SetOsdSize();
    void TriggerOsdResize();

    void readOsdUpdate(OsdStruct* osdUpdate);
    bool loadPage(std::string url);
    bool reopen();

    bool hideBrowser();
    bool showBrowser();

    bool setHtmlMode()  { return browserComm->SendToBrowser("MODE 1"); };
    bool setHbbtvMode() { return browserComm->SendToBrowser("MODE 2"); };

    void ClearRect(int x, int y, int width, int height);

    eOSState ProcessKey(eKeys Key) override;
};

extern CefHbbtvPage *hbbtvPage;

#endif // CEFHBBTVPAGE_H
