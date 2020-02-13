#ifndef _OSDDISPATCHER_H
#define _OSDDISPATCHER_H

#include <string>
#include <vdr/osdbase.h>

class OsdDispatcher {
public:
    static char* hbbtvUrl;
    static bool showMenu;

    OsdDispatcher();
    cOsdObject* get(const char *title);
};

#endif // _OSDDISPATCHER_H
