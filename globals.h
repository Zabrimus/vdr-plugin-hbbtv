#ifndef GLOBALS_H
#define GLOBALS_H

#include <atomic>
#include <vdr/player.h>
#include <vdr/tools.h>

const bool HBBTV_PLUGIN_DEBUG = false;
// const bool HBBTV_PLUGIN_DEBUG = true;

#define HBBTV_DBG(...) if (HBBTV_PLUGIN_DEBUG) { (fprintf(stderr, __VA_ARGS__)); fprintf(stderr, "\n"); }

// current video size and coordinates
extern int video_x, video_y;
extern int video_width, video_height;

// pid of the browser if the plugin by the plugin
extern pid_t OsrBrowserPid;
extern std::atomic_bool browserStarted;
int isBrowserAlive();

// Plugin configuration directory
extern const char* pluginConfigDirectory;

// send channel information to the browser
void sendChannelToBrowserData(const char* name, int nid, int tid, int sid, int rid);
void sendChannelToBrowser(int channelNumber);

int isVideoFullscreen();
void setVideoDefaultSize();
void SetVideoSize();
void calcVideoPosition(int *x, int *y, int *width, int *height);

#endif // GLOBALS_H
