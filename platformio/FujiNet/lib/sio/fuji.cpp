#include "fuji.h"
//#include "disk.h"

//File atrConfig;
// sioFuji theFuji;
//sioDisk configDisk;

TNFSFS TNFS[8]; // up to 8 TNFS servers
// could make a list of 8 pointers and create New TNFS objects at mounting and point to them
// might also need to make the FS pointers so that can use SD, SPIFFS, too

File dir[8];
File atr[8];     // up to 8 disk drives
sioDisk sioD[8]; //

void sioFuji::sio_status()
{
    return;
}

/**
   Scan for networks
*/
void sioFuji::sio_net_scan_networks()
{
    char ret[4] = {0, 0, 0, 0};

    // Scan to computer
    WiFi.mode(WIFI_STA);
    totalSSIDs = WiFi.scanNetworks();
    ret[0] = totalSSIDs;

    sio_to_computer((byte *)ret, 4, false);
}

/**
   Return scanned network entry
*/
void sioFuji::sio_net_scan_result()
{
    bool err = false;
    if (cmdFrame.aux1 < totalSSIDs)
    {
        strcpy(ssidInfo.ssid, WiFi.SSID(cmdFrame.aux1).c_str());
        ssidInfo.rssi = (char)WiFi.RSSI(cmdFrame.aux1);
    }
    else
    {
        memset(ssidInfo.rawData, 0x00, sizeof(ssidInfo.rawData));
        err = true;
    }

    sio_to_computer(ssidInfo.rawData, sizeof(ssidInfo.rawData), err);
}

/**
   Set SSID
*/
void sioFuji::sio_net_set_ssid()
{
    byte ck = sio_to_peripheral((byte *)&netConfig.rawData, sizeof(netConfig.rawData));

    if (sio_checksum(netConfig.rawData, sizeof(netConfig.rawData)) != ck)
    {
        sio_error();
    }
    else
    {
#ifdef DEBUG
        Debug_printf("Connecting to net: %s password: %s\n", netConfig.ssid, netConfig.password);
#endif
        WiFi.begin(netConfig.ssid, netConfig.password);
        // UDP.begin(16384); // move to TNFS.begin
        sio_complete();
    }
}

/**
   SIO get WiFi Status
*/
void sioFuji::sio_net_get_wifi_status()
{
    char wifiStatus = WiFi.status();

    // Update WiFi Status LED
    if (wifiStatus == WL_CONNECTED)
        wifi_led(true);
    else
        wifi_led(false);

    sio_to_computer((byte *)&wifiStatus, 1, false);
}

/**
   SIO TNFS Server Mount
*/
void sioFuji::sio_tnfs_mount_host()
{
    unsigned char hostSlot = cmdFrame.aux1;
    bool err = TNFS[hostSlot].begin(hostSlots.host[hostSlot], TNFS_PORT);
    //bool err = tnfs_mount(hostSlot);

    if (!err)
        sio_error();
    else
        sio_complete();
}

/**
   SIO TNFS Disk Image Mount
*/
void sioFuji::sio_disk_image_mount()
{
    unsigned char deviceSlot = cmdFrame.aux1;
    unsigned char options = cmdFrame.aux2; // 1=R | 2=R/W | 128=FETCH
    char flag[3] = {'r', 0, 0};
    if (options == 2)
    {
        flag[1] = '+';
    }

    atr[deviceSlot] = TNFS[deviceSlots.slot[deviceSlot].hostSlot].open(deviceSlots.slot[deviceSlot].file, flag);
    //todo: implement what does FETCH mean?
    //bool opened = tnfs_open(deviceSlot, options, false);
    if (!atr[deviceSlot])
    {
        sio_error();
    }
    else
    {
        sioD[deviceSlot].mount(&atr[deviceSlot]);
        // moved all this stuff to .mount
        sio_complete();
    }
}

/**
   SIO TNFS Disk Image uMount
*/
void sioFuji::sio_disk_image_umount()
{
    unsigned char deviceSlot = cmdFrame.aux1;
    atr[deviceSlot].close();
    sio_complete(); // always completes.
}

/**
   Open TNFS Directory
*/
void sioFuji::sio_tnfs_open_directory()
{
    char current_entry[256];
    byte hostSlot = cmdFrame.aux1;
    byte ck = sio_to_peripheral((byte *)&current_entry, sizeof(current_entry));

    if (sio_checksum((byte *)&current_entry, sizeof(current_entry)) != ck)
    {
        sio_error();
        return;
    }

#ifdef DEBUG
    Debug_print("FujiNet is opening directory for reading: ");
    Debug_println(current_entry);
#endif

    if (current_entry[0] != '/')
    {
        current_entry[0] = '/';
        current_entry[1] = '\0';
#ifdef DEBUG
        Debug_print("No directory defined for reading, setting to: ");
        Debug_println(current_entry);
#endif
    }

    dir[hostSlot] = TNFS[hostSlot].open(current_entry, "r");

    if (dir[hostSlot])
        sio_complete();
    else
        sio_error();
}

/**
   Read next TNFS Directory entry
*/
void sioFuji::sio_tnfs_read_directory_entry()
{
    char current_entry[256];
    byte hostSlot = cmdFrame.aux2;
    byte len = cmdFrame.aux1;
    //byte ret = tnfs_readdir(hostSlot);
    File f = dir[hostSlot].openNextFile();

    if (!f)
        current_entry[0] = 0x7F; // end of dir
    else
    {
        strcpy(current_entry, f.name());
    }
    sio_to_computer((byte *)&current_entry, len, false);
}

/**
   Close TNFS Directory
*/
void sioFuji::sio_tnfs_close_directory()
{
    byte hostSlot = cmdFrame.aux1;

    dir[hostSlot].close();
    sio_complete();
}

/**
   Read hosts Slots
*/
void sioFuji::sio_read_hosts_slots()
{
    sio_to_computer(hostSlots.rawData, sizeof(hostSlots.rawData), false);
}

/**
   Read Device Slots
*/
void sioFuji::sio_read_device_slots()
{
    load_config = false;
    sio_to_computer(deviceSlots.rawData, sizeof(deviceSlots.rawData), false);
}

/**
   Write hosts slots
*/
void sioFuji::sio_write_hosts_slots()
{
    byte ck = sio_to_peripheral(hostSlots.rawData, sizeof(hostSlots.rawData));

    if (sio_checksum(hostSlots.rawData, sizeof(hostSlots.rawData)) == ck)
    {
        atrConfig.seek(91792, SeekSet);
        atrConfig.write(hostSlots.rawData, sizeof(hostSlots.rawData));
        atrConfig.flush();
        sio_complete();
    }
    else
        sio_error();
}

/**
   Write Device slots
*/
void sioFuji::sio_write_device_slots()
{
    byte ck = sio_to_peripheral(deviceSlots.rawData, sizeof(deviceSlots.rawData));

    if (sio_checksum(deviceSlots.rawData, sizeof(deviceSlots.rawData)) == ck)
    {
        atrConfig.seek(91408, SeekSet);
        atrConfig.write(deviceSlots.rawData, sizeof(deviceSlots.rawData));
        atrConfig.flush();
        sio_complete();
    }
    else
        sio_error();
}

/**
   Get Adapter config.
*/
void sioFuji::sio_get_adapter_config()
{
    strcpy(adapterConfig.ssid, netConfig.ssid);

#ifdef ESP8266
    strcpy(adapterConfig.hostname, WiFi.hostname().c_str());
#else
    strcpy(adapterConfig.hostname, WiFi.getHostname());
#endif

    adapterConfig.localIP[0] = WiFi.localIP()[0];
    adapterConfig.localIP[1] = WiFi.localIP()[1];
    adapterConfig.localIP[2] = WiFi.localIP()[2];
    adapterConfig.localIP[3] = WiFi.localIP()[3];

    adapterConfig.gateway[0] = WiFi.gatewayIP()[0];
    adapterConfig.gateway[1] = WiFi.gatewayIP()[1];
    adapterConfig.gateway[2] = WiFi.gatewayIP()[2];
    adapterConfig.gateway[3] = WiFi.gatewayIP()[3];

    adapterConfig.netmask[0] = WiFi.subnetMask()[0];
    adapterConfig.netmask[1] = WiFi.subnetMask()[1];
    adapterConfig.netmask[2] = WiFi.subnetMask()[2];
    adapterConfig.netmask[3] = WiFi.subnetMask()[3];

    adapterConfig.dnsIP[0] = WiFi.dnsIP()[0];
    adapterConfig.dnsIP[1] = WiFi.dnsIP()[1];
    adapterConfig.dnsIP[2] = WiFi.dnsIP()[2];
    adapterConfig.dnsIP[3] = WiFi.dnsIP()[3];

    adapterConfig.macAddress[0] = WiFi.macAddress()[0];
    adapterConfig.macAddress[1] = WiFi.macAddress()[1];
    adapterConfig.macAddress[2] = WiFi.macAddress()[2];
    adapterConfig.macAddress[3] = WiFi.macAddress()[3];
    adapterConfig.macAddress[4] = WiFi.macAddress()[4];
    adapterConfig.macAddress[5] = WiFi.macAddress()[5];

    sio_to_computer(adapterConfig.rawData, sizeof(adapterConfig.rawData), false);
}

void sioFuji::sio_process()
{
    //   cmdPtr[0xE7] = sio_new_disk;

    switch (cmdFrame.comnd)
    {
    case 'S':
        sio_ack();
        sio_status();
        break;
    case 0xFD:
        sio_ack();
        sio_net_scan_networks();
        break;
    case 0xFC:
        sio_ack();
        sio_net_scan_result();
        break;
    case 0xFB:
        sio_ack();
        sio_net_set_ssid();
        break;
    case 0xFA:
        sio_ack();
        sio_net_get_wifi_status();
        break;
    case 0xF9:
        sio_ack();
        sio_tnfs_mount_host();
        break;
    case 0xF8:
        sio_ack();
        sio_disk_image_mount();
        break;
    case 0xF7:
        sio_ack();
        sio_tnfs_open_directory();
        break;
    case 0xF6:
        sio_ack();
        sio_tnfs_read_directory_entry();
        break;
    case 0xF5:
        sio_ack();
        sio_tnfs_close_directory();
    case 0xF4:
        sio_ack();
        sio_read_hosts_slots(); // 0xF4
        break;
    case 0xF3:
        sio_ack();
        sio_write_hosts_slots(); // 0xF3
        break;
    case 0xF2:
        sio_ack();
        sio_read_device_slots(); // 0xF2
        break;
    case 0xF1:
        sio_ack();
        sio_write_device_slots(); // 0xF1
        break;
    case 0xE9:
        sio_ack();
        sio_disk_image_umount();
        break;
    case 0xE8:
        sio_ack();
        sio_get_adapter_config();
        break;
    case 0xE7:
        //sio_ack();
        //sio_new_disk();
        //break;
    default:
        sio_nak();
    }
}

/**
   Set WiFi LED
*/
void sioFuji::wifi_led(bool onOff)
{
#ifdef ESP8266
    digitalWrite(PIN_LED, (onOff ? LOW : HIGH));
#elif defined(ESP32)
    digitalWrite(PIN_LED1, (onOff ? LOW : HIGH));
#endif
}

void sioFuji::begin()
{
    // set up Fuji device
    atrConfig = SPIFFS.open("/autorun.atr", "r+");

    // Go ahead and read the host slots from disk
    atrConfig.seek(91792, SeekSet);
    atrConfig.read(hostSlots.rawData, 256);

    // And populate the device slots
    atrConfig.seek(91408, SeekSet);
    atrConfig.read(deviceSlots.rawData, 304);

    // Go ahead and mark all device slots local
    for (int i = 0; i < 8; i++)
    {
        if (deviceSlots.slot[i].file[0] == 0x00)
        {
            deviceSlots.slot[i].hostSlot = 0xFF;
        }
    }
    configDisk.mount(&atrConfig); // set up a special disk drive not on the bus
}

sioDisk *sioFuji::disk()
{
    return &configDisk;
}