#include "osddispatcher.h"
#include "hbbtvmenu.h"
#include "cefhbbtvpage.h"

char* OsdDispatcher::hbbtvUrl = NULL;
bool OsdDispatcher::showMenu = true;

OsdDispatcher::OsdDispatcher() {
}

cOsdObject* OsdDispatcher::get(const char *title) {
    if (showMenu) {
        return new cHbbtvMenu(title);
    } else {
        CefHbbtvPage *page = new CefHbbtvPage();
        page->LoadUrl(hbbtvUrl);
        showMenu = true;

        return page;
    }
}
