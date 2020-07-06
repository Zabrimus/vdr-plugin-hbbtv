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

#define BUFFER_SIZE 31960

HbbtvVideoPlayer *hbbtvVideoPlayer;
bool isHbbtvPlayerActivated;

HbbtvVideoPlayer::HbbtvVideoPlayer() {
    dsyslog("[hbbtv] Create Player...");
    hbbtvVideoPlayer = this;
    udpsock = -1;
}

HbbtvVideoPlayer::~HbbtvVideoPlayer() {
    dsyslog("[hbbtv] Delete Player...");

    setVideoDefaultSize();

    Detach();

    if (udpsock != -1) {
        int cl = close(udpsock);
        if (cl < 0) {
            esyslog("[hbtv] Unable to close UDP socket: %s", strerror(errno));
        }

        udpsock = -1;
    }

    if (Running()) {
        Cancel(-1);
    }
    hbbtvVideoPlayer = nullptr;

    if (!browserComm->SendToBrowser("PLAYER_DETACHED")) {
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
    // FIXME: What shall happen here?
    dsyslog("[hbbtv] Start UDP video reader");

    socklen_t len;
    struct sockaddr_in cliAddr, servAddr;
    int n, rc;
    const int y = 1;

    // create UDP socket
    udpsock = socket (AF_INET, SOCK_DGRAM, 0);
    if (udpsock < 0) {
        esyslog("[hbtv] Unable to open UDP socket: %s", strerror(errno));
        return;
    }

    // bind socket
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

    servAddr.sin_port = htons (VIDEO_UDP_PORT);
    setsockopt(udpsock, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int));

    rc = bind(udpsock, (struct sockaddr *) &servAddr, sizeof (servAddr));
    if (rc < 0) {
        esyslog("[hbbtv] Unable to bind UDP socket on port %d: %s", VIDEO_UDP_PORT, strerror(errno));
        return;
    }

    uint8_t buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    while (Running()) {
        // read next packet
        len = sizeof (cliAddr);
        n = recvfrom(udpsock, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &cliAddr, &len);

        if (n < 0) {
            continue;
        }

        if (n % 188 != 0) {
            esyslog("Got Video data, but size is not a multiple of 188: %d", n);
        }

        PlayTs(buffer, n);
    }

    dsyslog("[hbbtv] Stop UDP video reader");
}

void HbbtvVideoPlayer::readTsFrame(uint8_t *buf, int bufsize) {
    hbbtvVideoPlayer->PlayTs((uchar*)buf, bufsize);
}

void HbbtvVideoPlayer::SetVideoSize() {
    dsyslog("[hbbtv] SetVideoSize in video player: x=%d, y=%d, width=%d, height=%d", video_x, video_y, video_width, video_height);

    // calculate the new coordinates
    if (isVideoFullscreen()) {
        // fullscreen
        cRect r = {0,0,0,0};
        cDevice::PrimaryDevice()->ScaleVideo(r);
    } else {
        int osdWidth;
        int osdHeight;
        double osdPh;
        cDevice::PrimaryDevice()->GetOsdSize(osdWidth, osdHeight, osdPh);

        int newX, newY, newWidth, newHeight;
        calcVideoPosition(&newX, &newY, &newWidth, &newHeight);

        cRect r = {newX, newY, newWidth, newHeight};
        cDevice::PrimaryDevice()->ScaleVideo(r);
    }
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
