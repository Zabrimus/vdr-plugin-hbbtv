/*
 * hbbtvmenu.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
 
#include <vdr/osd.h>
#include <vdr/menu.h>
#include "hbbtvurl.h"
#include "cefhbbtvpage.h"

class cHbbtvMainMenu:public cOsdMenu {
    private:
        void read_directory(const cString& dir, cStringList& v);
        void read_urlfile(const cString& dir, const cString& name, cList<cHbbtvURL>* v);

        cString pluginName;

    public:
        cHbbtvMainMenu(const char *title, const char *name);
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
        cList<cHbbtvURL> *urls;

    public:
        cHbbtvBookmarkMenu(const char * title, cList<cHbbtvURL> *urls);
        virtual ~cHbbtvBookmarkMenu();
        virtual eOSState ProcessKey(eKeys);
};
