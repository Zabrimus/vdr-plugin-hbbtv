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

#include "cefhbbtvpage.h"

CefHbbtvPage::CefHbbtvPage() {
    fprintf(stderr, "Construct HbbtvPage...\n");

    browser = new Browser();

}

CefHbbtvPage::~CefHbbtvPage() {
    fprintf(stderr, "Destroy HbbtvPage...\n");
    delete browser;
}

void CefHbbtvPage::Show() {
    osd = browser->GetOsd();
    browser->createOsd(0, 0, 1920, 1080);
    cOsdObject::Show();
}

void CefHbbtvPage::Hide() {
    browser->hideBrowser();
}

void CefHbbtvPage::LoadUrl(std::string _hbbtvUrl) {
    hbbtvUrl = _hbbtvUrl;
    browser->setBrowserSize(1920, 1080);
    browser->setHbbtvMode();
    browser->loadPage(hbbtvUrl, 0);
}

eOSState CefHbbtvPage::ProcessKey(eKeys Key) {
    eOSState state = cOsdObject::ProcessKey(Key);

    if (state == osUnknown) {
        if (Key == kBack) {
            browser->hideBrowser();
            return osEnd;
        }

        bool result = browserComm->SendKey(Key);
        if (result) {
            return osContinue;
        }
    }

    return state;
}


