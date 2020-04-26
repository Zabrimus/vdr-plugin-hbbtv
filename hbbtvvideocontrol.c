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

void HbbtvVideoPlayer::readTsFrame(uint8_t *buf, int bufsize) {
    player->PlayTs((uchar*)buf, bufsize);
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
