#include "osddispatcher.h"
#include "hbbtvmenu.h"
#include "cefhbbtvpage.h"

char* OsdDispatcher::hbbtvUrl = NULL;
OSDType OsdDispatcher::osdType = OSDType::MENU;

OsdDispatcher::OsdDispatcher() {
}

cOsdObject* OsdDispatcher::get(const char *title, const char *name) {
    if (osdType == MENU) {
        dsyslog("[hbbtv] OsdDispatcher: Construct new HbbtvMenu");

        return new cHbbtvMainMenu(title, name);
    } else if (osdType == HBBTV) {
        dsyslog("[hbbtv] OsdDispatcher: Construct new HbbtvPage");

        CefHbbtvPage *page = new CefHbbtvPage();
        if (!page->loadPage(hbbtvUrl)) {
            OsdDispatcher::osdType = OSDType::CLOSE;
            delete page;
            page = NULL;

            return NULL;
        }

        OsdDispatcher::osdType = OSDType::MENU;

        return page;
    } else if (osdType == CLOSE) {
        dsyslog("[hbbtv] OsdDispatcher: Close");

        // close OSD
        OsdDispatcher::osdType = OSDType::MENU;
        return NULL;
    } else {
        dsyslog("[hbbtv] OsdDispatcher: Internal error");

        // must not happen
        return NULL;
    }
}
