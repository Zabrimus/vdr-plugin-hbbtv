/*
 * hbbtv.h: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#ifndef HBBTV_H
#define HBBTV_H

#include <string>
#include <ctime>
#include <thread>
#include <atomic>
#include <vdr/plugin.h>
#include <vdr/device.h>
#include "osddispatcher.h"
#include "browsercommunication.h"

class cPluginHbbtv : public cPlugin {
private:
    bool showPlayer;
    OsdDispatcher *osdDispatcher;

    bool         OsrBrowserStart;
    std::string  OsrBrowserPath;
    std::string  OsrBrowserCmdLine;
    std::string  OsrBrowserLogFile;
    std::string  OsrBrowserDisplay;

    int lastDisplayWidth = 0;
    int lastDisplayHeight = 0;

    cString currentUrlChannel;

    std::thread urlwriter_thread;
    bool urlwriter_running;
    void WriteUrlsToFile();

    bool startVdrOsrBrowser();
    void stopVdrOsrBrowser();

    void ShowPlayer();
    void HidePlayer();

public:
    cPluginHbbtv(void);
    virtual ~cPluginHbbtv();
    virtual const char *Version(void);
    virtual const char *Description(void);
    virtual bool Start(void);
    virtual void Stop(void);
    virtual bool Initialize(void);
    virtual const char *MainMenuEntry(void);
    virtual cOsdObject *MainMenuAction(void);
    virtual void MainThreadHook(void);
    virtual bool Service(const char *Id, void *Data);
    virtual const char *CommandLineHelp(void);
    virtual bool ProcessArgs(int argc, char *argv[]);
    virtual const char **SVDRPHelpPages(void);
    virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
};

#endif // HBBTV_H
