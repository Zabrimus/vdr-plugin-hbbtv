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

CefHbbtvPage::CefHbbtvPage() {
    fprintf(stderr, "Construct HbbtvPage...\n");

    osd = nullptr;
    pixmap = nullptr;
    hbbtvPage = this;
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
    show_mutex.lock();

    osd = cOsdProvider::NewOsd(0, 0);

    int mw, mh;
    double ph;
    cDevice::PrimaryDevice()->GetOsdSize(mw, mh, ph);

    tArea Area = {0, 0, mw - 1, mh - 1, 32};
    osd->SetAreas(&Area, 1);

    cRect rect(0, 0, mw, mh);
    pixmap = osd->CreatePixmap(1, rect, rect);
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

    pixmap->Lock();

    // create image from input data
    cSize recImageSize(osdUpdate->width, osdUpdate->height);
    cPoint recPoint(0, 0);
    const cImage recImage(recImageSize);
    auto *data2 = const_cast<tColor *>(recImage.Data());

    osd_shm.copyTo(data2, osdUpdate->width * osdUpdate->height * 4);

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

    pixmap->DrawImage(recPoint, recImage);
    pixmap->Unlock();
    osd->Flush();
    browserComm->SendToBrowser("OSDU");

    show_mutex.unlock();
}
