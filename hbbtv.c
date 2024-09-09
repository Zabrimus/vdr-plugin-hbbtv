/*
 * hbbtv.c: A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */
#include <fstream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string>
#include <sys/wait.h>
#include <vdr/skins.h>
#include <vdr/remote.h>
#include <getopt.h>
#include "hbbtv.h"
#include "hbbtvurl.h"
#include "hbbtvmenu.h"
#include "status.h"
#include "hbbtvservice.h"
#include "hbbtvvideocontrol.h"
#include "browsercommunication.h"
#include "cefhbbtvpage.h"
#include "osdshm.h"
#include "globals.h"

static const char *VERSION = "0.1.0pre1";
static const char *DESCRIPTION = "HbbTV Plugin";
static const char *MAINMENUENTRY = "HbbTV";

std::mutex browser_start_mtx;

cHbbtvDeviceStatus *HbbtvDeviceStatus;

void browser_signal_handler(int signum) {
    int stat;

    if (OsrBrowserPid > 0) {
        if (waitpid(OsrBrowserPid, &stat, WNOHANG) == OsrBrowserPid) {
            isyslog("[hbbtv] Browser has been stopped/killed, Signal %d", signum);
        }
    }
}

cPluginHbbtv::cPluginHbbtv(void) {
    // Initialize any member variables here.
    // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
    // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
    HbbtvDeviceStatus = NULL;
    browserComm = NULL;
    showPlayer = false;
    OsrBrowserStart = false;
    osdDispatcher = new OsdDispatcher();

    // set default values for video size (fullscreen)
    video_x = video_y = 0;
    video_width = 1280;
    video_height = 720;

    OsrBrowserPid = -1;
    browserStarted = false;
}

cPluginHbbtv::~cPluginHbbtv() {
    HBBTV_DBG("[hbbtv] Destroy Plugin\n");

    // Clean up after yourself!
    stopVdrOsrBrowser();
    DELETENULL(osdDispatcher);
}

const char *cPluginHbbtv::Version(void) { return VERSION; }

const char *cPluginHbbtv::Description(void) { return DESCRIPTION; }

const char *cPluginHbbtv::MainMenuEntry(void) { return MAINMENUENTRY; }

void cPluginHbbtv::WriteUrlsToFile() {
    char *urlFileName;
    asprintf(&urlFileName, "%s/hbbtv_urls.list", ConfigDirectory(Name()));

    HBBTV_DBG("[hbbtv] Start URL writer thread: %s\n", urlFileName);

    while (urlwriter_running) {
        HBBTV_DBG("[hbbtv] Write URLs to file %s\n", urlFileName);

        // wait 5 minutes
        int i = 0;
        while (urlwriter_running && i < 5 * 60 * 10) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ++i;
        }

        const cStringList *allUrls = cHbbtvURLs::AllURLs();

        FILE *urlFile = fopen(urlFileName, "w");
        if (urlFile) {
            for (int i = 0; i < allUrls->Size(); i++) {
                fprintf(urlFile, "%s\n", (*allUrls)[i]);
            }
            fclose(urlFile);
        }
    }

    free(urlFileName);

    HBBTV_DBG("[hbbtv] Stop URL writer thread\n");
}

bool cPluginHbbtv::Start(void) {
    // Start any background activities the plugin shall perform.
    HBBTV_DBG("[hbbtv] Start Plugin\n");

    pluginConfigDirectory = strdup(cPlugin::ConfigDirectory(Name()));

    // start vdr-osr-browser if configured
    startVdrOsrBrowser();

    browserComm = new BrowserCommunication(Name());
    browserComm->Start();

    // read existing HbbTV URL list
    char *urlFileName;
    asprintf(&urlFileName, "%s/hbbtv_urls.list", ConfigDirectory(Name()));
    cStringList *allURLs = cHbbtvURLs::AllURLs();

    FILE *urlFile = fopen(urlFileName, "r");
    int bufferLength = 1024;
    char buffer[bufferLength];

    if (urlFile) {
        while(fgets(buffer, bufferLength, urlFile)) {
            strtok(buffer, "\n");

            if (allURLs->Find(buffer) == -1 && strlen(buffer) > 3) {
                allURLs->InsertUnique(strdup(buffer));
            }
        }
        fclose(urlFile);
    } else {
        esyslog("Unable to open URL file %s", urlFileName);
    }

    free(urlFileName);

    HbbtvDeviceStatus = new cHbbtvDeviceStatus();

    // start url writer thread
    urlwriter_running = true;
    urlwriter_thread = std::thread(&cPluginHbbtv::WriteUrlsToFile, this);
    urlwriter_thread.detach();

    return true;
}

void cPluginHbbtv::Stop(void) {
    HBBTV_DBG("[hbbtv] Stop Plugin\n");

    // Stop any background activities the plugin is performing.
    urlwriter_running = false;

    if (urlwriter_thread.joinable()) {
        urlwriter_thread.join();
    }

    if (HbbtvDeviceStatus) DELETENULL(HbbtvDeviceStatus);
    if (browserComm) DELETENULL(browserComm);

    stopVdrOsrBrowser();

    lastDisplayWidth = 0;
    lastDisplayHeight = 0;

    DELETENULL(pluginConfigDirectory);
}

cOsdObject *cPluginHbbtv::MainMenuAction(void) {
    // Perform the action when selected from the main VDR menu.
    return osdDispatcher->get(MAINMENUENTRY);
}

void cPluginHbbtv::MainThreadHook(void) {
    HBBTV_DBG("[hbbtv] In MainThreadHook\n");

    if (showPlayer) {
        HBBTV_DBG("[hbbtv] In MainThreadHook, show player\n");

        showPlayer = false;

        if (!isHbbtvPlayerActivated) {
            HBBTV_DBG("[hbbtv] In MainThreadHook, create Video Player\n");
            auto player = new HbbtvVideoPlayer(OsrBrowserVideoProto);
            SetVideoSize();

            HBBTV_DBG("[hbbtv] In MainThreadHook, create Video Control\n");
            auto video = new HbbtvVideoControl(player);
            cControl::Launch(video);
            video->Attach();
        }
    }

    if (hbbtvPage != nullptr) {
        if (OsrBrowserStart && OsrBrowserPid == 0) {
            return;
        }

        static int osdState = 0;
        if (cOsdProvider::OsdSizeChanged(osdState)) {
            int newWidth;
            int newHeight;
            double ph;

            cDevice::PrimaryDevice()->GetOsdSize(newWidth, newHeight, ph);

            if (newWidth != lastDisplayWidth || newHeight != lastDisplayHeight) {
                dsyslog("[hbbtv] Old Size: %dx%d, New Size. %dx%d", lastDisplayWidth, lastDisplayHeight, newWidth, newHeight);
                lastDisplayWidth = newWidth;
                lastDisplayHeight = newHeight;

                hbbtvPage->Display();
            }
        }
    }
}

bool cPluginHbbtv::Service(const char *Id, void *Data) {
    if (strcmp(Id, "BrowserStatus-1.0") == 0) {
        if (Data) {
            BrowserStatus_v1_0 *status = (BrowserStatus_v1_0 *) Data;

            dsyslog("[hbbtv] Received Status: %s", *status->message);

            if (strncmp(status->message, "PLAY_VIDEO:", 11) == 0) {
                ShowPlayer();
            } else if (strncmp(status->message, "STOP_VIDEO", 10) == 0) {
                HidePlayer();
            } else if (strncmp(status->message, "START_BROWSER", 13) == 0) {
                if (OsrBrowserPid > 0) {
                    // try to stop if running
                    stopVdrOsrBrowser();
                }

                startVdrOsrBrowser();
            } else if (strncmp(status->message, "STOP_BROWSER", 12) == 0) {
                stopVdrOsrBrowser();
            } else if (strncmp(status->message, "RESTART_BROWSER", 15) == 0) {
                stopVdrOsrBrowser();
                startVdrOsrBrowser();
            } else if (strncmp(status->message, "GETURL: ", 8) == 0) {
                currentUrlChannel = cString(status->message + 8);
            } else if (strncmp(status->message, "VIDEO_FAILED", 12) == 0) {
                HidePlayer();
                Skins.Message(mtError, tr("Video cannot be played!"));
            }
        }
        return true;
    }
    return false;
}

void cPluginHbbtv::ShowPlayer() {
    HBBTV_DBG("[hbbtv] Show Player\n");

    // show video player
    showPlayer = true;
}

void cPluginHbbtv::HidePlayer() {
    HBBTV_DBG("[hbbtv] Hide Player\n");

    cMutexLock MutexLock(&controlMutex);

    showPlayer = false;

    // stop video player and show TV again
    cControl * current = cControl::Control(MutexLock);

    if (dynamic_cast<HbbtvVideoControl * >(current)) {
        cControl::Shutdown();
        cControl::Attach();
    }
}

bool cPluginHbbtv::startVdrOsrBrowser() {
    HBBTV_DBG("[hbbtv] Start Browser\n");

    const std::lock_guard<std::mutex> lock(browser_start_mtx);

    if (!OsrBrowserStart) {
        HBBTV_DBG("[hbbtv] Browser start is not configured\n");

        return true;
    }

    if (OsrBrowserPid > 0) {
        HBBTV_DBG("[hbbtv] Browser already running with pid %d\n", OsrBrowserPid);

        // already running
        return true;
    }

    // fork and start vdrosrbrowser
    isyslog("[hbbtv] fork browser\n");

    pid_t pid = fork();
    if (pid == -1) {
        esyslog("[hbbtv] browser fork failed. Aborting...\n");
        return false;
    } else if (pid == 0) {
        HBBTV_DBG("[hbbtv] Browser fork successful");

        // create the final commandline parameter for execv
        std::vector <char*> cmd_params;
        std::stringstream cmd(OsrBrowserCmdLine);
        std::string inter;

        cmd_params.push_back(strdup(OsrBrowserPath.c_str()));

        while(getline(cmd, inter, ' ')) {
            cmd_params.push_back(strdup(inter.c_str()));
        }

        std::string logcmd = "--logfile=" + OsrBrowserLogFile;
        cmd_params.push_back(strdup(logcmd.c_str()));

        std::string videocmd = "--video=" + OsrBrowserVideoProto;
        cmd_params.push_back(strdup(videocmd.c_str()));

        cmd_params.push_back((char*)NULL);

        // start the browser
        std::string wd = OsrBrowserPath.substr(0, OsrBrowserPath.find_last_of('/'));
        isyslog("[hbbtv] Start vdrosrbrowser, change directory to %s", wd.c_str());
        chdir(wd.c_str());

        char **command = cmd_params.data();

        HBBTV_DBG("[hbbtv] Forked, execute execv");

        if (OsrBrowserDisplay.empty()) {
            execv(command[0], &command[0]);
        } else {
            char *display;
            asprintf(&display, "DISPLAY=%s", OsrBrowserDisplay.c_str());

            char *dbus;
            asprintf(&dbus, "DBUS_SESSION_BUS_ADDRESS=autolaunch:");

            char *envp[] = {display, dbus, NULL};

            execvpe(command[0], &command[0], envp);
        }
        exit(0);
    }

    HBBTV_DBG("[hbbtv] Add signal handler\n");

    signal(SIGCHLD, browser_signal_handler);

    HBBTV_DBG("[hbbtv] BrowserPid %d\n", pid);

    OsrBrowserPid = pid;

    return true;
}

void cPluginHbbtv::stopVdrOsrBrowser() {
    HBBTV_DBG("[hbbtv] Stop Browser, pid %d\n", OsrBrowserPid);

    if (OsrBrowserPid == 0) {
        // already stopped
        HBBTV_DBG("[hbbtv] Browser already stopped");
        return;
    }

    if (OsrBrowserPid == -1) {
        // browser externally started
        HBBTV_DBG("[hbbtv] Browser externally started.");
        return;
    }

    if (OsrBrowserPid == -2) {
        // browser already manually stopped
        HBBTV_DBG("[hbbtv] Browser manually stopped.");
        return;
    }

    HBBTV_DBG("[hbbtv] Send kill to pid %d", OsrBrowserPid);
    kill(OsrBrowserPid, SIGTERM);

    HBBTV_DBG("[hbbtv] Wait for Browser stop");
    int status;
    waitpid(OsrBrowserPid, &status, 0);

    HBBTV_DBG("[hbbtv] Browser stopped, reset Pid");
    OsrBrowserPid = -2;
}

const char *cPluginHbbtv::CommandLineHelp(void)
{
    // Return a string that describes all known command line options.
    return "  -s,               --start-browser              starts and stops the vdrosrbrowser if necessary. If not set the browser has to be started manually.\n"
           "  -p <path>,        --path=<path>                the full path to vdrosrbrowser\n"
           "  -c <cmdline>,     --commandline=<commandline>  the full command line used for vdrosrbrowser\n"
           "  -l <logfile>,     --logfile=<logfile>          log file for vdrosrbrowser\n"
           "  -d <display>,     --display=<display>          the X display to use\n"
           "  -v [UDP,TCP,UNIX] --video=[UDP,TCP,UNIX]       Sets the transport protocol to use for incoming video data: UDP, TCP or Unix domain socket\n";
}

bool cPluginHbbtv::ProcessArgs(int argc, char *argv[])
{
    // Implement command line argument processing here if applicable.
    static const struct option long_options[] = {
            { "start-browser",  no_argument,       NULL, 's' },
            { "path",           required_argument, NULL, 'p' },
            { "commandline",    required_argument, NULL, 'c' },
            { "logfile",        required_argument, NULL, 'l' },
            { "display",        required_argument, NULL, 'd' },
            { "video",          required_argument, NULL, 'v' },
            { NULL,             no_argument,       NULL,  0  }
    };

    // set default values
    OsrBrowserVideoProto = std::string("UDP");

    int c;
    while ((c = getopt_long(argc, argv, "p:c:l:d:v:s", long_options, NULL)) != -1) {
        switch (c) {
            case 's':
                OsrBrowserStart = true;
                OsrBrowserPid = 0;
                break;
            case 'c':
                OsrBrowserCmdLine = std::string(optarg);
                break;
            case 'p':
                OsrBrowserPath = std::string(optarg);
                break;
            case 'l':
                OsrBrowserLogFile = std::string(optarg);
                break;
            case 'd':
                OsrBrowserDisplay = std::string(optarg);
                break;
            case 'v':
                OsrBrowserVideoProto = std::string(optarg);

                if (OsrBrowserVideoProto != "TCP" && OsrBrowserVideoProto != "UDP" && OsrBrowserVideoProto != "UNIX") {
                    esyslog("[hbbtv] Error: Video protocol '%s' is not valid", optarg);
                    return false;
                }
                break;
            default:
                esyslog("[hbbtv] Error: Unknown command line parameter '%c'", c);
                return false;
        }
    }

    if (OsrBrowserStart) {
        if (OsrBrowserPath.empty()) {
            esyslog("[hbbtv] Error: StartBrowser set but browser path is empty.");
            return false;
        }

        if (OsrBrowserLogFile.empty()) {
            esyslog("[hbbtv] Error: StartBrowser set but browser log file is empty.");
            return false;
        }
    } else {
        // we have no control over browser start. Assume, it's already started.
        browserStarted = true;
    }

    return true;
}

const char **cPluginHbbtv::SVDRPHelpPages(void)
{
    static const char *HelpPages[] = {
            "PING\n"
            "    Test if vdrosrbrowser (internal or external) is running and available.",
            "STOP\n"
            "    Stop the internal vdrosrbrowser.",
            "START\n"
            "    Start the internal vdrosrbrowser.",
            "RESTART\n"
            "    Restart the internal vdrosrbrowser.",
            "URL <url>\n"
            "    Load the URL in vdrosrbrowser.",
            "JS <command>\n"
            "    Execute the javascript command in vdrosrbrowser.",
            "KEY <key>\n"
            "    Send the key event to the browser.",
            "ATTACH\n"
            "    Attach the player.",
            "DETACH\n"
            "    Deattach the player.",
            "GETURL\n"
            "    Returns the current channel information and URL of the browser.",
            "STATUS\n"
            "    Returns if Player or OSD is open.",
            "DCHANNEL\n"
            "    Debug: Sends Channel information to the browser (Parameter is a file containing the channel)",
            "DAPPURL\n"
            "    Debug: Sends an APPURL to the browser",
            NULL
    };

    return HelpPages;
}

cString cPluginHbbtv::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
    if (strcasecmp(Command, "PING") == 0) {
        int isalive = isBrowserAlive();

        if (isalive > 0) {
            return "Browser is alive";
        } else if (isalive == 0) {
            ReplyCode = 901;
            return "Browser has been stopped/killed.";
        } else {
            if (!browserComm->Heartbeat()) {
                ReplyCode = 901;
                return "Browser does not answer. Possibly stopped.";
            } else {
                return "Browser is alive";
            }
        }
    } else if (strcasecmp(Command, "STOP") == 0) {
        stopVdrOsrBrowser();
        return cString::sprintf("Browser stopped.");
    } else if (strcasecmp(Command, "START") == 0) {
        startVdrOsrBrowser();
        return cString::sprintf("Browser started.");
    } else if (strcasecmp(Command, "RESTART") == 0) {
        stopVdrOsrBrowser();
        startVdrOsrBrowser();
        return cString::sprintf("Browser restarted.");
    } else if (strcasecmp(Command, "URL") == 0) {
        if (!*Option) {
            ReplyCode = 902;
            return "Missing parameter for command URL.";
        }

        char *cmd;
        asprintf(&cmd, "URL %s", Option);
        browserComm->SendToBrowser(cmd);
        free(cmd);

        return cString::sprintf("URL %s loaded.", Option);
    } else if (strcasecmp(Command, "JS") == 0) {
        if (!*Option) {
            ReplyCode = 902;
            return "Missing parameter for command JS.";
        }

        char *cmd;
        asprintf(&cmd, "JS %s", Option);
        browserComm->SendToBrowser(cmd);
        free(cmd);

        return cString::sprintf("JS %s send.", Option);
    } else if (strcasecmp(Command, "KEY") == 0) {
        if (!*Option) {
            ReplyCode = 902;
            return "Missing parameter for command KEY.";
        }

        char *cmd;
        asprintf(&cmd, "KEY %s", Option);
        browserComm->SendToBrowser(cmd);
        free(cmd);

        return cString::sprintf("KEY %s send.", Option);
    } else if (strcasecmp(Command, "ATTACH") == 0) {
        ShowPlayer();
        return "Player attached";
    } else if (strcasecmp(Command, "DETACH") == 0) {
        HidePlayer();
        return "Player detached";
    } else if (strcasecmp(Command, "GETURL") == 0) {
        if (isempty(*currentUrlChannel)) {
            ReplyCode = 503;
            return ("URL and channel information are not available");
        }
        return currentUrlChannel;
    } else if (strcasecmp(Command, "STATUS") == 0) {
        char *buffer;

        asprintf(&buffer, "OSD: %s\nPlayer: %s", hbbtvPage != nullptr ? "open" : "closed", hbbtvVideoPlayer != nullptr ? "open" : "closed");
        cString result(buffer, true);

        return result;
    } else if (strcasecmp(Command, "DCHANNEL") == 0) {
        if (!*Option) {
            ReplyCode = 902;
            return "Missing parameter for command DCHANNEL.";
        }

        std::ifstream infile(Option);

        if (infile.good()) {
            std::string sLine;
            std::getline(infile, sLine);

            char *cmd;
            asprintf(&cmd, "CHANNEL %s", sLine.c_str());
            browserComm->SendToBrowser(cmd);
            free(cmd);

            return cString::sprintf("CHANNEL sent.");
        } else {
            ReplyCode = 903;
            return cString::sprintf("Unable to open file %s", Option);
        }
    } else if (strcasecmp(Command, "DAPPURL") == 0) {
        if (!*Option) {
            ReplyCode = 902;
            return "Missing parameter for command DAPPURL.";
        }

        char *cmd;
        asprintf(&cmd, "APPURL %s", Option);
        browserComm->SendToBrowser(cmd);
        free(cmd);

        return cString::sprintf("APPURL sent.");
    }

    ReplyCode = 502;
    return NULL;
}


VDRPLUGINCREATOR(cPluginHbbtv);  // Don't touch this!
