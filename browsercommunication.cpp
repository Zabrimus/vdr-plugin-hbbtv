#include <unistd.h>
#include <cstdio>
#include <vdr/tools.h>
#include <vdr/plugin.h>
#include <vdr/remote.h>
#include "browsercommunication.h"
#include "hbbtvservice.h"
#include "hbbtvvideocontrol.h"
#include "cefhbbtvpage.h"
#include "globals.h"
#include "hbbtvurl.h"
#include "sharedmemory.h"

// set to true, if you want to see commands sent to the browser
const bool DEBUG_SEND_COMMAND = false;

BrowserCommunication *browserComm;

BrowserCommunication::BrowserCommunication(const char* name) : cThread("BrowserInThread") {
    HBBTV_DBG("[hbbtv] Start BrowserCommunication");

    initKeyMapping();
    pluginName = name;
    lastHeartbeat = time(NULL) - 31;
}

BrowserCommunication::~BrowserCommunication() {
    HBBTV_DBG("[hbbtv] close sockets");
}

void BrowserCommunication::Action(void) {
    uint8_t *buf;
    int bytes;
    BrowserStatus_v1_0 status;

    // start the thread
    while (Running()) {
        if (sharedMemory.waitForRead(BrowserCommand) == -1) {
            // got currently no new command
            cCondWait::SleepMs(10);
            continue;
        }

        sharedMemory.read(&buf, &bytes, BrowserCommand);

        uint8_t type = buf[0];

        HBBTV_DBG("[hbbtv] Received Action %d, %s", type, buf+1);

        switch (type) {
            case CMD_STATUS:
                // Status message from vdrosrbrowser
                if (strncmp((char *) buf + 1, "VIDEO_SIZE: ", 12) == 0) {
                    // Video resize requested
                    int x, y, w, h;
                    int ret = std::sscanf((const char *) buf + 1 + 12, "%d,%d,%d,%d", &w, &h, &x, &y);

                    HBBTV_DBG("[hbbtv] Received action VIDEO_SIZE %d,%d,%d,%d", w, h, x, y);

                    if (ret == 4) {
                        video_x = x;
                        video_y = y;
                        video_width = w;
                        video_height = h;

                        SetVideoSize();
                    }
                } else if (strncmp((char *) buf + 1, "SEND_INIT", 9) == 0) {
                    HBBTV_DBG("[hbbtv] Received action SEND_INIT");

                    // send current channel to browser
                    sendChannelToBrowser(cDevice::CurrentChannel());

                    // send appurl to the browser
                    cHbbtvURLs *hbbtvURLs = (cHbbtvURLs *)cHbbtvURLs::HbbtvURLs();
                    for (cHbbtvURL *url = hbbtvURLs->First(); url; url = hbbtvURLs->Next(url)) {
                        char *cmd;
                        char *appurl = url->ToAppUrlString();
                        asprintf(&cmd, "APPURL %s", appurl);
                        SendToBrowser(cmd);
                        free(cmd);
                        free(appurl);
                    }
                } else {
                    status.message = cString((char *) buf + 1);
                    cPluginManager::CallAllServices("BrowserStatus-1.0", &status);
                }
                break;

            case CMD_OSD:
                HBBTV_DBG("[hbbtv] Received OSD Update, Page %s, Player %s", (hbbtvPage ? "exists" : "does not exists"), (hbbtvVideoPlayer ? "exists" : "does not exists"));

                // OSD update from vdrosrbrowser
                if (hbbtvPage && hbbtvVideoPlayer == nullptr) {
                    OsdStruct* osdUpdate = (OsdStruct*)(buf + 1);
                    hbbtvPage->readOsdUpdate(osdUpdate);
                }
                break;

            /*
            case CMD_VIDEO:
                if (hbbtvVideoPlayer) {
                    hbbtvVideoPlayer->newPacketReceived();
                }
                break;
            */

            case CMD_PING:
                HBBTV_DBG("[hbbtv] Received action Ping from browser");

                // Ping from browser
                lastHeartbeat = time(NULL);
                browserStarted = true;
                break;

            default:
                // something went wrong
                break;
        }

        sharedMemory.finishedReading(BrowserCommand);
    }
};

bool BrowserCommunication::Heartbeat() {
    time_t current = time(NULL);

    HBBTV_DBG("[hbbtv] Heartbeat. Result %s", (current - lastHeartbeat >= 30) ? "false" : "true");

    if (current - lastHeartbeat >= 30) {
        return false;
    }

    return true;
}

bool BrowserCommunication::SendToBrowser(const char* command) {
    bool result;
    static int countlog = 0;

    if (OsrBrowserPid == -2) {
        // browser manually stopped.
        return false;
    }

    HBBTV_DBG("[hbbtv] SendToBrowser %s", command);

    // try to wait for the browser
    int i = 0;
    while (i < 10 && !browserStarted) {
        std::this_thread::sleep_for (std::chrono::milliseconds(100));
        ++i;
    }

    HBBTV_DBG("[hbbtv] SendToBrowser, waited %d ms, result %s", (i * 100), (browserStarted ? "true" : "false"));

    if (OsrBrowserPid == -1 && !Heartbeat()) {
        if (++countlog < 4) {
            esyslog("[hbbtv] external browser is not running, command '%s' will be ignored", command);
        }
        return false;
    }

    if (!isBrowserAlive()) {
        esyslog("[hbbtv] browser is not running, command '%s' will be ignored, try to restart the browser", command);

        OsdDispatcher::osdType = OSDType::CLOSE;
        cRemote::CallPlugin(pluginName);

        // try to restart the browser
        BrowserStatus_v1_0 status;
        status.message = cString("START_BROWSER");
        cPluginManager::CallAllServices("BrowserStatus-1.0", &status);

        return false;
    }

    HBBTV_DBG("[hbbtv] Send command '%s'", command);

    result = true;

    if (sharedMemory.waitForWrite(VdrCommand) != -1) {
        sharedMemory.write(const_cast<char *>(command), VdrCommand);
        HBBTV_DBG("[hbbtv] Command send");
    } else {
        if (++countlog < 8) {
            esyslog("[hbbtv] Unable to send command: Timeout");
        }
        result = false;
    }

    if (result) {
        countlog = 0;
    }

    return result;
}

bool BrowserCommunication::SendKey(cString key) {
    char *cmd;

    asprintf(&cmd, "KEY %s", *key);

    HBBTV_DBG("[hbbtv] Send Key Command '%s' to browser", cmd);

    auto result = SendToBrowser(cmd);
    free(cmd);

    return result;
}

bool BrowserCommunication::SendKey(eKeys Key) {
    std::map<eKeys, std::string>::iterator it;

    it = keyMapping.find(Key);
    if (it != keyMapping.end()) {
        return SendKey(it->second);
    }

    return false;
}

bool BrowserCommunication::SendKey(std::string key) {
    return SendKey(cString(key.c_str()));
}

void BrowserCommunication::initKeyMapping() {
    keyMapping.insert(std::pair<eKeys, std::string>(kUp,      "VK_UP"));
    keyMapping.insert(std::pair<eKeys, std::string>(kDown,    "VK_DOWN"));
    keyMapping.insert(std::pair<eKeys, std::string>(kLeft,    "VK_LEFT"));
    keyMapping.insert(std::pair<eKeys, std::string>(kRight,   "VK_RIGHT"));
    // keyMapping.insert(std::pair<eKeys, std::string>(kBack,    "VK_BACK"));

    // keyMapping.insert(std::pair<eKeys, std::string>(???,      "VK_PAGE_UP"));
    // keyMapping.insert(std::pair<eKeys, std::string>(???,      "VK_PAGE_DOWN"));

    keyMapping.insert(std::pair<eKeys, std::string>(kOk,      "VK_ENTER"));

    keyMapping.insert(std::pair<eKeys, std::string>(kRed,     "VK_RED"));
    keyMapping.insert(std::pair<eKeys, std::string>(kGreen,   "VK_GREEN"));
    keyMapping.insert(std::pair<eKeys, std::string>(kYellow,  "VK_YELLOW"));
    keyMapping.insert(std::pair<eKeys, std::string>(kBlue,    "VK_BLUE"));

    keyMapping.insert(std::pair<eKeys, std::string>(kPlay,    "VK_PLAY"));
    keyMapping.insert(std::pair<eKeys, std::string>(kPause,   "VK_PAUSE"));
    keyMapping.insert(std::pair<eKeys, std::string>(kStop,    "VK_STOP"));
    keyMapping.insert(std::pair<eKeys, std::string>(kFastFwd, "VK_FAST_FWD"));
    keyMapping.insert(std::pair<eKeys, std::string>(kFastRew, "VK_REWIND"));

    keyMapping.insert(std::pair<eKeys, std::string>(k0,       "VK_0"));
    keyMapping.insert(std::pair<eKeys, std::string>(k1,       "VK_1"));
    keyMapping.insert(std::pair<eKeys, std::string>(k2,       "VK_2"));
    keyMapping.insert(std::pair<eKeys, std::string>(k3,       "VK_3"));
    keyMapping.insert(std::pair<eKeys, std::string>(k4,       "VK_4"));
    keyMapping.insert(std::pair<eKeys, std::string>(k5,       "VK_5"));
    keyMapping.insert(std::pair<eKeys, std::string>(k6,       "VK_6"));
    keyMapping.insert(std::pair<eKeys, std::string>(k7,       "VK_7"));
    keyMapping.insert(std::pair<eKeys, std::string>(k8,       "VK_8"));
    keyMapping.insert(std::pair<eKeys, std::string>(k9,       "VK_9"));
}
