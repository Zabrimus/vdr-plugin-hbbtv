#include <sys/shm.h>
#include "osdshm.h"

OsdShm::OsdShm() {
    dsyslog("[hbbtv] initialize shared memory");

    // init shared memory
    shmid = -1;
    shmp = nullptr;

    shmid = shmget(SHM_KEY, SHM_BUF_SIZE, 0644 | IPC_CREAT | IPC_EXCL);

    if (errno == EEXIST) {
        shmid = shmget(SHM_KEY, SHM_BUF_SIZE, 0644);
    }

    if (shmid == -1) {
        perror("Unable to get shared memory");
        return;
    }

    shmp = (uint8_t *) shmat(shmid, NULL, 0);
    if (shmp == (void *) -1) {
        perror("Unable to attach to shared memory");
        return;
    }
}

OsdShm::~OsdShm() {
    dsyslog("[hbbtv] detach from shared memory");

    if (shmdt(shmp) == -1) {
        perror("Unable to detach from shared memory");
        return;
    }

    shmp = nullptr;

    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        // Either this process or vdrosrbrowser removes the shared memory
    }
}

// memcpy(data2, shmp, osdUpdate->width * osdUpdate->height * 4);

void OsdShm::copyTo(tColor *dest, int size) {
    if (size > SHM_BUF_SIZE) {
        size = SHM_BUF_SIZE;
    }

    if (shmp == nullptr) {
        esyslog("Shared memory in OsdShm does not exists.");
        return;
    }

    if (dest == nullptr) {
        esyslog("Destination for copying the shared memory is NULL.");
        return;
    }

    memcpy(dest, shmp, size);
}

OsdShm osd_shm;