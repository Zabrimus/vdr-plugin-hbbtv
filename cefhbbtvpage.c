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
#include "globals.h"

// #define SET_AREA_VERY_EARLY

CefHbbtvPage *hbbtvPage;

struct SwsContext *swsCtx = nullptr;

CefHbbtvPage::CefHbbtvPage() {
    dsyslog("[hbbtv] Construct HbbtvPage...");

    osd = nullptr;
    pixmap = nullptr;
    hbbtvPage = this;
    resizeOsd = false;
}

CefHbbtvPage::~CefHbbtvPage() {
    dsyslog("[hbbtv] Destroy HbbtvPage...");

    // hide only, of player is not attached
    if (hbbtvVideoPlayer != nullptr && !hbbtvVideoPlayer->IsAttached()) {
        dsyslog("[hbbtv] Destroy HbbtvPage: hideBrowser");
        hideBrowser();
    }

    if (osd != nullptr) {
        dsyslog("[hbbtv] Destroy HbbtvPage: delete osd");
        delete osd;
        osd = nullptr;
    } else {
        dsyslog("[hbbtv] Destroy HbbtvPage: osd is already null");
    }

    shm_mutex.unlock();
    show_mutex.unlock();

    pixmap = nullptr;
    hbbtvPage = nullptr;

    setVideoDefaultSize();
}

void CefHbbtvPage::Show() {
    dsyslog("[hbbtv] HbbtvPage Show()");
    Display();
}

void CefHbbtvPage::Display() {
    dsyslog("[hbbtv] HbbtvPage Display()");

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
            isyslog("[hbbtv] Area size set to %d:%d - %d:%d", areas[i].x1, areas[i].y1, areas[i].x2, areas[i].y2);
            areaFound = true;
            break;
        }
    }

    if (!areaFound) {
        esyslog("[hbbtv] Unable set any OSD area. OSD will not be created");
    }
#endif

    SetOsdSize();
    browserComm->SendToBrowser("OSDU");
}

void CefHbbtvPage::TriggerOsdResize() {
    dsyslog("[hbbtv] HbbtvPage TriggerOsdResize()");

    resizeOsd = true;

    dsyslog("[hbbtv] HbbtvPage TriggerOsdResize, SendOSD");
    browserComm->SendToBrowser("SENDOSD");
}

void CefHbbtvPage::SetOsdSize() {
    dsyslog("[hbbtv] HbbtvPage SetOsdSize()");

    if (pixmap != nullptr) {
        dsyslog("[hbbtv] HbbtvPage SetOsdSize, Destroy old pixmap");

        osd->DestroyPixmap(pixmap);
        pixmap = nullptr;
    }

    dsyslog("[hbbtv] HbbtvPage SetOsdSize, Get new OSD size");
    double ph;
    cDevice::PrimaryDevice()->GetOsdSize(disp_width, disp_height, ph);

    if (disp_width <= 0 || disp_height <= 0 || disp_width > 4096 || disp_height > 2160) {
        esyslog("[hbbtv] Got illegal OSD size %dx%d", disp_width, disp_height);
        return;
    }

#ifndef SET_AREA_VERY_EARLY
    tArea area  = {0, 0, disp_width - 1, disp_height - 1, 32};
    auto areaResult = osd->SetAreas(&area, 1);

    if (areaResult == oeOk) {
        dsyslog("[hbbtv] Area size set to %d:%d - %d:%d\n", area.x1, area.y1, area.x2, area.y2);
    } else {
        esyslog("[hbbtv] Unable to set area %d:%d - %d:%d\n", area.x1, area.y1, area.x2, area.y2);
        return;
    }
#endif

    cRect rect(0, 0, disp_width, disp_height);

    HBBTV_DBG("[hbbtv] HbbtvPage SetOsdSize, Mutex Lock");
    show_mutex.lock();

    // try to get a pixmap
    HBBTV_DBG("[hbbtv] HbbtvPage SetOsdSize, Create pixmap %dx%d", disp_width, disp_height);
    pixmap = osd->CreatePixmap(0, rect, rect);

    HBBTV_DBG("[hbbtv] HbbtvPage SetOsdSize, Clear Pixmap");
    pixmap->Lock();
    pixmap->Clear();
    pixmap->Unlock();

    HBBTV_DBG("[hbbtv] HbbtvPage SetOsdSize, Mutex unlock");
    show_mutex.unlock();
}

eOSState CefHbbtvPage::ProcessKey(eKeys Key) {
    eOSState state = cOsdObject::ProcessKey(Key);

    if (state == osUnknown) {
        if (Key == kBack) {
            dsyslog("[hbbtv] HbbtvPage ProcessKey, hide browser");
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
    dsyslog("[hbbtv] HbbtvPage loadPage, Show browser");
    if (!showBrowser()) {
        // browser is not running
        return false;
    }

    dsyslog("[hbbtv] HbbtvPage loadPage, set HbbTV mode");
    hbbtvUrl = url;
    if (!setHbbtvMode()) {
        // browser is not running
        return false;
    }

    std::string cmdUrl("URL ");
    cmdUrl.append(url);

    dsyslog("[hbbtv] HbbtvPage loadPage, Send URL to browser");
    if (!browserComm->SendToBrowser(cmdUrl.c_str())) {
        // browser is not running
        return false;
    }

    return true;
}

bool CefHbbtvPage::reopen() {
    if (!showBrowser()) {
        // browser is not running
        return false;
    }

    if (!browserComm->SendToBrowser("OSDU")) {
        // browser is not running
        return false;
    }

    return true;
}

bool CefHbbtvPage::hideBrowser() {
    dsyslog("[hbbtv] Hide Browser");
    setVideoDefaultSize();
    SetVideoSize();
    return browserComm->SendToBrowser("PAUSE");
}

bool CefHbbtvPage::showBrowser() {
    dsyslog("[hbbtv] Show Browser");
    return browserComm->SendToBrowser("RESUME");
}

void CefHbbtvPage::readOsdUpdate(OsdStruct* osdUpdate) {
    if (resizeOsd) {
        SetOsdSize();
        resizeOsd = false;
    }

    if (disp_width <= 0 || disp_height <= 0 || disp_width > 4096 || disp_height > 2160) {
        esyslog("[hbbtv] Got illegal OSD size %dx%d", disp_width, disp_height);
        return;
    }

    HBBTV_DBG("[hbbtv] Show_Mutex try to get lock");
    show_mutex.lock();
    HBBTV_DBG("[hbbtv] Show_Mutex got lock");

    if (strncmp(osdUpdate->message, "OSDU", 4) != 0) {
        // Internal error. Expected command OSDU, but got something else
        esyslog("[hbbtv] HbbtvPage readOsdUpdate, unknown message %s", osdUpdate->message);
        browserComm->SendToBrowser("OSDU");
        show_mutex.unlock();
        return;
    }

    // sanity check
    if (osdUpdate->width > 1920 || osdUpdate->height > 1080 || osdUpdate->width <= 0 || osdUpdate->height <= 0) {
        // there is some garbage in the shared memory => ignore
        esyslog("[hbbtv] HbbtvPage readOsdUpdate, illegal width %dx%d", osdUpdate->width, osdUpdate->height);
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
        esyslog("[hbbtv] Out of memory reading OSD image");
        browserComm->SendToBrowser("OSDU");
        show_mutex.unlock();
        return;
    }

    // scale image
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
        HBBTV_DBG("[hbbtv] Try to get pixmap lock");
        pixmap->Lock();
        HBBTV_DBG("[hbbtv] Got pixmap lock");

        pixmap->DrawImage(recPoint, recImage);

        if (!isVideoFullscreen()) {
            int x, y, w, h;
            calcVideoPosition(&x, &y, &w, &h);

            cSize trans_size(w, h);
            cImage trans_image(trans_size);
            trans_image.Clear();
            cPoint trans_point(x, y);
            pixmap->DrawImage(trans_point, trans_image);
        }

        pixmap->Unlock();
    }

    if (osd != nullptr) {
        osd->Flush();
    }

    browserComm->SendToBrowser("OSDU");
    show_mutex.unlock();
}

void CefHbbtvPage::ClearRect(int x, int y, int width, int height) {
    cPoint recPoint(0, 0);

    // create a new image
    cSize trans_size(width, height);
    cImage trans_image(trans_size);
    trans_image.Clear();

    cPoint trans_point(x, y);

    HBBTV_DBG("[hbbtv] Try to get pixmap lock (ClearRect)");
    pixmap->Lock();
    HBBTV_DBG("[hbbtv] Got pixmap lock (ClearRect)");

    pixmap->DrawImage(trans_point, trans_image);
    pixmap->Unlock();
}
