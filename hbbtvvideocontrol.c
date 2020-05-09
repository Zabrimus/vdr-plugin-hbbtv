#include <nanomsg/nn.h>
#include "hbbtvvideocontrol.h"

HbbtvVideoPlayer *player;
bool isHbbtvPlayerActivated;

HbbtvVideoPlayer::HbbtvVideoPlayer() {
    dsyslog("Create Player...");
    player = this;
}

HbbtvVideoPlayer::~HbbtvVideoPlayer() {
    dsyslog("Delete Player...");
}

void HbbtvVideoPlayer::Activate(bool On) {
    dsyslog("Activate video player: %s", On ? " Ja" : "Nein");

    if (On) {
        isHbbtvPlayerActivated = true;
    } else {
        isHbbtvPlayerActivated = false;
    }
}

void HbbtvVideoPlayer::Action(void) {
    // FIXME: What shall happen here?
    dsyslog("In Player Action");
}

void HbbtvVideoPlayer::readTsFrame(uint8_t *buf, int bufsize) {
    player->PlayTs((uchar*)buf, bufsize);
}

HbbtvVideoControl::HbbtvVideoControl(cPlayer* Player, bool Hidden) : cControl(Player) {
    dsyslog("Create Control...");
}

HbbtvVideoControl::~HbbtvVideoControl() {
    dsyslog("Delete Control...");
}

void HbbtvVideoControl::Hide(void) {
    dsyslog("Hide Control...");
}

cOsdObject* HbbtvVideoControl::GetInfo(void) {
    dsyslog("GetInfo Control...");
    return nullptr;
}

cString HbbtvVideoControl::GetHeader(void) {
    dsyslog("Get Header Control...");
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
