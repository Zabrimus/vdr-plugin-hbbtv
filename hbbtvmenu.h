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

class cHbbtvMenu:public cOsdMenu
{
  private:
  	cHbbtvURLs *hbbtvURLs;
    void Display(void);

  public:
    cHbbtvMenu(const char *, int = 0, int = 0, int = 0, int = 0, int = 0);
    virtual ~cHbbtvMenu();
    virtual eOSState ProcessKey(eKeys);
};
