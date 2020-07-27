#include <sys/types.h>
#include <sys/wait.h>
#include "globals.h"

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
