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

class HbbtvVideoPlayer : public cPlayer, cThread {
    friend BrowserCommunication;

    private:
        int udpsock;

        void readTsFrame(uint8_t *buf, int bufsize);

    protected:
        void Activate(bool On) override;
        void Action(void) override;

    public:
        HbbtvVideoPlayer();
        ~HbbtvVideoPlayer();

        void SetVideoSize();
};

class HbbtvVideoControl : public cControl {
    public:
        HbbtvVideoControl(cPlayer* Player, bool Hidden = false);
        ~HbbtvVideoControl();
        void Hide(void) override;
        cOsdObject *GetInfo(void) override;
        cString GetHeader(void) override;
        eOSState ProcessKey(eKeys Key) override;
};

extern bool isHbbtvPlayerActivated;
extern HbbtvVideoPlayer *hbbtvVideoPlayer;

#endif // HBBTV_HBBTVVIDEOCONTROL_H
