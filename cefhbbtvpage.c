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
    initKeyMapping();
}

CefHbbtvPage::~CefHbbtvPage() {
    fprintf(stderr, "Destroy HbbtvPage...\n");
    delete browser;
}

void CefHbbtvPage::Show() {
    osd = browser->GetOsd();
    browser->startUpdate(0, 0, 1920, 1080);
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

        std::map<eKeys, std::string>::iterator it;

        it = keyMapping.find(Key);
        if (it != keyMapping.end()) {
            SendKey(it->second);
            return osContinue;
        }
    }

    return state;
}

void CefHbbtvPage::SendKey(std::string key) {
    browser->sendKeyEvent(cString(key.c_str()));
}

void CefHbbtvPage::initKeyMapping() {
    keyMapping.insert(std::pair<eKeys, std::string>(kUp,      "VK_UP"));
    keyMapping.insert(std::pair<eKeys, std::string>(kDown,    "VK_DOWN"));
    keyMapping.insert(std::pair<eKeys, std::string>(kLeft,    "VK_LEFT"));
    keyMapping.insert(std::pair<eKeys, std::string>(kRight,   "VK_RIGHT"));

    // keyMapping.insert(std::pair<eKeys, std::string>(???,   "VK_PAGE_UP"));
    // keyMapping.insert(std::pair<eKeys, std::string>(???,   "VK_PAGE_DOWN"));

    keyMapping.insert(std::pair<eKeys, std::string>(kOk,      "VK_ENTER"));

    keyMapping.insert(std::pair<eKeys, std::string>(kRed,     "VK_RED"));
    keyMapping.insert(std::pair<eKeys, std::string>(kGreen,   "VK_GREEN"));
    keyMapping.insert(std::pair<eKeys, std::string>(kYellow,  "VK_YELLOW"));
    keyMapping.insert(std::pair<eKeys, std::string>(kBlue,    "VK_BLUE"));

    keyMapping.insert(std::pair<eKeys, std::string>(kPlay,    "VK_PLAY"));
    keyMapping.insert(std::pair<eKeys, std::string>(kPause,   "VK_PAUSE"));
    keyMapping.insert(std::pair<eKeys, std::string>(kStop,    "VK_STOP"));
    keyMapping.insert(std::pair<eKeys, std::string>(kFastFwd, "VK_FAST_FWD"));
    keyMapping.insert(std::pair<eKeys, std::string>(kFastRew, "VK_REWIND"));

    keyMapping.insert(std::pair<eKeys, std::string>(k0,       "VK_0"));
    keyMapping.insert(std::pair<eKeys, std::string>(k1,       "VK_1"));
    keyMapping.insert(std::pair<eKeys, std::string>(k2,       "VK_2"));
    keyMapping.insert(std::pair<eKeys, std::string>(k3,       "VK_3"));
    keyMapping.insert(std::pair<eKeys, std::string>(k4,       "VK_4"));
    keyMapping.insert(std::pair<eKeys, std::string>(k5,       "VK_5"));
    keyMapping.insert(std::pair<eKeys, std::string>(k6,       "VK_6"));
    keyMapping.insert(std::pair<eKeys, std::string>(k7,       "VK_7"));
    keyMapping.insert(std::pair<eKeys, std::string>(k8,       "VK_8"));
    keyMapping.insert(std::pair<eKeys, std::string>(k9,       "VK_9"));
}

