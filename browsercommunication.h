#ifndef HBBTV_BROWSERCOMMUNICATION_H
#define HBBTV_BROWSERCOMMUNICATION_H

#include <map>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pipeline.h>
#include <vdr/thread.h>

class BrowserCommunication : public cThread {

private:
    const char* pluginName;

    cString ipcToVdrFile = cString::sprintf("/tmp/vdrosr_tovdr.ipc");
    int inSocketId;
    int inEndpointId;

    cString ipcToBrowserFile = cString::sprintf("/tmp/vdrosr_tobrowser.ipc");
    int outSocketId;
    int outEndpointId;

    std::map<eKeys, std::string> keyMapping;
    void initKeyMapping();

protected:
    void Action(void) override;

public:
    BrowserCommunication(const char* name);
    ~BrowserCommunication();

    bool SendToBrowser(const char* command, bool readResponse = false);

    bool SendKey(std::string key);
    bool SendKey(eKeys Key);
    bool SendKey(cString key);

    bool Heartbeat();
};

extern BrowserCommunication *browserComm;

#endif // HBBTV_BROWSERCOMMUNATION_H