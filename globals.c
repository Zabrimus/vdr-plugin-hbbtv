#include <sys/types.h>
#include <sys/wait.h>
#include <vdr/channels.h>
#include "globals.h"
#include "browsercommunication.h"

int video_x, video_y;
int video_width, video_height;

std::atomic_bool browserStarted;
pid_t OsrBrowserPid = -1;

int isBrowserAlive() {
    if (OsrBrowserPid == -1) {
        // the plugin is not responsible for starting the browser
        return -1;
    }

    if (OsrBrowserPid == 0) {
        // either the browser has not yet been started or has been killed
        return 0;
    }

    while(waitpid(OsrBrowserPid, 0, WNOHANG) > 0) {
        // Wait for defunct....
    }

    if (0 == kill(OsrBrowserPid, 0)) {
        // process exists and is alive
        return 1;
    }

    // either the browser has not yet been started or has been killed
    return 0;
}

void sendChannelToBrowser(int channelNumber) {
#if APIVERSNUM >= 20301
    LOCK_CHANNELS_READ
    auto channel = Channels->GetByNumber(channelNumber);
    const char* currentChannel = channel->Name();
#else
    auto channel = Channels->GetByNumber(channelNumber);
         sid = channel->Sid();
         const char* currentChannel = channel->Name();
#endif
    // inform browser about the channel switch

    // longName, Name => currentChannel
    // nid            => ??? (use 1 as default)
    // onid           => channel, Nid
    // sid            => channel, Sid
    // tsid           => channel, Tid
    // channelType    => HDTV 0x19, TV 0x01, Radio 0x02
    // idType         => ??? (use 15 as default)
    int channelType;

    if (strstr(currentChannel, "HD") != NULL) {
        channelType = 0x19;
    } else if (channel->Rid() > 0) {
        channelType = 0x02;
    } else {
        channelType = 0x01;
    }

    char *cmd;
    asprintf(&cmd, "CHANNEL {\"channelType\":%d,\"ccid\":\"ccid://1.0\",\"nid\":%d,\"dsd\":\"\",\"onid\":%d,\"tsid\":%d,\"sid\":%d,\"name\":\"%s\",\"longName\":\"%s\",\"description\":\"OIPF (SD&amp;S) - TCServiceData doesn’t support yet!\",\"authorised\":true,\"genre\":null,\"hidden\":false,\"idType\":%d,\"channelMaxBitRate\":0,\"manualBlock\":false,\"majorChannel\":1,\"ipBroadcastID\":\"rtp://1.2.3.4/\",\"locked\":false}", channelType, 1, channel->Nid(), channel->Tid(), channel->Sid(), currentChannel, currentChannel, 15);
    browserComm->SendToBrowser(cmd);
    free(cmd);
}

int isVideoFullscreen() {
    if ((video_x == 0) && (video_y == 0) && (video_width == 1280) && (video_height == 720)) {
        return 1;
    }

    return 0;
}

void setVideoDefaultSize() {
    video_x = 0;
    video_y = 0;
    video_width = 1280;
    video_height = 720;
}

void calcVideoPosition(int *x, int *y, int *width, int *height) {
    int osdWidth;
    int osdHeight;
    double osdPh;
    cDevice::PrimaryDevice()->GetOsdSize(osdWidth, osdHeight, osdPh);

    *x = (video_x * osdWidth) / 1280;
    *y = (video_y * osdHeight) / 720;
    *width = (video_width * osdWidth) / 1280;
    *height = (video_height * osdHeight) / 720;
}

void SetVideoSize() {
    dsyslog("[hbbtv] SetVideoSize in video player: x=%d, y=%d, width=%d, height=%d", video_x, video_y, video_width, video_height);

    // calculate the new coordinates
    if (isVideoFullscreen()) {
        // fullscreen
        cRect r = {0,0,0,0};
        cDevice::PrimaryDevice()->ScaleVideo(r);
    } else {
        int osdWidth;
        int osdHeight;
        double osdPh;
        cDevice::PrimaryDevice()->GetOsdSize(osdWidth, osdHeight, osdPh);

        int newX, newY, newWidth, newHeight;
        calcVideoPosition(&newX, &newY, &newWidth, &newHeight);

        cRect r = {newX, newY, newWidth, newHeight};
        cDevice::PrimaryDevice()->ScaleVideo(r);
    }
}
