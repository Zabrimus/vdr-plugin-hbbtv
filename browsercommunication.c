#include <unistd.h>
#include <vdr/tools.h>
#include <vdr/plugin.h>
#include "browsercommunication.h"
#include "hbbtvservice.h"
#include "browser.h"
#include "hbbtvvideocontrol.h"

BrowserCommunication *browserComm;

BrowserCommunication::BrowserCommunication() : cThread("BrowserInThread") {
    // open the input socket
    cString toVdrUrl = cString::sprintf("ipc://%s", *ipcToVdrFile);

    if ((inSocketId = nn_socket(AF_SP, NN_PULL)) < 0) {
        esyslog("Unable to create socket");
    }

    if ((inEndpointId = nn_connect(inSocketId, toVdrUrl)) < 0) {
        esyslog("unable to connect nanomsg socket to %s\n", *toVdrUrl);
    }

    // set timeout in ms
    int to = 50;
    nn_setsockopt (inSocketId, NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof (to));

    // open the input/output socket
    cString toBrowserUrl = cString::sprintf("ipc://%s", *ipcToBrowserFile);

    if ((outSocketId = nn_socket(AF_SP, NN_REQ)) < 0) {
        esyslog("Unable to create socket");
    }

    if ((outEndpointId = nn_connect(outSocketId, toBrowserUrl)) < 0) {
        esyslog("unable to connect nanomsg socket to %s\n", *toBrowserUrl);
    }

    // set timeout in ms
    int tout = 2000;
    nn_setsockopt (outSocketId, NN_SOL_SOCKET, NN_RCVTIMEO, &tout, sizeof (tout));
    nn_setsockopt (outSocketId, NN_SOL_SOCKET, NN_SNDTIMEO, &tout, sizeof (tout));

    initKeyMapping();
}

BrowserCommunication::~BrowserCommunication() {
    nn_close(inSocketId);
    unlink(*ipcToVdrFile);

    nn_close(outSocketId);
    unlink(*ipcToBrowserFile);
}

void BrowserCommunication::Action(void) {
    char *buf = nullptr;

    // start the thread
    while (Running()) {
        int bytes;
        uint8_t type = 0;
        if ((bytes = nn_recv(inSocketId, &type, 1, NN_DONTWAIT)) < 0) {
            if ((nn_errno() != ETIMEDOUT) && (nn_errno() != EAGAIN)) {
                esyslog("Error reading command byte. Error %s\n", nn_strerror(nn_errno()));

                // FIXME: Something useful shall happen here
            }

            // got currently no new command
            cCondWait::SleepMs(50);
            continue;
        }

        if (bytes == 1) {
            switch (type) {
                case 1:
                    // Status message from vdrosrbrowser
                    if (buf != nullptr) {
                        free(buf);
                        buf = nullptr;
                    }

                    if ((bytes = nn_recv(inSocketId, &buf, NN_MSG, 0)) > 0) {
                        BrowserStatus_v1_0 status;
                        status.message = cString(buf);

                        cPluginManager::CallAllServices("BrowserStatus-1.0", &status);

                        nn_freemsg(buf);
                        buf = nullptr;
                    }
                    break;

                case 2:
                    /// OSD update from vdrosrbrowser
                    if (browser) {
                        browser->readOsdUpdate(inSocketId);
                    } else {
                        esyslog("Internal error. Got OSD message, but browser does not exists.");
                    }
                    break;

                case 3:
                    // video update from vdrosrbrowser
                    if (player) {
                        player->readTsFrame(inSocketId);
                    } else {
                        esyslog("Internal error. Got Video message, but player does not exists.");
                    }
                    break;

                default:
                    // something went wrong
                    break;
            }
        } else {
            cCondWait::SleepMs(10);
        }
    }
};

bool BrowserCommunication::SendToBrowser(const char* command) {
    bool returnValue;

    dbgbrowser("Send command '%s'\n", command);

    char *response = nullptr;
    int bytes;

    if ((bytes = nn_send(outSocketId, command, strlen(command) + 1, 0)) < 0) {
        esyslog("Unable to send command...");
        return false;
    }

    if (bytes > 0 && (bytes = nn_recv(outSocketId, &response, NN_MSG, 0)) < 0) {
        esyslog("Unable to read response...");
        returnValue = false;
    } else {
        dbgbrowser("Response received: '%s', %d\n", response, bytes);

        returnValue = strcasecmp(response, "ok") == 0;
    }

    if (response) {
        nn_freemsg(response);
    }

    return returnValue;
}

bool BrowserCommunication::SendKey(cString key) {
    char *cmd;

    asprintf(&cmd, "KEY %s", *key);
    auto result = SendToBrowser(cmd);
    free(cmd);

    return result;
}

bool BrowserCommunication::SendKey(eKeys Key) {
    std::map<eKeys, std::string>::iterator it;

    it = keyMapping.find(Key);
    if (it != keyMapping.end()) {
        SendKey(it->second);
        return true;
    }

    return false;
}

void BrowserCommunication::SendKey(std::string key) {
    SendKey(cString(key.c_str()));
}

void BrowserCommunication::initKeyMapping() {
    keyMapping.insert(std::pair<eKeys, std::string>(kUp,      "VK_UP"));
    keyMapping.insert(std::pair<eKeys, std::string>(kDown,    "VK_DOWN"));
    keyMapping.insert(std::pair<eKeys, std::string>(kLeft,    "VK_LEFT"));
    keyMapping.insert(std::pair<eKeys, std::string>(kRight,   "VK_RIGHT"));

    // keyMapping.insert(std::pair<eKeys, std::string>(???,   "VK_PAGE_UP"));
    // keyMapping.insert(std::pair<eKeys, std::string>(???,   "VK_PAGE_DOWN"));

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
