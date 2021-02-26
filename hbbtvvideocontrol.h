#ifndef HBBTV_HBBTVVIDEOCONTROL_H
#define HBBTV_HBBTVVIDEOCONTROL_H

#include <sys/socket.h>
#include <errno.h>
#include <sys/un.h>
#include <thread>
#include <vdr/osd.h>
#include <vdr/player.h>
#include <vdr/tools.h>
#include "browsercommunication.h"
#include "cefhbbtvpage.h"
#include "osdshm.h"

class HbbtvVideoPlayer : public cPlayer, cThread {
    friend BrowserCommunication;

    private:
        void PlayPacket(uint8_t *buffer, int len);

        uint8_t tsbuf[188];
        int filled;

        bool packetReaderRunning;
        std::thread packetReaderThread;

    protected:
        void Activate(bool On) override;
        void Action(void) override;

    public:
        HbbtvVideoPlayer();
        ~HbbtvVideoPlayer();

        void newPacketReceived();
};

class HbbtvVideoControl : public cControl {
    public:
        HbbtvVideoControl(HbbtvVideoPlayer* Player, bool Hidden = false);
        ~HbbtvVideoControl();
        void Hide(void) override;
        cOsdObject *GetInfo(void) override;
        cString GetHeader(void) override;
        eOSState ProcessKey(eKeys Key) override;
};

extern bool isHbbtvPlayerActivated;
extern HbbtvVideoPlayer *hbbtvVideoPlayer;

#endif // HBBTV_HBBTVVIDEOCONTROL_H
