#ifndef HBBTV_HBBTVVIDEOCONTROL_H
#define HBBTV_HBBTVVIDEOCONTROL_H

#include <sys/socket.h>
#include <errno.h>
#include <sys/un.h>
#include <thread>
#include <vdr/osd.h>
#include <vdr/player.h>
#include <vdr/tools.h>
#include "nngsocket.h"

class HbbtvVideoPlayer : public cPlayer {
    private:
        NngVideoSocket *nngsocket;
        bool isRunning;
        std::thread *updateThread;
        static void readTsStream();

    protected:
        void Activate(bool On) override;
        void stopStream();

    public:
        HbbtvVideoPlayer();
        ~HbbtvVideoPlayer();
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

#endif // HBBTV_HBBTVVIDEOCONTROL_H
