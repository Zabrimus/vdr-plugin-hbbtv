#include <nanomsg/nn.h>
#include "hbbtvvideocontrol.h"

HbbtvVideoPlayer *player;
bool isHbbtvPlayerActivated;

HbbtvVideoPlayer::HbbtvVideoPlayer() {
    fprintf(stderr, "Create Player...\n");
    player = this;
}

HbbtvVideoPlayer::~HbbtvVideoPlayer() {
    fprintf(stderr, "Delete Player...\n");
}

void HbbtvVideoPlayer::Activate(bool On) {
    fprintf(stderr, "Activate video player: %s\n", On ? " Ja" : "Nein");

    if (On) {
        isHbbtvPlayerActivated = true;
    } else {
        isHbbtvPlayerActivated = false;
    }
}

void HbbtvVideoPlayer::Action(void) {
    // FIXME: What shall happen here?
    fprintf(stderr, "In Player Action\n");
}

void HbbtvVideoPlayer::readTsFrame(int socketId) {
    int bytes;
    uint8_t buffer[32712];

    if ((bytes = nn_recv(socketId, &buffer, 32712, 0)) < 0) {
        // stop this thread
        fprintf(stderr, "Error in recv: %s -> %d\n", strerror(errno), bytes);
        esyslog("Read socket error. Stop playing...");
        return;
    }

    player->PlayTs((uchar*)buffer, bytes);
}

HbbtvVideoControl::HbbtvVideoControl(cPlayer* Player, bool Hidden) : cControl(Player) {
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
