#include "osddispatcher.h"
#include "hbbtvmenu.h"
#include "cefhbbtvpage.h"

char* OsdDispatcher::hbbtvUrl = NULL;
OSDType OsdDispatcher::osdType = OSDType::MENU;

OsdDispatcher::OsdDispatcher() {
}

cOsdObject* OsdDispatcher::get(const char *title) {
    if (osdType == MENU) {
        dsyslog("OsdDispatcher: Construct new HbbtvMenu");

        return new cHbbtvMenu(title);
    } else if (osdType == HBBTV) {
        dsyslog("OsdDispatcher: Construct new HbbtvPage");

        CefHbbtvPage *page = new CefHbbtvPage();
        page->loadPage(hbbtvUrl);
        OsdDispatcher::osdType = OSDType::MENU;

        return page;
    } else if (osdType == CLOSE) {
        dsyslog("OsdDispatcher: Close");

        // close OSD
        osdType = MENU;
        return NULL;
    } else {
        dsyslog("OsdDispatcher: Internal error");

        // must not happen
        return NULL;
    }
}
