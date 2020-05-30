
/*
 * hbbtvmenu.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include <sys/types.h>
#include <dirent.h>
#include "hbbtvmenu.h"
#include "cefhbbtvpage.h"
#include "osddispatcher.h"
#include <vdr/menuitems.h>
#include <vdr/tools.h>
#include <vdr/device.h>
#include <vdr/plugin.h>

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


cHbbtvUrlListMenu::cHbbtvUrlListMenu(const char *title, int c0, int c1, int c2, int c3, int c4)
:cOsdMenu(title, 4, 3, 2, 13, c4)
{
   hbbtvURLs = (cHbbtvURLs *)cHbbtvURLs::HbbtvURLs();
   SetHelp("Refresh");
}

cHbbtvUrlListMenu::~cHbbtvUrlListMenu()
{
}

void cHbbtvUrlListMenu::Display(void)
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


eOSState cHbbtvUrlListMenu::ProcessKey(eKeys Key)
{
   eOSState state = cOsdMenu::ProcessKey(Key);

   if (state == osUnknown) {
      switch (Key) {
       case kOk:     {
                        cHbbtvURL *url = hbbtvURLs->Get(Current()-1);
                        if (url) 
                        {
                           DSYSLOG("[hbbtv] Menuitem: %d %s", Current(), *cString::sprintf("DISPLAY=:0 %s %s%s", BROWSER, *url->UrlBase(),  *url->UrlLoc()));
                           char *mainUrl;
                           asprintf(&mainUrl,"%s%s", *url->UrlBase(), *url->UrlLoc());

                           if (OsdDispatcher::hbbtvUrl != NULL) {
                               free(OsdDispatcher::hbbtvUrl);
                               OsdDispatcher::hbbtvUrl = NULL;
                           }

                           OsdDispatcher::hbbtvUrl = mainUrl;
                           OsdDispatcher::osdType = OSDType::HBBTV;
                        }
                        return osPlugin;
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

cHbbtvMainMenu::cHbbtvMainMenu(const char *title, const char *name) : cOsdMenu(title) {
    pluginName = name;

    Clear();

    // Fixed Menu: Red Button
    cOsdMenu::Add(new cOsdItem(tr("Red Button")));

    // create dynamic menu
    cStringList urllists;
    read_directory(cPlugin::ConfigDirectory(pluginName), urllists);

    for (int i = 0; i < urllists.Size(); i++) {
        cOsdMenu::Add(new cOsdItem(urllists[i]));
    }

    // Fixed Menu: Channel URL list
    cOsdMenu::Add(new cOsdItem(tr("Channel URL list")));

    SetHelp(0, 0, 0,0);
    Display();
}

cHbbtvMainMenu::~cHbbtvMainMenu() {

}

void cHbbtvMainMenu::read_directory(const cString& dir, cStringList& v) {
    DIR* dirp = opendir(*dir);

    struct dirent* dp;
    while ((dp = readdir(dirp)) != nullptr) {
        if (strncmp(dp->d_name, "menu_", 5) == 0) {
            char *name = strstr(dp->d_name + 5, "_");
            if (name != nullptr) {
                v.InsertUnique(strdup(name+1));
            }
        }
    }
    closedir(dirp);
}

void cHbbtvMainMenu::read_urlfile(const cString& dir, const cString& name, cList<cHbbtvURL>* v) {
    DIR* dirp = opendir(*dir);

    struct dirent* dp;
    while ((dp = readdir(dirp)) != nullptr) {
        if (strncmp(dp->d_name, "menu_", 5) == 0) {
            if (strstr(dp->d_name + 5, *name) != 0) {
                char *urlFileName;
                asprintf(&urlFileName, "%s/%s", *dir, dp->d_name);

                FILE *urlFile = fopen(urlFileName, "r");
                int bufferLength = 1024;
                char buffer[bufferLength];

                if (urlFile) {
                    while(fgets(buffer, bufferLength, urlFile)) {
                        strtok(buffer, "\n");
                        cHbbtvURL *url = cHbbtvURL::FromString(buffer);
                        v->Add(url);
                    }
                    fclose(urlFile);
                }

                free(urlFileName);
            }
        }
    }
    closedir(dirp);
}

eOSState cHbbtvMainMenu::ProcessKey(eKeys key) {
    eOSState state = cOsdMenu::ProcessKey(key);

    if (state != osUnknown)
        return state;

    switch (key) {
        case kOk: {
            if (Current() == 0) {
                // Red Button
                cHbbtvURLs *urls = (cHbbtvURLs *)cHbbtvURLs::HbbtvURLs();
                for (int i = 0; i < urls->Count(); ++i) {
                    cHbbtvURL *url = urls->Get(i);
                    if (url->ControlCode() == 1) {
                        char *mainUrl;
                        asprintf(&mainUrl,"%s%s", *url->UrlBase(), *url->UrlLoc());
                        if (OsdDispatcher::hbbtvUrl != NULL) {
                            free(OsdDispatcher::hbbtvUrl);
                            OsdDispatcher::hbbtvUrl = NULL;
                        }

                        OsdDispatcher::hbbtvUrl = mainUrl;
                        OsdDispatcher::osdType = OSDType::HBBTV;

                        return osPlugin;
                    }
                }

                Skins.QueueMessage(mtInfo, tr("No HbbTV page found!"));
                return osEnd;
            } else if (Current() == Count() - 1) {
                // All URLs from current channel
                return AddSubMenu(new cHbbtvUrlListMenu("HbbTV URLs"));
            } else {
                // Categorized URL list
                cList<cHbbtvURL> *blist = new cList<cHbbtvURL>();
                read_urlfile(cPlugin::ConfigDirectory(pluginName), cOsdMenu::Get(Current())->cOsdItem::Text(), blist);
                return AddSubMenu(new cHbbtvBookmarkMenu(cOsdMenu::Get(Current())->cOsdItem::Text(), blist));
            }

            return osEnd;
        }

        default:
            break;
    }

    return state;
}

cHbbtvBookmarkMenu::cHbbtvBookmarkMenu(const char * title, cList<cHbbtvURL> *urls) : cOsdMenu(title) {
    this->urls = urls;

    Clear();
    for (int i = 0; i < this->urls->Count(); i++) {
        cOsdMenu::Add(new cOsdItem(this->urls->Get(i)->Name()));
    }

    SetHelp(0, 0, 0,0);
    Display();
};

cHbbtvBookmarkMenu::~cHbbtvBookmarkMenu() {
    if (urls != nullptr) {
        delete urls;
    }
}

eOSState cHbbtvBookmarkMenu::ProcessKey(eKeys key) {
    eOSState state = cOsdMenu::ProcessKey(key);

    if (state != osUnknown)
        return state;

    switch (key) {
        case kOk: {
            cHbbtvURL *url = urls->Get(Current());
            if (url)
            {
                DSYSLOG("[hbbtv] Menuitem: %d %s", Current(), *cString::sprintf("DISPLAY=:0 %s %s%s", BROWSER, *url->UrlBase(),  *url->UrlLoc()));
                char *mainUrl;
                asprintf(&mainUrl,"%s%s", *url->UrlBase(), *url->UrlLoc());

                if (OsdDispatcher::hbbtvUrl != NULL) {
                    free(OsdDispatcher::hbbtvUrl);
                    OsdDispatcher::hbbtvUrl = NULL;
                }

                OsdDispatcher::hbbtvUrl = mainUrl;
                OsdDispatcher::osdType = OSDType::HBBTV;
            }
            return osPlugin;
        }

        default:
            break;
    }

    return state;
}

