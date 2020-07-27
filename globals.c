#include <sys/types.h>
#include <sys/wait.h>
#include "globals.h"

int video_x, video_y;
int video_width, video_height;

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

    if (0 == kill(OsrBrowserPid, 0))
        // process exists and is alive
        return 1;

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