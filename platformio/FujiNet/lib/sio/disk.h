#ifndef DISK_H
#define DISK_H
#include <Arduino.h>

#include "sio.h"
#include <FS.h>

unsigned short para_to_num_sectors(unsigned short para, unsigned char para_hi, unsigned short ss);
unsigned long num_sectors_to_para(unsigned short num_sectors, unsigned short sector_size);

class sioDisk : public sioDevice
{
private:
    File *_file;

    byte sectorSize = 128;
    byte sector[256];

    byte sectorCache[2560];
    int firstCachedSector = 65535;
    unsigned char max_cached_sectors = 19;

    struct
    {
        unsigned char num_tracks;
        unsigned char step_rate;
        unsigned char sectors_per_trackM;
        unsigned char sectors_per_trackL;
        unsigned char num_sides;
        unsigned char density;
        unsigned char sector_sizeM;
        unsigned char sector_sizeL;
        unsigned char drive_present;
        unsigned char reserved1;
        unsigned char reserved2;
        unsigned char reserved3;
    } percomBlock;

    void sio_read();
    void sio_write();
    void sio_format();
    void sio_status() override;
    void sio_process() override;

    void derive_percom_block(unsigned short numSectors);
    void sio_read_percom_block();
    void sio_write_percom_block();
    void dump_percom_block();

public:
    void mount(File *f);
    File *file();
};

#endif // guard