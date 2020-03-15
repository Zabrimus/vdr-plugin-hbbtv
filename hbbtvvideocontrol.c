#include <nanomsg/nn.h>
#include "hbbtvvideocontrol.h"

HbbtvVideoPlayer *player;

HbbtvVideoPlayer::HbbtvVideoPlayer() {
    fprintf(stderr, "Create Player...\n");

    nngsocket = new NngVideoSocket();

    isRunning = false;
    player = this;
}

HbbtvVideoPlayer::~HbbtvVideoPlayer() {
    fprintf(stderr, "Delete Player...\n");

    DELETENULL(nngsocket);
}

void HbbtvVideoPlayer::Activate(bool On) {
    fprintf(stderr, "Activate video player: %s\n", On ? " Ja" : "Nein");

    if (On) {
        isRunning = true;
        updateThread = new std::thread(readTsStream);
    } else {
        stopStream();
    }
}

void HbbtvVideoPlayer::readTsStream() {
    int bytes;
    char buffer[18800];

    fprintf(stderr, "Enter readTsStream()\n");

    while(player->isRunning) {
        if ((bytes = nn_recv(NngVideoSocket::getVideoSocket(), &buffer, 18800, 0)) < 0) {
            // stop this thread
            fprintf(stderr, "Error in recv: %s -> %d\n", strerror(errno), bytes);
            esyslog("Read socket error. Stop playing...");
            player->stopStream();
            return;
        }

        fprintf(stderr, "Send TS packet...\n");

        player->PlayTs((uchar*)buffer, bytes);
    }

    fprintf(stderr, "ReadTsStream end...\n");
}

void HbbtvVideoPlayer::stopStream() {
    fprintf(stderr, "Player stopStream...\n");

    if (!isRunning) {
        return;
    }

    fprintf(stderr, "Stop Update Thread...\n");

    isRunning = false;
}

HbbtvVideoControl::HbbtvVideoControl(cPlayer* Player, bool Hidden) : cControl(player = new HbbtvVideoPlayer) {
    fprintf(stderr, "Create Control...\n");
}

HbbtvVideoControl::~HbbtvVideoControl() {
    fprintf(stderr, "Delete Control...\n");
}

void HbbtvVideoControl::Hide(void) {
    fprintf(stderr, "Hide Control...\n");
}

cOsdObject* HbbtvVideoControl::GetInfo(void) {
    fprintf(stderr, "GetInfo Control...\n");
    return nullptr;
}

cString HbbtvVideoControl::GetHeader(void) {
    fprintf(stderr, "Create Header Control...\n");
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
