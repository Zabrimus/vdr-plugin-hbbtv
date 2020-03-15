#include <unistd.h>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pipeline.h>
#include <vdr/tools.h>
#include "nngsocket.h"

std::string ipcCommandFile("/tmp/vdrosr_command.ipc");
std::string ipcStreamFile("/tmp/vdrosr_stream.ipc");
std::string ipcStatusFile("/tmp/vdrosr_status.ipc");
std::string ipcVideoFile("/tmp/vdrosr_video.ipc");

int NngSocket::commandSocketId;
int NngSocket::commandEndpointId;

int NngSocket::streamSocketId;
int NngSocket::streamEndpointId;

int NngSocket::statusSocketId;
int NngSocket::statusEndpointId;

int NngVideoSocket::videoSocketId;
int NngVideoSocket::videoEndpointId;

NngSocket::NngSocket() {
    fprintf(stderr, "Create Sockets nng...\n");

    // create all sockets

    // command socket
    auto commandUrl = std::string("ipc://");
    commandUrl.append(ipcCommandFile);

    if ((commandSocketId = nn_socket(AF_SP, NN_REQ)) < 0) {
        esyslog("Unable to create socket");
    }

    if ((commandEndpointId = nn_connect(commandSocketId, commandUrl.c_str())) < 0) {
        esyslog("unable to connect nanomsg socket to %s\n", commandUrl.c_str());
    }

    // set timeout
    int to = 2000;
    nn_setsockopt (commandSocketId, NN_SOL_SOCKET, NN_RCVTIMEO, &to, sizeof (to));
    nn_setsockopt (commandSocketId, NN_SOL_SOCKET, NN_SNDTIMEO, &to, sizeof (to));

    // stream socket
    auto streamUrl = std::string("ipc://");
    streamUrl.append(ipcStreamFile);

    if ((streamSocketId = nn_socket(AF_SP, NN_PULL)) < 0) {
        esyslog("unable to create nanomsg socket");
    }

    if ((streamEndpointId = nn_connect(streamSocketId, streamUrl.c_str())) < 0) {
        esyslog("unable to connect nanomsg socket to %s", streamUrl.c_str());
    }

    // status socket
    auto statusUrl = std::string("ipc://");
    statusUrl.append(ipcStatusFile);

    if ((statusSocketId = nn_socket(AF_SP, NN_PULL)) < 0) {
        esyslog("unable to create nanomsg socket");
    }

    if ((statusEndpointId = nn_connect(statusSocketId, statusUrl.c_str())) < 0) {
        esyslog("unable to connect nanomsg socket to %s", statusUrl.c_str());
    }
}

NngSocket::~NngSocket() {
    fprintf(stderr, "Delete Sockets nng...\n");

    nn_close(streamSocketId);
    nn_close(commandSocketId);
    nn_close(statusSocketId);

    unlink(ipcCommandFile.c_str());
    unlink(ipcStreamFile.c_str());
    unlink(ipcStatusFile.c_str());
}

NngVideoSocket::NngVideoSocket() {
    fprintf(stderr, "Create Video Sockets nng...\n");

    // create all sockets

    // video socket
    auto videoUrl = std::string("ipc://");
    videoUrl.append(ipcVideoFile);

    if ((videoSocketId = nn_socket(AF_SP, NN_REQ)) < 0) {
        esyslog("Unable to create socket");
    }

    if ((videoEndpointId = nn_connect(videoSocketId, videoUrl.c_str())) < 0) {
        esyslog("unable to connect nanomsg socket to %s\n", videoUrl.c_str());
    }
}

NngVideoSocket::~NngVideoSocket() {
    fprintf(stderr, "Delete Video Sockets nng...\n");

    nn_close(videoSocketId);
    unlink(ipcVideoFile.c_str());
}
