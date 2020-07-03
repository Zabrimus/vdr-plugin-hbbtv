#ifndef HBBTV_BROWSERCOMMUNICATION_H
#define HBBTV_BROWSERCOMMUNICATION_H

#include <map>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pipeline.h>
#include <vdr/thread.h>

// #define TO_VDR_CHANNEL "ipc:///tmp/vdrosr_tovdr.ipc"
// #define FROM_VDR_CHANNEL "ipc:///tmp/vdrosr_tobrowser.ipc"

#define TO_VDR_CHANNEL "tcp://127.0.0.1:5560"
#define FROM_VDR_CHANNEL "tcp://127.0.0.1:5561"
#define VIDEO_UDP_PORT 5560

class BrowserCommunication : public cThread {

private:
    const char* pluginName;

    cString ipcToVdrFile = cString::sprintf(TO_VDR_CHANNEL);
    int inSocketId;
    int inEndpointId;

    cString ipcToBrowserFile = cString::sprintf(FROM_VDR_CHANNEL);
    int outSocketId;
    int outEndpointId;

    std::map<eKeys, std::string> keyMapping;
    void initKeyMapping();

    time_t lastHeartbeat;

protected:
    void Action(void) override;

public:
    BrowserCommunication(const char* name);
    ~BrowserCommunication();

    bool SendToBrowser(const char* command);
    cString ReadResponse();

    bool SendKey(std::string key);
    bool SendKey(eKeys Key);
    bool SendKey(cString key);

    bool Heartbeat();
};

extern BrowserCommunication *browserComm;

#endif // HBBTV_BROWSERCOMMUNATION_H
