#include <nanomsg/nn.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include "hbbtvvideocontrol.h"
#include "globals.h"

// 348 * 188
#define BUFFER_SIZE 65424

HbbtvVideoPlayer *hbbtvVideoPlayer;
bool isHbbtvPlayerActivated;

HbbtvVideoPlayer::HbbtvVideoPlayer(std::string vproto) {
    dsyslog("[hbbtv] Create Player...");
    hbbtvVideoPlayer = this;
    videosocket = -1;
    filled = 0;
    this->vproto = vproto;
}

HbbtvVideoPlayer::~HbbtvVideoPlayer() {
    dsyslog("[hbbtv] Delete Player...");

    setVideoDefaultSize();

    Detach();

    closeSocket();

    if (Running()) {
        Cancel(-1);
    }
    hbbtvVideoPlayer = nullptr;

    if (!browserComm->SendToBrowser("PLAYER_DETACHED")) {
        // ???
    }
}

void HbbtvVideoPlayer::Activate(bool On) {
    dsyslog("[hbbtv] Activate video player: %s", On ? " Ja" : "Nein");

    if (On) {
        isHbbtvPlayerActivated = true;
        Start();
    } else {
        isHbbtvPlayerActivated = false;
        setVideoDefaultSize();

        cRect r = {0,0,0,0};
        cDevice::PrimaryDevice()->ScaleVideo(r);
    }
}

void HbbtvVideoPlayer::Action(void) {
    connectRequested = true;

    if (vproto == "UDP") {
        startUdpVideoReader();
    } else if (vproto == "TCP") {
        startTcpVideoReader();
    } else if (vproto == "UNIX") {
        startUnixVideoReader();
    }
}

void HbbtvVideoPlayer::closeSocket() {
    if (videosocket != -1) {
        int cl = close(videosocket);
        if (cl < 0) {
            esyslog("[hbtv] Unable to close UDP socket: %s", strerror(errno));
        }

        videosocket = -1;
    }
}

bool HbbtvVideoPlayer::connectUdp() {
    dsyslog("[hbbtv] Connect via UDP");

    struct sockaddr_in servAddr;
    int rc;
    const int y = 1;

    connectRequested = false;

    // create UDP socket
    videosocket = socket (AF_INET, SOCK_DGRAM, 0);
    if (videosocket < 0) {
        esyslog("[hbtv] Unable to open UDP socket: %d -> %s", nn_errno(), nn_strerror(nn_errno()));
        return false;
    }

    // bind socket
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

    servAddr.sin_port = htons (VIDEO_UDP_PORT);
    setsockopt(videosocket, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));

    rc = bind(videosocket, (struct sockaddr *) &servAddr, sizeof (servAddr));
    if (rc < 0) {
        esyslog("[hbbtv] Unable to bind UDP socket on port %d: %d -> %s", VIDEO_UDP_PORT, nn_errno(), nn_strerror(nn_errno()));
        return false;
    }

    return true;
}

bool HbbtvVideoPlayer::connectTcp() {
    dsyslog("[hbbtv] Connect via TCP");

    struct sockaddr_in servAddr;
    int rc;

    connectRequested = false;

    // create TCP socket
    videosocket = socket (AF_INET, SOCK_STREAM, 0);
    if (videosocket < 0) {
        esyslog("[hbtv] Unable to open TCP socket: %d -> %s", nn_errno(), nn_strerror(nn_errno()));
        return false;
    }

    // bind socket
    servAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(VIDEO_TCP_PORT);

    // wait some time and try to connect multiple times
    int i = 0;
    const int sleepinms = 20;
    while (i < 500) {
        rc = connect(videosocket , (struct sockaddr *)&servAddr , sizeof(servAddr));
        if (rc < 0) {
            // wait some time
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepinms));
        } else {
            break;
        }

        i = i + 1;
    }

    if (rc < 0) {
        esyslog("[hbbtv] Unable to bind TCP socket on port %d: %d -> %s", VIDEO_TCP_PORT, nn_errno(), nn_strerror(nn_errno()));
        return false;
    }

    dsyslog("[hbbtv] Waited %d ms for the tcp connect", (i * sleepinms));

    return true;
}

bool HbbtvVideoPlayer::connectUnixSocket() {
    dsyslog("[hbbtv] Connect via Unix domain socket");

    struct sockaddr_un servAddr;
    int rc;

    connectRequested = false;

    videosocket = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (videosocket < 0) {
        esyslog("[hbtv] Unable to open Unix domain socket: %d -> %s", nn_errno(), nn_strerror(nn_errno()));
        return false;
    }

    servAddr.sun_family = AF_LOCAL;
    strcpy(servAddr.sun_path, VIDEO_UNIX);

    // wait some time and try to connect multiple times
    int i = 0;
    const int sleepinms = 20;
    while (i < 200) {
        struct stat sb{};
        if (lstat(VIDEO_UNIX, &sb) != 0) {
            // wait some time
            std::this_thread::sleep_for(std::chrono::milliseconds(sleepinms));
        } else {
            break;
        }

        i = i + 1;
    }

    dsyslog("[hbbtv] Waited %d ms for the unix domain socket", (i * sleepinms));

    rc = connect(videosocket, (struct sockaddr *) &servAddr, sizeof (servAddr));
    if (rc < 0) {
        esyslog("[hbbtv] Unable to connect to Unix domain socket '%s': %d -> %s", VIDEO_UNIX, nn_errno(), nn_strerror(nn_errno()));
        return false;
    }

    return true;
}

void HbbtvVideoPlayer::Reconnect() {
    connectRequested = true;
}

void HbbtvVideoPlayer::startUdpVideoReader() {
    dsyslog("[hbbtv] Start UDP video reader");

    socklen_t len;
    struct sockaddr_in cliAddr;
    int n;

    if (!connectUdp()) {
        return;
    }

    uint8_t buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    while (Running()) {
        if (connectRequested) {
            closeSocket();
            connectUdp();
            filled = 0;
        }

        if (filled) {
            memcpy(&buffer[0], tsbuf, filled);
        }

        // read next packet
        len = sizeof (cliAddr);
        n = recvfrom(videosocket, &buffer[filled], BUFFER_SIZE - filled, 0, (struct sockaddr *) &cliAddr, &len);

        if (n < 0) {
            continue;
        }

        PlayPacket(&buffer[0], n);
    }

    dsyslog("[hbbtv] Stop UDP video reader");
}

void HbbtvVideoPlayer::startTcpVideoReader() {
    dsyslog("[hbbtv] Start TCP video reader");

    int n;

    if (!connectTcp()) {
        return;
    }

    uint8_t buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    while (Running()) {
        if (connectRequested) {
            closeSocket();
            connectTcp();
            filled = 0;
        }

        if (filled) {
            memcpy(&buffer[0], tsbuf, filled);
        }

        // read next packet
        n = recv(videosocket, &buffer[filled], BUFFER_SIZE - filled, 0);

        if (n < 0) {
            continue;
        }

        PlayPacket(&buffer[0], n);
    }
}

void HbbtvVideoPlayer::startUnixVideoReader() {
    dsyslog("[hbbtv] Start Unix socket video reader");

    int n;

    if (!connectUnixSocket()) {
        return;
    }

    uint8_t buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    while (Running()) {
        if (connectRequested) {
            closeSocket();
            connectUnixSocket();
            filled = 0;
        }

        if (filled) {
            memcpy(&buffer[0], tsbuf, filled);
        }

        // read next packet
        n = recv(videosocket, &buffer[filled], BUFFER_SIZE - filled, 0);

        if (n < 0) {
            continue;
        }

        PlayPacket(&buffer[0], n);
    }

    dsyslog("[hbbtv] Stop Unix socket video reader");
}

void HbbtvVideoPlayer::PlayPacket(uint8_t *buffer, int len) {
    len = len + filled;
    if ((filled = (len % 188)) != 0) {
        int fullpackets = len / 188;
        len = fullpackets * 188; // send only full ts packets
        memcpy(tsbuf, &buffer[len], filled); // save partial tspacket
    }

    if (len) { // at least one tspacket
        HBBTV_DBG("[hbbtv] Received TS packet with length %d", len);

        PlayTs(buffer, len);
    }

    if (len % 188 != 0) {
        esyslog("Got Video data, but size is not a multiple of 188: %d", len);
    }
}

void HbbtvVideoPlayer::readTsFrame(uint8_t *buf, int bufsize) {
    hbbtvVideoPlayer->PlayTs((uchar*)buf, bufsize);
}

HbbtvVideoControl::HbbtvVideoControl(cPlayer* Player, bool Hidden) : cControl(Player) {
    dsyslog("[hbbtv] Create Control...");
}

HbbtvVideoControl::~HbbtvVideoControl() {
    dsyslog("[hbbtv] Delete Control...");
    if (hbbtvVideoPlayer != nullptr) {
        delete hbbtvVideoPlayer;
    }
}

void HbbtvVideoControl::Hide(void) {
    dsyslog("[hbbtv] Hide Control...");
}

cOsdObject* HbbtvVideoControl::GetInfo(void) {
    dsyslog("[hbbtv] GetInfo Control...");
    return nullptr;
}

cString HbbtvVideoControl::GetHeader(void) {
    dsyslog("[hbbtv] Get Header Control...");
    return "";
}

eOSState HbbtvVideoControl::ProcessKey(eKeys Key) {
    switch (int(Key)) {
        case kBack:
            // stop player mode an return to TV
            // TODO: Implement me
            return osEnd;
            break;
        default:
            // send all other keys to the browser
            browserComm->SendKey(Key);
            return osContinue;
    }
}
