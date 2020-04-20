/**
 *  VDR Skin Plugin which uses CEF OSR as rendering engine
 *
 *  browser.cpp
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
#include "browser.h"
#include "hbbtvservice.h"
#include "hbbtvvideocontrol.h"

bool DumpDebugData = true;

// FULL HD
#define SHM_BUF_SIZE (1920 * 1080 * 4)
#define SHM_KEY 0xDEADC0DE

Browser *browser;

Browser::Browser() {
    // init shared memory
    shmid = -1;
    shmp = nullptr;

    shmid = shmget(SHM_KEY, SHM_BUF_SIZE, 0644 | IPC_CREAT | IPC_EXCL) ;

    if (errno == EEXIST) {
        shmid = shmget(SHM_KEY, SHM_BUF_SIZE, 0644);
    }

    if (shmid == -1) {
        perror("Unable to get shared memory");
        return;
    }

    shmp = (uint8_t *) shmat(shmid, NULL, 0);
    if (shmp == (void *) -1) {
        perror("Unable to attach to shared memory");
        return;
    }

    // update thread
    browser = this;
    showBrowser();
}

Browser::~Browser() {
    // hide only, of player is not attached
    if (player != nullptr && !player->IsAttached()) {
        hideBrowser();
    }

    osd->DestroyPixmap(pixmap);
    pixmap = nullptr;

    browser = nullptr;

    if (osd != nullptr) {
        delete osd;
        osd = nullptr;
    }

    if (shmdt(shmp) == -1) {
        perror("Unable to detach from shared memory");
        return;
    }

    shmp = nullptr;

    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        // Either this process or VDR removes the shared memory
    }
}

bool Browser::loadPage(std::string url, int rootFontSize) {
    std::string cmdUrl("URL ");
    cmdUrl.append(url);

    browserComm->SendToBrowser(cmdUrl.c_str());

    if (rootFontSize > 0) {
        setRootFontSize(rootFontSize);
    }

    return true;
}

bool Browser::hideBrowser() {
    return browserComm->SendToBrowser("PAUSE");
}

bool Browser::showBrowser() {
    return browserComm->SendToBrowser("RESUME");
}

void Browser::callRawJavascript(cString script) {
    char *cmd;
    asprintf(&cmd, "JS %s", *script);
    browserComm->SendToBrowser(cmd);
    free(cmd);
}

bool Browser::setBrowserSize(int width, int height) {
    char *cmd;

    osdWidth = width;
    osdHeight = height;

    hideBrowser();

    asprintf(&cmd, "SIZE %d %d", osdWidth, osdHeight);
    auto result = browserComm->SendToBrowser(cmd);
    free(cmd);

    showBrowser();

    return result;
}

bool Browser::setZoomLevel(double zoom) {
    char *cmd;

    asprintf(&cmd, "ZOOM %f", zoom);
    auto result = browserComm->SendToBrowser(cmd);
    free(cmd);

    return result;
}

bool Browser::setRootFontSize(int px) {
    char *cmd;

    asprintf(&cmd, "document.getElementsByTagName('html').item(0).style.fontSize = '%dpx'", px);
    callRawJavascript(cmd);
    free(cmd);

    return true;
}


void Browser::createOsd(int left, int top, int width, int height) {
    setBrowserSize(width, height);

    // try to calculate an appropriate zoom level
    // Full HD is 1920 x 1080 = 2073600 Pixel
    auto newPixel = (double)width * (double)height;
    auto zoom = sqrt(newPixel / 2073600.0);
    setZoomLevel(zoom);

    osd = cOsdProvider::NewOsd(left, top);

    tArea Area = { 0, 0, width - 1, height - 1, 32 };
    osd->SetAreas(&Area, 1);

    cRect rect(0, 0, width, height);
    pixmap = osd->CreatePixmap(0, rect, rect);
}

void Browser::FlushOsd() {
    osd->Flush();
}

void Browser::readOsdUpdate(int socketId) {
    int bytes;
    char *buf;
    if ((bytes = nn_recv(socketId, &buf, NN_MSG, 0)) > 0) {
        if (strncmp(buf, "OSDU", 4) != 0) {
            fprintf(stderr, "Internal error. Expected command OSDU, but got '%s'\n", buf);
            browserComm->SendToBrowser("OSDU");
            return;
        }

        int w, h;
        nn_recv(socketId, &w, sizeof(w), 0);
        nn_recv(socketId, &h, sizeof(h), 0);

        // sanity check
        if (w > 1920 || h > 1080 || w <= 0 || h <= 0) {
            // there is some garbage in the shared memory => ignore
            browserComm->SendToBrowser("OSDU");
            return;
        }

        // create image from input data
        cSize recImageSize(w, h);
        cPoint recPoint(0, 0);
        const cImage recImage(recImageSize);
        auto *data2 = const_cast<tColor *>(recImage.Data());

        if (shmp != nullptr) {
            memcpy(data2, shmp, w * h * 4);
        }

        if (pixmap != nullptr) {
            pixmap->DrawImage(recPoint, recImage);
            browser->osd->Flush();
        }

        browserComm->SendToBrowser("OSDU");
    }
}