#ifndef HBBTV_NNGSOCKET_H
#define HBBTV_NNGSOCKET_H

#include <string>

extern std::string ipcCommandFile;
extern std::string ipcStreamFile;
extern std::string ipcStatusFile;
extern std::string ipcVideoFile;

class NngSocket {
    public:
        NngSocket();
        ~NngSocket();

        static int getCommandSocket() { return commandSocketId; };
        static int getStreamSocket() { return streamSocketId; };
        static int getStatusSocket() { return statusSocketId; };

    private:
        static int commandSocketId;
        static int commandEndpointId;

        static int streamSocketId;
        static int streamEndpointId;

        static int statusSocketId;
        static int statusEndpointId;
};

class NngVideoSocket {
public:
    NngVideoSocket();
    ~NngVideoSocket();

    static int getVideoSocket() { return videoSocketId; };

private:
    static int videoSocketId;
    static int videoEndpointId;
};


#endif // HBBTV_NNGSOCKET_H
