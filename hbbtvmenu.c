
/*
 * hbbtvmenu.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include "hbbtvmenu.h"
#include <vdr/menuitems.h>
#include <vdr/tools.h>
#include <vdr/device.h>

#define BROWSER "/usr/bin/firefox"
#define DEBUG

#ifdef DEBUG
#       define DSYSLOG(x...)    dsyslog(x);
#else
#       define DSYSLOG(x...)    
#endif


static const char *CtrlCodes[8] =
{  "Autostart",
   "Present",
   "Destroy",
   "Kill",
   "Prefetch",
   "Remote",
   "Disabled",
   "Playback_autostart"
};


cHbbtvMenu::cHbbtvMenu(const char *title, int c0, int c1, int c2, int c3, int c4)
:cOsdMenu(title, 4, 3, 2, 13, c4)
{
   hbbtvURLs = (cHbbtvURLs *)cHbbtvURLs::HbbtvURLs();
   SetHelp("Refresh");
}

cHbbtvMenu::~cHbbtvMenu()
{
}

void cHbbtvMenu::Display(void)
{
   int current;

   current = Current();
   Clear();

   SetTitle(*cString::sprintf("HbbTV URLs - Anzahl: %d", hbbtvURLs->Count()));
   DSYSLOG("[hbbtv] URL List %d entries --------------------------------------------", hbbtvURLs->Count());
   Add (new cOsdItem(*cString::sprintf("CtlC\tAID\tPri\tName\tURL")));
   //hbbtvURLs->Sort();
   for (cHbbtvURL *url = hbbtvURLs->First(); url; url = hbbtvURLs->Next(url)) {
      Add(new cOsdItem(*cString::sprintf("%s\t%d\t%d\t%s \t%s%s",  
          url->ControlCode() == 1 ? "Auto" : url->ControlCode() == 2 ? "Pres" : "unknown", url->ApplicationId(), url->Priority(),
          *url->Name(),  *url->UrlBase(),  *url->UrlLoc())));
      DSYSLOG("[hbbtv] CtrlCode=%-9s AppID=%2d Prio=%2d Name=%s URL=%s%s", CtrlCodes[url->ControlCode()-1], url->ApplicationId(), 
              url->Priority(), *url->Name(), *url->UrlBase(),  *url->UrlLoc());
   }
   SetCurrent(Get(current));
   cOsdMenu::Display();
}


eOSState cHbbtvMenu::ProcessKey(eKeys Key)
{
   eOSState state = cOsdMenu::ProcessKey(Key);

   if (state == osUnknown) {
      switch (Key) {
       case kOk:     {
                        cHbbtvURL *url = hbbtvURLs->Get(Current()-1);
                        if (url) 
                        {
                           DSYSLOG("Menuitem: %d %s", Current(), *cString::sprintf("DISPLAY=:0 %s %s%s", BROWSER, *url->UrlBase(),  *url->UrlLoc()));
                           SystemExec(*cString::sprintf("DISPLAY=:0 %s %s%s", BROWSER, *url->UrlBase(),  *url->UrlLoc()), true);
                        }
                        return osContinue;
                     }
       case kRed:    Display();
                     break; 
       case kGreen:  break;
       case kYellow: break;
       case kBlue:   break;
       case kUp:     cDevice::SwitchChannel(1);
                     break;
       case kDown:   cDevice::SwitchChannel(-1);
                     break;
                     
       default: break;
      }
   }

   return state;
}

  
