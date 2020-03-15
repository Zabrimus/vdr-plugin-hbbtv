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

bool DumpDebugData = true;

Browser *browser;

Browser::Browser() {
    // update thread
    browser = this;
    showBrowser();
}

Browser::~Browser() {
    hideBrowser();

    browser = nullptr;

    if (osd != nullptr) {
        delete osd;
        osd = nullptr;
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

bool Browser::sendKeyEvent(cString key) {
    char *cmd;

    asprintf(&cmd, "KEY %s", *key);
    auto result = browserComm->SendToBrowser(cmd);
    free(cmd);

    return result;
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
    unsigned long dirtyRecs = 0;
    if ((bytes = nn_recv(socketId, &dirtyRecs, sizeof(dirtyRecs), 0)) > 0) {
        // sanity check: If dirtyRecs > 20 then ignore this
        if (dirtyRecs > 20) {
            // FIXME: Try to clear the input buffer to get a new valid state
            return;
        }

        for (unsigned long i = 0; i < dirtyRecs; ++i) {
            // read coordinates and size
            int x, y, w, h;
            if ((bytes = nn_recv(socketId, &x, sizeof(x), 0)) > 0) {
            }

            if ((bytes = nn_recv(socketId, &y, sizeof(y), 0)) > 0) {
            }

            if ((bytes = nn_recv(socketId, &w, sizeof(w), 0)) > 0) {
            }

            if ((bytes = nn_recv(socketId, &h, sizeof(h), 0)) > 0) {
            }

            // dsyslog("Received dirty rec: (x %d, y %d) -> (w %d, h %d)\n", x, y, w, h);
            // dbgbrowser("Received dirty rec: (x %d, y %d) -> (w %d, h %d)\n", x, y, w, h);

            // create image from input data
            cSize recImageSize(w, h);
            cPoint recPoint(x, y);
            const cImage recImage(recImageSize);
            auto *data2 = const_cast<tColor *>(recImage.Data());

            for (int j = 0; j < h; ++j) {
                if ((bytes = nn_recv(socketId, data2 + (w * j), 4 * w, 0)) > 0) {
                    // everything is fine
                } else {
                    // TODO: Und nun?
                }
            }

            pixmap->DrawImage(recPoint, recImage);

            browser->osd->Flush();
        }
    }
}