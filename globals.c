#include "globals.h"

int video_x, video_y;
int video_width, video_height;

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