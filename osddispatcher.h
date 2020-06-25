#ifndef _OSDDISPATCHER_H
#define _OSDDISPATCHER_H

#include <string>
#include <vdr/osdbase.h>

enum OSDType { MENU, HBBTV, CLOSE, REOPEN };

class OsdDispatcher {
public:
    static char* hbbtvUrl;
    static OSDType osdType;

    OsdDispatcher();
    cOsdObject* get(const char *title, const char *name);
};

#endif // _OSDDISPATCHER_H
