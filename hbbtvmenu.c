/*
 * hbbtvmenu.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include "hbbtvmenu.h"
#include "cefhbbtvpage.h"
#include "osddispatcher.h"
#include "hbbtvservice.h"
#include "globals.h"
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

cHbbtvMainMenu::cHbbtvMainMenu(const char *title) : cOsdMenu(title) {
    Clear();

    // Fixed Menu: Red Button
    cOsdMenu::Add(new cOsdItem(tr("Red Button")));

    // create dynamic menu
    cStringList urllists;
    read_directory(pluginConfigDirectory, urllists);

    for (int i = 0; i < urllists.Size(); i++) {
        cOsdMenu::Add(new cOsdItem(urllists[i]));
    }

    // Fixed Menu: Channel URL list
    cOsdMenu::Add(new cOsdItem(tr("Channel URL list")));

    // Fixed Menu: All found hbbtv URLs
    cOsdMenu::Add(new cOsdItem(tr("All applications")));

    // Fixed Menu: Browser control
    cOsdMenu::Add(new cOsdItem(tr("Browser")));

    // Fixed Menu: Reopen OSD
    cOsdMenu::Add(new cOsdItem(tr("Reopen OSD")));

    SetHelp(0, 0, 0,0);
    Display();
}

cHbbtvMainMenu::~cHbbtvMainMenu() {

}

void cHbbtvMainMenu::read_directory(const cString& dir, cStringList& v) {
    struct dirent **namelist;
    int n;

    n = scandir(*dir, &namelist, NULL, alphasort);
    if (n == -1) {
        return;
    }

    while (n--) {
        if (strncmp(namelist[n]->d_name, "menu_", 5) == 0) {
            char *name = strstr(namelist[n]->d_name + 5, "_");
            if (name != nullptr) {
                v.InsertUnique(strdup(name+1));
            }
        }
        free(namelist[n]);
    }
    free(namelist);
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
            } else if (Current() == Count() - 4) {
                // All URLs from current channel
                return AddSubMenu(new cHbbtvUrlListMenu(tr("HbbTV URLs")));
            } else if (Current() == Count() - 3) {
                return AddSubMenu(new cHbbtvBookmarkMenu(tr("All applications"), "hbbtv_all_urls.list"));
            } else if (Current() == Count() - 2) {
                // Browser control
                return AddSubMenu(new cHbbtvBrowserMenu(tr("Browser")));
            } else if (Current() == Count() - 1) {
                // Reopen OSD
                OsdDispatcher::osdType = OSDType::REOPEN;
                return osPlugin;
            } else {
                // Categorized URL list

                // search filename and create OSD entry
                cHbbtvBookmarkMenu * newMenu;
                DIR* dirp = opendir(pluginConfigDirectory);

                struct dirent* dp;
                while ((dp = readdir(dirp)) != nullptr) {
                    if (strncmp(dp->d_name, "menu_", 5) == 0) {
                        char *name = strstr(dp->d_name + 5, "_");
                        if (name != nullptr && strstr(name+1, cOsdMenu::Get(Current())->cOsdItem::Text()) != nullptr) {
                            newMenu = new cHbbtvBookmarkMenu(cOsdMenu::Get(Current())->cOsdItem::Text(), dp->d_name);
                        }
                    }
                }
                closedir(dirp);

                return AddSubMenu(newMenu);
            }

            return osEnd;
        }

        default:
            break;
    }

    return state;
}

cHbbtvBookmarkMenu::cHbbtvBookmarkMenu(const char * title, const char * filename) : cOsdMenu(title) {
        Clear();

        SetCols(20);

        // read all applications
        char *urlFileName;
        asprintf(&urlFileName, "%s/%s", pluginConfigDirectory, filename);

        FILE *urlFile = fopen(urlFileName, "r");
        int bufferLength = 1024;
        char buffer[bufferLength];

        if (urlFile) {
            while (fgets(buffer, bufferLength, urlFile)) {
                strtok(buffer, "\n");
                apps.AppendUnique(strdup(buffer));
            }

            fclose(urlFile);
        }

        free(urlFileName);

        for (int i = 0; i < apps.Size(); ++i) {
            char *app = apps[i];

            std::string channel;
            std::string name;

            size_t pos = 0;
            std::string token;

            std::string s(app);
            int idx = 0;
            while ((pos = s.find(":")) != std::string::npos) {
                token = s.substr(0, pos);

                if (token.length() > 0) {
                    switch (idx) {
                        case 0:
                            channel = std::string(token);
                            break;

                        case 3:
                            name = std::string(token);
                            break;

                        default:
                            break;
                    }
                }
                s.erase(0, pos + 1);
                idx++;
            }

            cOsdMenu::Add(new cOsdItem((channel + "\t" + name).c_str()));
        }

        SetHelp(0, 0, 0,0);
        Display();
};

cHbbtvBookmarkMenu::~cHbbtvBookmarkMenu() {
}

eOSState cHbbtvBookmarkMenu::ProcessKey(eKeys key) {
    eOSState state = cOsdMenu::ProcessKey(key);

    if (state != osUnknown)
        return state;

    switch (key) {
        case kOk: {
            char *u = apps[Current()];

            std::string channel;
            std::string name;
            std::string dvb;
            std::string appid;
            std::string url;

            size_t pos = 0;
            std::string token;

            // find channel name
            std::string s(u);
            pos = s.find(":");
            channel = s.substr(0, pos);
            s.erase(0, pos + 1);

            // find dvb triple
            pos = s.find(":");
            dvb = s.substr(0, pos);
            s.erase(0, pos + 1);

            // send channel information to browser
            int tid;
            int sid;
            int nid;
            int ret = std::sscanf(dvb.c_str(), "%d.%d.%d", &nid, &tid, &sid);

            sendChannelToBrowserData(channel.c_str(), nid, tid, sid, 0);

            // find all apps and urls for this channel
            for (int i = 0; i < apps.Size(); ++i) {
                char *app = apps[i];
                if ((strncmp(app, channel.c_str(), channel.length()) == 0) && (*(app + channel.length()) == ':'))  {
                    std::string s2(app);

                    int idx = 0;
                    while ((pos = s2.find(":")) != std::string::npos) {
                        token = s2.substr(0, pos);

                        if (token.length() > 0) {
                            switch (idx) {
                                case 0:
                                case 1:
                                    break;

                                case 2:
                                    appid = std::string(token);
                                    break;

                                case 3:
                                    name = std::string(token);
                                    break;

                                case 4:
                                    url = std::string(s2);
                                    break;

                                default:
                                    break;
                            }
                        }
                        s2.erase(0, pos + 1);
                        idx++;
                    }

                    // send app ids to browser
                    char *cmd;
                    asprintf(&cmd, "APPURL %02X:%s", atoi(appid.c_str()), url.c_str());
                    browserComm->SendToBrowser(cmd);
                    free(cmd);
                }
            }

            // load desired URL
            if (OsdDispatcher::hbbtvUrl != NULL) {
                free(OsdDispatcher::hbbtvUrl);
                OsdDispatcher::hbbtvUrl = NULL;
            }

            OsdDispatcher::hbbtvUrl = strdup(url.c_str());
            OsdDispatcher::osdType = OSDType::HBBTV;

            return osPlugin;
        }

        default:
            break;
    }

    return state;
}



cHbbtvBrowserMenu::cHbbtvBrowserMenu(const char * title) : cOsdMenu(title) {
    Clear();

    cOsdMenu::Add(new cOsdItem(tr("Ping Browser")));
    cOsdMenu::Add(new cOsdItem(tr("Stop Browser")));
    cOsdMenu::Add(new cOsdItem(tr("Start Browser")));
    cOsdMenu::Add(new cOsdItem(tr("Restart Browser")));

    SetHelp(0, 0, 0,0);
    Display();
}

cHbbtvBrowserMenu::~cHbbtvBrowserMenu() {
}

eOSState cHbbtvBrowserMenu::ProcessKey(eKeys key) {
    eOSState state = cOsdMenu::ProcessKey(key);

    if (state != osUnknown)
        return state;

    switch (key) {
        case kOk: {
            BrowserStatus_v1_0 status;
            switch (Current()) {

                // try to restart the browser
                case 0:
                    // Ping Browser
                    if (browserComm->Heartbeat()) {
                        Skins.Message(mtInfo, tr("Browser is alive and running."));
                    } else {
                        Skins.Message(mtError, tr("Browser is not running!"));
                    }
                    break;

                case 1:
                    // Stop Browser
                    status.message = cString("STOP_BROWSER");
                    cPluginManager::CallAllServices("BrowserStatus-1.0", &status);

                    Skins.Message(mtInfo, tr("Browser will be stopped."));
                    break;

                case 2:
                    // Start Browser
                    status.message = cString("START_BROWSER");
                    cPluginManager::CallAllServices("BrowserStatus-1.0", &status);

                    Skins.Message(mtInfo, tr("Browser will be started."));
                    break;

                case 3:
                    // Restart Browser
                    status.message = cString("RESTART_BROWSER");
                    cPluginManager::CallAllServices("BrowserStatus-1.0", &status);

                    Skins.Message(mtInfo, tr("Browser will be restarted."));
                    break;
            }

            return osEnd;
        }

        default:
            break;
    }

    return state;
}