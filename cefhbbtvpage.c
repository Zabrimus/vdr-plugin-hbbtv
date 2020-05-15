/**
 *  VDR HbbTV Plugin
 *
 *  cefhbbtvpage.cpp
 *
 *  (c) 2019 Robert Hannebauer
 *
 * This code is distributed under the terms and conditions of the
 * GNU GENERAL PUBLIC LICENSE. See the file COPYING for details.
 *
 **/

#ifdef __cplusplus
extern "C" {
#endif
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif

#include <chrono>
#include <thread>
#include <vdr/plugin.h>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pipeline.h>
#include <unistd.h>
#include "hbbtvservice.h"
#include "hbbtvvideocontrol.h"
#include "cefhbbtvpage.h"
#include "osdshm.h"

#define SET_AREA_VERY_EARLY

CefHbbtvPage *hbbtvPage;

struct SwsContext *swsCtx = nullptr;

CefHbbtvPage::CefHbbtvPage() {
    dsyslog("Construct HbbtvPage...");

    osd = nullptr;
    pixmap = nullptr;
    hbbtvPage = this;
    resizeOsd = false;
}

CefHbbtvPage::~CefHbbtvPage() {
    dsyslog("Destroy HbbtvPage...");

    // hide only, of player is not attached
    if (hbbtvVideoPlayer != nullptr && !hbbtvVideoPlayer->IsAttached()) {
        dsyslog("Destroy HbbtvPage: hideBrowser");
        hideBrowser();
    }

    if (osd != nullptr) {
        dsyslog("Destroy HbbtvPage: delete osd");
        delete osd;
        osd = nullptr;
    } else {
        dsyslog("Destroy HbbtvPage: osd is already null");
    }

    shm_mutex.unlock();
    show_mutex.unlock();

    pixmap = nullptr;
    hbbtvPage = nullptr;
}

void CefHbbtvPage::Show() {
    dsyslog("HbbtvPage Show()");
    Display();
}

void CefHbbtvPage::Display() {
    dsyslog("HbbtvPage Show()");

    if (osd) {
        delete osd;
    }

    osd = cOsdProvider::NewOsd(0, 0);

#ifdef SET_AREA_VERY_EARLY
    tArea areas[] = {
            {0, 0, 4096 - 1, 2160 - 1, 32}, // 4K
            {0, 0, 2560 - 1, 1440 - 1, 32}, // 2K
            {0, 0, 1920 - 1, 1080 - 1, 32}, // Full HD
            {0, 0, 1280 - 1,  720 - 1, 32}, // 720p
    };

    // set the maximum area size to 4K
    bool areaFound = false;
    for (int i = 0; i < 4; ++i) {
        auto areaResult = osd->SetAreas(&areas[i], 1);

        if (areaResult == oeOk) {
            isyslog("Area size set to %d:%d - %d:%d", areas[i].x1, areas[i].y1, areas[i].x2, areas[i].y2);
            areaFound = true;
            break;
        }
    }

    if (!areaFound) {
        esyslog("Unable set any OSD area. OSD will not be created");
    }
#endif

    SetOsdSize();
    browserComm->SendToBrowser("OSDU");
}

void CefHbbtvPage::TriggerOsdResize() {
    dsyslog("HbbtvPage TriggerOsdResize()");

    resizeOsd = true;

    dsyslog("HbbtvPage TriggerOsdResize, SendOSD");
    browserComm->SendToBrowser("SENDOSD");
}

void CefHbbtvPage::SetOsdSize() {
    dsyslog("HbbtvPage SetOsdSize()");

    if (pixmap != nullptr) {
        dsyslog("HbbtvPage SetOsdSize, Destroy old pixmap");

        osd->DestroyPixmap(pixmap);
        pixmap = nullptr;
    }

    dsyslog("HbbtvPage SetOsdSize, Get new OSD size");
    double ph;
    cDevice::PrimaryDevice()->GetOsdSize(disp_width, disp_height, ph);

    if (disp_width <= 0 || disp_height <= 0 || disp_width > 4096 || disp_height > 2160) {
        esyslog("hbbtv: Got illegal OSD size %dx%d", disp_width, disp_height);
        return;
    }

#ifndef SET_AREA_VERY_EARLY
    tArea area  = {0, 0, disp_width - 1, disp_height - 1, 32};
    auto areaResult = osd->SetAreas(&area, 1);

    if (areaResult == oeOk) {
        dsyslog("Area size set to %d:%d - %d:%d\n", area.x1, area.y1, area.x2, area.y2);
    } else {
        esyslog("Unable to set area %d:%d - %d:%d\n", area.x1, area.y1, area.x2, area.y2);
        return;
    }
#endif

    cRect rect(0, 0, disp_width, disp_height);

    dsyslog("HbbtvPage SetOsdSize, Mutex Lock");
    show_mutex.lock();

    // try to get a pixmap
    dsyslog("HbbtvPage SetOsdSize, Create pixmap %dx%d", disp_width, disp_height);
    pixmap = osd->CreatePixmap(0, rect, rect);

    dsyslog("HbbtvPage SetOsdSize, Clear Pixmap");
    pixmap->Lock();
    pixmap->Clear();
    pixmap->Unlock();

    dsyslog("HbbtvPage SetOsdSize, Mutex unlock");
    show_mutex.unlock();
}

eOSState CefHbbtvPage::ProcessKey(eKeys Key) {
    eOSState state = cOsdObject::ProcessKey(Key);

    if (state == osUnknown) {
        if (Key == kBack) {
            dsyslog("HbbtvPage ProcessKey, hide browser");
            hideBrowser();
            return osEnd;
        }

        bool result = browserComm->SendKey(Key);
        if (result) {
            return osContinue;
        }
    }

    return state;
}

bool CefHbbtvPage::loadPage(std::string url) {
    dsyslog("HbbtvPage loadPage, Show browser");
    showBrowser();

    dsyslog("HbbtvPage loadPage, set HbbTV mode");
    hbbtvUrl = url;
    setHbbtvMode();

    std::string cmdUrl("URL ");
    cmdUrl.append(url);

    dsyslog("HbbtvPage loadPage, Send URL to browser");
    browserComm->SendToBrowser(cmdUrl.c_str());

    return true;
}

bool CefHbbtvPage::hideBrowser() {
    dsyslog("Hide Browser");
    return browserComm->SendToBrowser("PAUSE");
}

bool CefHbbtvPage::showBrowser() {
    dsyslog("Show Browser");
    return browserComm->SendToBrowser("RESUME");
}

void CefHbbtvPage::readOsdUpdate(OsdStruct* osdUpdate) {
    if (resizeOsd) {
        dsyslog("HbbtvPage readOsdUpdate, setOsdSize");
        SetOsdSize();
        resizeOsd = false;
    }

    if (disp_width <= 0 || disp_height <= 0 || disp_width > 4096 || disp_height > 2160) {
        esyslog("hbbtv: Got illegal OSD size %dx%d", disp_width, disp_height);
        return;
    }

    dsyslog("HbbtvPage readOsdUpdate, mutex lock");
    show_mutex.lock();

    dsyslog("HbbtvPage readOsdUpdate, message received %s", osdUpdate->message);
    if (strncmp(osdUpdate->message, "OSDU", 4) != 0) {
        // Internal error. Expected command OSDU, but got something else
        esyslog("HbbtvPage readOsdUpdate, unknown message %s", osdUpdate->message);
        browserComm->SendToBrowser("OSDU");
        show_mutex.unlock();
        return;
    }

    // sanity check
    if (osdUpdate->width > 1920 || osdUpdate->height > 1080 || osdUpdate->width <= 0 || osdUpdate->height <= 0) {
        // there is some garbage in the shared memory => ignore
        esyslog("HbbtvPage readOsdUpdate, illegal width %dx%d", osdUpdate->width, osdUpdate->height);
        browserComm->SendToBrowser("OSDU");
        show_mutex.unlock();
        return;
    }

    // create image buffer for scaled image
    cSize recImageSize(disp_width, disp_height);
    cPoint recPoint(0, 0);
    const cImage recImage(recImageSize);
    auto *scaled  = (uint8_t*)(recImage.Data());

    if (scaled == nullptr) {
        esyslog("Out of memory reading OSD image");
        browserComm->SendToBrowser("OSDU");
        show_mutex.unlock();
        return;
    }

    // scale image
    dsyslog("HbbtvPage readOsdUpdate, get scale context");
    if (swsCtx != nullptr) {
        swsCtx = sws_getCachedContext(swsCtx,
                                      osdUpdate->width, osdUpdate->height, AV_PIX_FMT_BGRA,
                                      disp_width, disp_height, AV_PIX_FMT_BGRA,
                                      SWS_BILINEAR, NULL, NULL, NULL);
    } else {
        swsCtx = sws_getContext(osdUpdate->width, osdUpdate->height, AV_PIX_FMT_BGRA,
                                disp_width, disp_height, AV_PIX_FMT_BGRA,
                                SWS_BILINEAR, NULL, NULL, NULL);
    }

    uint8_t *inData[1] = {osd_shm.get()};
    int inLinesize[1] = {4 * osdUpdate->width};
    int outLinesize[1] = {4 * disp_width};

    dsyslog("HbbtvPage readOsdUpdate, scale image");
    sws_scale(swsCtx, inData, inLinesize, 0, osdUpdate->height, &scaled, outLinesize);

    // TEST
    // This is incredible slow. Use it only if you want to see the incoming images
    /*
    static int i = 0;
    char *filename = nullptr;
    asprintf(&filename, "image_%d.rgba", i);
    FILE *f = fopen(filename, "wb");
    fwrite(data2, osdUpdate->width * osdUpdate->height * 4, 1, f);
    fclose(f);

    char *pngfile = nullptr;
    asprintf(&pngfile, "gm convert -size 1280x720 -depth 8 %s %s.png", filename, filename);
    system(pngfile);

    free(filename);
    free(pngfile);
    pngfile = nullptr;
    filename = nullptr;

    ++i;
    */
    // TEST

    if (pixmap != nullptr) {
        dsyslog("HbbtvPage readOsdUpdate, draw image lock");
        pixmap->Lock();
        pixmap->DrawImage(recPoint, recImage);
        pixmap->Unlock();
        dsyslog("HbbtvPage readOsdUpdate, draw image unlock");
    }

    dsyslog("HbbtvPage readOsdUpdate, flush osd");
    if (osd != nullptr) {
        osd->Flush();
    }

    dsyslog("HbbtvPage readOsdUpdate, send OSDU to browser");
    browserComm->SendToBrowser("OSDU");

    dsyslog("HbbtvPage readOsdUpdate, mutex unlock");
    show_mutex.unlock();
}
