/*
 * hbbtvmenu.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
 
#include <vdr/osd.h>
#include <vdr/menu.h>
#include <vdr/tools.h>
#include "hbbtvurl.h"
#include "cefhbbtvpage.h"
#include "globals.h"

class cHbbtvMainMenu:public cOsdMenu {
    private:
        void read_directory(const cString& dir, cStringList& v);

    public:
        cHbbtvMainMenu(const char *title);
        virtual ~cHbbtvMainMenu();
        virtual eOSState ProcessKey(eKeys);
};

class cHbbtvUrlListMenu:public cOsdMenu {
    private:
  	    cHbbtvURLs *hbbtvURLs;
        void Display(void);

    public:
        cHbbtvUrlListMenu(const char *, int = 0, int = 0, int = 0, int = 0, int = 0);
        virtual ~cHbbtvUrlListMenu();
        virtual eOSState ProcessKey(eKeys);
};

class cHbbtvBookmarkMenu:public cOsdMenu {
    private:
        cStringList apps;

    public:
        cHbbtvBookmarkMenu(const char * title, const char * filename);
        virtual ~cHbbtvBookmarkMenu();
        virtual eOSState ProcessKey(eKeys);
};

class cHbbtvBrowserMenu:public cOsdMenu {
    public:
        cHbbtvBrowserMenu(const char * title);
        virtual ~cHbbtvBrowserMenu();
        virtual eOSState ProcessKey(eKeys);
};