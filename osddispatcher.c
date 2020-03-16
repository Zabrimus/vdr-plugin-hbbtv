#include "osddispatcher.h"
#include "hbbtvmenu.h"
#include "cefhbbtvpage.h"

char* OsdDispatcher::hbbtvUrl = NULL;
OSDType OsdDispatcher::osdType = OSDType::MENU;

OsdDispatcher::OsdDispatcher() {
}

cOsdObject* OsdDispatcher::get(const char *title) {
    if (osdType == MENU) {
        return new cHbbtvMenu(title);
    } else if (osdType == HBBTV) {
        CefHbbtvPage *page = new CefHbbtvPage();
        page->LoadUrl(hbbtvUrl);
        OsdDispatcher::osdType = OSDType::MENU;

        return page;
    } else if (osdType == CLOSE) {
        // close OSD
        osdType = MENU;
        return NULL;
    } else {
        // must not happen
        return NULL;
    }
}
