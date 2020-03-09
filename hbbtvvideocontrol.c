#include "hbbtvvideocontrol.h"

HbbtvVideoPlayer *player;

HbbtvVideoPlayer::HbbtvVideoPlayer() {
    isRunning = false;
    player = this;
}

HbbtvVideoPlayer::~HbbtvVideoPlayer() {
    if (fd >= 0) {
        close(fd);
    }

    unlink (VIDEO_SERVER_SOCKET);
}

void HbbtvVideoPlayer::Activate(bool On) {
    if (On) {
        // create server socket and start read thread
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, VIDEO_SERVER_SOCKET);
        if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            esyslog("Unable to create socket: " VIDEO_SERVER_SOCKET);
            esyslog("Video will not be played.");

            isRunning = false;
            return;
        }

        updateThread = new std::thread(readTsStream);
        isRunning = true;
    } else {
        stopStream();
    }
}

void HbbtvVideoPlayer::readTsStream() {
    int len;
    uchar buffer[1316];

    while(player->isRunning) {
        if ((len = recv(player->fd, buffer, 1316, 0)) < 0) {
            // stop this thread
            esyslog("Read socket error. Stop playing.");
            player->stopStream();
            return;
        }

        player->PlayTs(buffer, len);
    }
}

void HbbtvVideoPlayer::stopStream() {
    if (!isRunning) {
        return;
    }

    isRunning = false;
    updateThread->join();
}

HbbtvVideoControl::HbbtvVideoControl(cPlayer* Player, bool Hidden) : cControl(player = new HbbtvVideoPlayer) {
}

HbbtvVideoControl::~HbbtvVideoControl() {
}

void HbbtvVideoControl::Hide(void) {
}

cOsdObject* HbbtvVideoControl::GetInfo(void) {
    return nullptr;
}

cString HbbtvVideoControl::GetHeader(void) {
    return "";
}

eOSState HbbtvVideoControl::ProcessKey(eKeys Key) {
    switch (int(Key)) {
        case kStop:
        case kBack:
            return osEnd;
            break;
        default: break;
    }
    return osContinue;
}
