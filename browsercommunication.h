#ifndef HBBTV_BROWSERCOMMUNICATION_H
#define HBBTV_BROWSERCOMMUNICATION_H

#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pipeline.h>
#include <vdr/thread.h>

class BrowserCommunication : public cThread {

private:
    cString ipcToVdrFile = cString::sprintf("/tmp/vdrosr_tovdr.ipc");
    int inSocketId;
    int inEndpointId;

    cString ipcToBrowserFile = cString::sprintf("/tmp/vdrosr_tobrowser.ipc");
    int outSocketId;
    int outEndpointId;

protected:
    void Action(void) override;

public:
    BrowserCommunication();
    ~BrowserCommunication();

    bool SendToBrowser(const char* command);
};

extern BrowserCommunication *browserComm;

#endif // HBBTV_BROWSERCOMMUNATION_H
