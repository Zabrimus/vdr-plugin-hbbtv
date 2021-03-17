#ifndef HBBTV_BROWSERCOMMUNICATION_H
#define HBBTV_BROWSERCOMMUNICATION_H

#include <map>
#include <vdr/thread.h>
#include <vdr/keys.h>

const uint8_t CMD_STATUS = 1;
const uint8_t CMD_OSD = 2;
const uint8_t CMD_VIDEO = 3;
const uint8_t CMD_PING = 5;

class BrowserCommunication : public cThread {

private:
    const char* pluginName;

    std::map<eKeys, std::string> keyMapping;
    void initKeyMapping();

    time_t lastHeartbeat;

protected:
    void Action(void) override;

public:
    explicit BrowserCommunication(const char* name);
    ~BrowserCommunication() override;

    bool SendToBrowser(const char* command);

    bool SendKey(std::string key);
    bool SendKey(eKeys Key);
    bool SendKey(cString key);

    bool Heartbeat();
};

extern BrowserCommunication *browserComm;

#endif // HBBTV_BROWSERCOMMUNATION_H
