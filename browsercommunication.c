#include <unistd.h>
#include <vdr/tools.h>
#include <vdr/plugin.h>
#include <vdr/remote.h>
#include "browsercommunication.h"
#include "hbbtvservice.h"
#include "hbbtvvideocontrol.h"
#include "cefhbbtvpage.h"

BrowserCommunication *browserComm;

BrowserCommunication::BrowserCommunication(const char* name) : cThread("BrowserInThread") {
    // open the input socket
    cString toVdrUrl = cString::sprintf("ipc://%s", *ipcToVdrFile);

    if ((inSocketId = nn_socket(AF_SP, NN_PULL)) < 0) {
        esyslog("[hbbtv] Unable to create socket");
    }

    if ((inEndpointId = nn_connect(inSocketId, toVdrUrl)) < 0) {
        esyslog("[hbbtv] unable to connect nanomsg socket to %s\n", *toVdrUrl);
    }

    // set timeout in ms
    int to = 50;
    nn_setsockopt (inSocketId, NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof (to));

    // open the input/output socket
    cString toBrowserUrl = cString::sprintf("ipc://%s", *ipcToBrowserFile);

    if ((outSocketId = nn_socket(AF_SP, NN_REQ)) < 0) {
        esyslog("[hbbtv] Unable to create socket");
    }

    if ((outEndpointId = nn_connect(outSocketId, toBrowserUrl)) < 0) {
        esyslog("[hbbtv] unable to connect nanomsg socket to %s\n", *toBrowserUrl);
    }

    // set timeout in ms
    int tout = 2000;
    nn_setsockopt (outSocketId, NN_SOL_SOCKET, NN_RCVTIMEO, &tout, sizeof (tout));
    nn_setsockopt (outSocketId, NN_SOL_SOCKET, NN_SNDTIMEO, &tout, sizeof (tout));

    initKeyMapping();

    pluginName = name;
}

BrowserCommunication::~BrowserCommunication() {
    nn_close(inSocketId);
    nn_close(outSocketId);
}

void BrowserCommunication::Action(void) {
    uint8_t buf[32712 + 1];
    int bytes;
    BrowserStatus_v1_0 status;

    // start the thread
    while (Running()) {
        if ((bytes = nn_recv(inSocketId, &buf, 32712 + 1, NN_DONTWAIT)) < 0) {
            if ((nn_errno() != ETIMEDOUT) && (nn_errno() != EAGAIN)) {
                esyslog("[hbbtv] Error reading command byte. Error %s\n", nn_strerror(nn_errno()));

                // FIXME: Something useful shall happen here
            }

            // got currently no new command
            cCondWait::SleepMs(10);
            continue;
        }

        uint8_t type = buf[0];

        switch (type) {
            case 1:
                // Status message from vdrosrbrowser
                if (strncmp((char*)buf+1, "SEEK", 4) == 0) {
                    hbbtvVideoPlayer->Detach();
                    cDevice::PrimaryDevice()->AttachPlayer(hbbtvVideoPlayer);
                } else {
                    status.message = cString((char *) buf + 1);
                    cPluginManager::CallAllServices("BrowserStatus-1.0", &status);
                }
                break;

            case 2:
                // OSD update from vdrosrbrowser
                if (hbbtvPage) {
                    OsdStruct* osdUpdate = (OsdStruct*)(buf + 1);
                    hbbtvPage->readOsdUpdate(osdUpdate);
                }
                break;

            case 3:
                // video update from vdrosrbrowser
                if (hbbtvVideoPlayer) {
                    hbbtvVideoPlayer->readTsFrame(&buf[1], bytes - 1);
                }
                break;

            default:
                // something went wrong
                break;
        }
    }
};

bool BrowserCommunication::Heartbeat() {
    int bytes;

    if ((bytes = nn_send(outSocketId, "PING", 4 + 1, 0)) < 0) {
        return false;
    }

    char *buf = nullptr;
    if ((bytes = nn_recv(outSocketId, &buf, NN_MSG, 0)) < 0) {
        if (buf != nullptr) {
            nn_freemsg(buf);
        }

        return false;
    }

    if (strncmp(buf+1, "ok", 2) != 0) {
        nn_freemsg(buf);
        return false;
    }

    nn_freemsg(buf);

    return true;
}

bool BrowserCommunication::SendToBrowser(const char* command) {
    bool result;
    int bytes;

    if (!Heartbeat()) {
        esyslog("[hbbtv] browser is not running, command '%s' will be ignored", command);
        Skins.Message(mtError, tr("Browser is not running!"));

        OsdDispatcher::osdType = OSDType::CLOSE;
        cRemote::CallPlugin(pluginName);

        // try to restart the browser
        BrowserStatus_v1_0 status;
        status.message = cString("START_BROWSER");
        cPluginManager::CallAllServices("BrowserStatus-1.0", &status);

        return false;
    }

    bool returnValue;

    if (strncmp("PING", command, 4) != 0) {
        dsyslog("[hbbtv] Send command '%s'", command);
    }

    result = true;

    if ((bytes = nn_send(outSocketId, command, strlen(command) + 1, 0)) < 0) {
        esyslog("[hbbtv] Unable to send command...");
        result = false;
    }

    return result;
}

cString BrowserCommunication::ReadResponse() {
    int bytes;
    char *buf = nullptr;
    if ((bytes = nn_recv(outSocketId, &buf, NN_MSG, 0)) < 0) {
        if (buf != nullptr) {
            nn_freemsg(buf);
        }

        return nullptr;
    }

    cString result(buf);
    nn_freemsg(buf);

    return result;
}

bool BrowserCommunication::SendKey(cString key) {
    char *cmd;

    asprintf(&cmd, "KEY %s", *key);

    dsyslog("[hbbtv] Send Key Command '%s' to browser", cmd);

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
