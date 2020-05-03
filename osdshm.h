#ifndef OSDSHM_H
#define OSDSHM_H

#include <vdr/osd.h>

// FULL HD
const int SHM_BUF_SIZE = (1920 * 1080 * 4);
const int SHM_KEY = 0xDEADC0DE;

class OsdShm {

private:
    int shmid;
    uint8_t *shmp;

public:
    OsdShm();
    ~OsdShm();

    void copyTo(tColor *dest, int size);
};

extern OsdShm osd_shm;

#endif // OSDSHM_H
