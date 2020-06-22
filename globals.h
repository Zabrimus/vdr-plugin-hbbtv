#ifndef GLOBALS_H
#define GLOBALS_H

#include <vdr/player.h>

// current video size and coordinates
extern int video_x, video_y;
extern int video_width, video_height;

int isVideoFullscreen();
void setVideoDefaultSize();
void calcVideoPosition(int *x, int *y, int *width, int *height);

#endif // GLOBALS_H
