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

CefHbbtvPage *hbbtvPage;

struct SwsContext *swsCtx = nullptr;

CefHbbtvPage::CefHbbtvPage() {
    fprintf(stderr, "Construct HbbtvPage...\n");

    osd = nullptr;
    pixmap = nullptr;
    hbbtvPage = this;
    resizeOsd = false;
}

CefHbbtvPage::~CefHbbtvPage() {
    fprintf(stderr, "Destroy HbbtvPage...\n");

    // hide only, of player is not attached
    if (player != nullptr && !player->IsAttached()) {
        hideBrowser();
    }

    if (osd != nullptr) {
        delete osd;
        osd = nullptr;
    }

    pixmap = nullptr;
    hbbtvPage = nullptr;
}

void CefHbbtvPage::Show() {
    // osd = cOsdProvider::NewOsd(0, 0, 1);
    osd = cOsdProvider::NewOsd(0, 0);

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

    SetOsdSize();
    browserComm->SendToBrowser("SENDOSD");
}

void CefHbbtvPage::TriggerOsdResize() {
    resizeOsd = true;
    browserComm->SendToBrowser("SENDOSD");
}

void CefHbbtvPage::SetOsdSize() {
    if (pixmap != nullptr) {
        osd->DestroyPixmap(pixmap);
        pixmap = nullptr;
    }

    double ph;
    cDevice::PrimaryDevice()->GetOsdSize(disp_width, disp_height, ph);
    cRect rect(0, 0, disp_width, disp_height);

    show_mutex.lock();

    // try to get a pixmap
    pixmap = osd->CreatePixmap(0, rect, rect);

    pixmap->Lock();
    pixmap->Clear();
    pixmap->Unlock();

    show_mutex.unlock();
}

eOSState CefHbbtvPage::ProcessKey(eKeys Key) {
    eOSState state = cOsdObject::ProcessKey(Key);

    if (state == osUnknown) {
        if (Key == kBack) {
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
    showBrowser();

    hbbtvUrl = url;
    setHbbtvMode();

    std::string cmdUrl("URL ");
    cmdUrl.append(url);

    browserComm->SendToBrowser(cmdUrl.c_str());

    return true;
}

bool CefHbbtvPage::hideBrowser() {
    fprintf(stderr, "Hide Browser\n");
    return browserComm->SendToBrowser("PAUSE");
}

bool CefHbbtvPage::showBrowser() {
    fprintf(stderr, "Show Browser\n");
    return browserComm->SendToBrowser("RESUME");
}

void CefHbbtvPage::readOsdUpdate(OsdStruct* osdUpdate) {
    if (resizeOsd) {
        SetOsdSize();
        resizeOsd = false;
    }

    show_mutex.lock();

    if (strncmp(osdUpdate->message, "OSDU", 4) != 0) {
        // Internal error. Expected command OSDU, but got something else
        browserComm->SendToBrowser("OSDU");
        show_mutex.unlock();
        return;
    }

    // sanity check
    if (osdUpdate->width > 1920 || osdUpdate->height > 1080 || osdUpdate->width <= 0 || osdUpdate->height <= 0) {
        // there is some garbage in the shared memory => ignore
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
    swsCtx = sws_getCachedContext(swsCtx,
                                  osdUpdate->width, osdUpdate->height, AV_PIX_FMT_BGRA,
                                  disp_width, disp_height, AV_PIX_FMT_BGRA,
                                  SWS_BILINEAR, NULL, NULL, NULL);

    uint8_t *inData[1] = { osd_shm.get() };
    int inLinesize[1] = { 4 * osdUpdate->width };
    int outLinesize[1] = { 4 * disp_width };

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
        pixmap->Lock();
        pixmap->DrawImage(recPoint, recImage);
        pixmap->Unlock();
    }

    osd->Flush();
    browserComm->SendToBrowser("OSDU");
    show_mutex.unlock();
}
