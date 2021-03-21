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
#include <vdr/remote.h>
#include "hbbtvvideocontrol.h"
#include "globals.h"
#include "sharedmemory.h"

#define BUFFER_SIZE 65424

uint8_t testbuffer[BUFFER_SIZE];

HbbtvVideoPlayer *hbbtvVideoPlayer;
bool isHbbtvPlayerActivated;

HbbtvVideoPlayer::HbbtvVideoPlayer() {
    dsyslog("[hbbtv] Create Player...");
    hbbtvVideoPlayer = this;
    filled = 0;
}

HbbtvVideoPlayer::~HbbtvVideoPlayer() {
    dsyslog("[hbbtv] Delete Player...");
    packetReaderRunning = false;

    setVideoDefaultSize();

    Detach();

    if (Running()) {
        Cancel(-1);
    }
    hbbtvVideoPlayer = nullptr;

    if (!browserComm->SendToBrowser("PLAYER_DETACHED")) {
        // ???
    }

    sharedMemory.setMode(shmpWriteMode, Data);
}

void HbbtvVideoPlayer::Activate(bool On) {
    dsyslog("[hbbtv] Activate video player: %s", On ? " Ja" : "Nein");

    if (On) {
        isHbbtvPlayerActivated = true;
        packetReaderRunning = true;
        packetReaderThread = std::thread(&HbbtvVideoPlayer::newPacketReceived, this);
    } else {
        isHbbtvPlayerActivated = false;
        packetReaderRunning = false;

        packetReaderThread.join();

        setVideoDefaultSize();

        cRect r = {0,0,0,0};
        cDevice::PrimaryDevice()->ScaleVideo(r);
    }
}

void HbbtvVideoPlayer::Action(void) {
    dsyslog("[hbbtv] HbbtvVideoPlayer::Action");
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

void HbbtvVideoPlayer::newPacketReceived() {
    HBBTV_DBG("[hbbtv] HbbtvVideoPlayer::newPacketReceived: %s", (packetReaderRunning ? "running" : "stopped"));

    while (packetReaderRunning) {
        uint8_t *buffer;
        int size;

        if (sharedMemory.waitForRead(Data) != -1) {
            sharedMemory.read(&buffer, &size, Data);
            PlayPacket(buffer, size);
            sharedMemory.finishedReading(Data);
        }
    }
}

HbbtvVideoControl::HbbtvVideoControl(HbbtvVideoPlayer* Player, bool Hidden) : cControl(Player) {
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
    // dsyslog("[hbbtv] HbbtvVideoControl::ProcessKey");

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
