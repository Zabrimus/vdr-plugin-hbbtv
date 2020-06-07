#include <nanomsg/nn.h>
#include "hbbtvvideocontrol.h"

HbbtvVideoPlayer *hbbtvVideoPlayer;
bool isHbbtvPlayerActivated;

HbbtvVideoPlayer::HbbtvVideoPlayer() {
    dsyslog("[hbbtv] Create Player...");
    hbbtvVideoPlayer = this;
}

HbbtvVideoPlayer::~HbbtvVideoPlayer() {
    dsyslog("[hbbtv] Delete Player...");
    Detach();
    hbbtvVideoPlayer = nullptr;

    if (!browserComm->SendToBrowser("PLAYER_DETACHED")) {

    }
}

void HbbtvVideoPlayer::Activate(bool On) {
    dsyslog("[hbbtv] Activate video player: %s", On ? " Ja" : "Nein");

    if (On) {
        isHbbtvPlayerActivated = true;
    } else {
        isHbbtvPlayerActivated = false;
    }
}

void HbbtvVideoPlayer::Action(void) {
    // FIXME: What shall happen here?
    dsyslog("[hbbtv] In Player Action");
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
