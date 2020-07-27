#ifndef GLOBALS_H
#define GLOBALS_H

#include <vdr/player.h>

// current video size and coordinates
extern int video_x, video_y;
extern int video_width, video_height;

// pid of the browser if the plugin by the plugin
extern pid_t OsrBrowserPid;
int isBrowserAlive();

int isVideoFullscreen();
void setVideoDefaultSize();
void SetVideoSize();
void calcVideoPosition(int *x, int *y, int *width, int *height);

#endif // GLOBALS_H
