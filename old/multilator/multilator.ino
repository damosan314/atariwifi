/**
   Test #24 - multi-diskulator
*/

#include <ESP8266WiFi.h>
#include <FS.h>
#include <WiFiUdp.h>

enum {ID, COMMAND, AUX1, AUX2, CHECKSUM, ACK, NAK, PROCESS, WAIT} cmdState;

// Uncomment for Debug on 2nd UART (GPIO 2)
// #define DEBUG_S

// Uncomment for Debug on TCP/6502 to DEBUG_HOST
// Run:  `nc -vk -l 6502` on DEBUG_HOST
// #define DEBUG_N
// #define DEBUG_HOST "192.168.1.7"


#define PIN_LED         2
#define PIN_INT         5
#define PIN_PROC        4
#define PIN_MTR        16
#define PIN_CMD        12

#define DELAY_T5          1500
#define READ_CMD_TIMEOUT  12
#define CMD_TIMEOUT       50

#define STATUS_SKIP       8

WiFiUDP UDP;
File atr;

unsigned long cmdTimer = 0;

/**
   A Single command frame, both in structured and unstructured
   form.
*/
union
{
  struct
  {
    unsigned char devic;
    unsigned char comnd;
    unsigned char aux1;
    unsigned char aux2;
    unsigned char cksum;
  };
  byte cmdFrameData[5];
} cmdFrame;

/**
   A single SSID entry
*/
union
{
  struct
  {
    char ssid[32];
    char rssi;
  };
  unsigned char rawData[33];
} ssidInfo;

/**
   Network Configuration
*/
union
{
  struct
  {
    char ssid[32];
    char password[64];
  };
  unsigned char rawData[96];
} netConfig;

/**
   TNFS Packet
*/
union
{
  struct
  {
    byte session_idl;
    byte session_idh;
    byte retryCount;
    byte command;
    byte data[512];
  };
  byte rawData[516];
} tnfsPacket;

byte sectorCache[8][2560];

byte sector[128];
char tnfsServer[256];
char mountPath[256];
char current_entry[256];
char tnfs_fds[8];
char tnfs_dir_fds[8];
int firstCachedSector[8] = {65535,65535,65535,65535,65535,65535,65535,65535};
bool load_config=true;

#ifdef DEBUG_N
WiFiClient wificlient;
#endif

union
{
  char host[8][32];
  unsigned char rawData[256];
} hostSlots;

union
{
  struct
  {
  unsigned char hostSlot;
  char file[36];
  } slot[8];
  unsigned char rawData[296];
} deviceSlots;

struct 
{
  unsigned char session_idl;
  unsigned char session_idh;
} tnfsSessionIDs[8];

#ifdef DEBUG
#define Debug_print(...) Debug_print( __VA_ARGS__ )
#define Debug_printf(...) Debug_printf( __VA_ARGS__ )
#define Debug_println(...) Debug_println( __VA_ARGS__ )
#define DEBUG
#endif
#ifdef DEBUG_N
#define Debug_print(...) wificlient.print( __VA_ARGS__ )
#define Debug_printf(...) wificlient.printf( __VA_ARGS__ )
#define Debug_println(...) wificlient.println( __VA_ARGS__ )
#define DEBUG
#endif

/**
   calculate 8-bit checksum.
*/
byte sio_checksum(byte* chunk, int length)
{
  int chkSum = 0;
  for (int i = 0; i < length; i++) {
    chkSum = ((chkSum + chunk[i]) >> 8) + ((chkSum + chunk[i]) & 0xff);
  }
  return (byte)chkSum;
}

/**
   ISR for falling COMMAND
*/
void ICACHE_RAM_ATTR sio_isr_cmd()
{
  if (digitalRead(PIN_CMD) == LOW)
  {
    cmdState = ID;
    cmdTimer = millis();
  }
}

/**
 * Return true if valid device ID
 */
bool sio_valid_device_id()
{
  unsigned char deviceSlot=cmdFrame.devic-0x31;
  if ((load_config==true) && (cmdFrame.devic==0x31))
    return true;
  else if (cmdFrame.devic==0x70)
    return true;
  else if (cmdFrame.devic==0x4F)
    return false;
  else if (deviceSlots.slot[deviceSlot].hostSlot!=0xFF)
    return true;
  else
    return false;
}

/**
   Get ID
*/
void sio_get_id()
{
  cmdFrame.devic = Serial.read();
  if (sio_valid_device_id())
    cmdState = COMMAND;
  else
  {
    cmdState = WAIT;
    cmdTimer = 0;
  }

#ifdef DEBUG
  Debug_print("CMD DEVC: ");
  Debug_println(cmdFrame.devic, HEX);
#endif
}

void sio_get_command()
{
  cmdFrame.comnd = Serial.read();
  cmdState = AUX1;

#ifdef DEBUG
  Debug_print("CMD CMND: ");
  Debug_println(cmdFrame.comnd, HEX);
#endif
}

/**
   Get aux1
*/
void sio_get_aux1()
{
  cmdFrame.aux1 = Serial.read();
  cmdState = AUX2;

#ifdef DEBUG
  Debug_print("CMD AUX1: ");
  Debug_println(cmdFrame.aux1, HEX);
#endif
}

/**
   Get aux2
*/
void sio_get_aux2()
{
  cmdFrame.aux2 = Serial.read();
  cmdState = CHECKSUM;

#ifdef DEBUG
  Debug_print("CMD AUX2: ");
  Debug_println(cmdFrame.aux2, HEX);
#endif
}

/**
   Get Checksum, and compare
*/
void sio_get_checksum()
{
  byte ck;
  cmdFrame.cksum = Serial.read();
  ck = sio_checksum((byte *)&cmdFrame.cmdFrameData, 4);

#ifdef DEBUG
  Debug_print("CMD CKSM: ");
  Debug_print(cmdFrame.cksum, HEX);
#endif

  if (ck == cmdFrame.cksum)
  {
#ifdef DEBUG
    Debug_println(", ACK");
#endif
    sio_ack();
  }
  else
  {
#ifdef DEBUG
    Debug_println(", NAK");
#endif
    sio_nak();
  }
}

/**
   scan for networks
*/
void sio_scan_networks()
{
  byte ck;
  char totalSSIDs;
  char ret[4] = {0, 0, 0, 0};

  WiFi.mode(WIFI_STA);
  totalSSIDs = WiFi.scanNetworks();
  ret[0] = totalSSIDs;

#ifdef DEBUG
  Debug_printf("Scan networks returned: %d\n\n", totalSSIDs);
#endif
  ck = sio_checksum((byte *)&ret, 4);

  Serial.write('C');     // Completed command
  Serial.flush();

  delayMicroseconds(1500); // t5 delay

  // Write data frame
  Serial.write((byte *)&ret, 4);

  // Write data frame checksum
  Serial.write(ck);
  Serial.flush();

#ifdef DEBUG
  Debug_printf("Wrote data packet/Checksum: $%02x $%02x $%02x $%02x/$02x\n\n", ret[0], ret[1], ret[2], ret[3], ck);
#endif
  delayMicroseconds(200);
}

/**
   Return scanned network entry
*/
void sio_scan_result()
{
  byte ck;

  strcpy(ssidInfo.ssid, WiFi.SSID(cmdFrame.aux1).c_str());
  ssidInfo.rssi = (char)WiFi.RSSI(cmdFrame.aux1);

  ck = sio_checksum((byte *)&ssidInfo.rawData, 33);

  delayMicroseconds(1500); // t5 delay
  Serial.write('C');     // Completed command
  Serial.flush();

  // Write data frame
  Serial.write(ssidInfo.rawData, 33);

  // Write data frame checksum
  Serial.write(ck);
  Serial.flush();
  delayMicroseconds(200);
}

/**
   SIO set SSID/Password
*/
void sio_set_ssid()
{
  byte ck;

  Serial.readBytes(netConfig.rawData, 96);
  while (Serial.available()==0) { delayMicroseconds(200); }
  ck = Serial.read(); // Read checksum
  Serial.write('A'); // Write ACK

  if (ck == sio_checksum(netConfig.rawData, 96))
  {
    Serial.write('C');
    WiFi.begin(netConfig.ssid, netConfig.password);
    yield();
  }
  else
  {
    Serial.write('E');
    yield();
  }
}

/**
   SIO get WiFi Status
*/
void sio_get_wifi_status()
{
  byte ck;
  char wifiStatus;

  wifiStatus = WiFi.status();

  ck = sio_checksum((byte *)&wifiStatus, 1);

  delayMicroseconds(1500); // t5 delay
  Serial.write('C');     // Completed command
  Serial.flush();

  // Write data frame
  Serial.write(wifiStatus);

  // Write data frame checksum
  Serial.write(ck);
  Serial.flush();
  delayMicroseconds(200);
}

/**
   Write, called for both W and P commands.
*/
void sio_write()
{
  byte ck;
  int offset = (256 * cmdFrame.aux2) + cmdFrame.aux1;
  unsigned char deviceSlot=cmdFrame.devic-0x31;
  
  offset *= 128;
  offset -= 128;
  offset += 16; // skip 16 byte ATR Header

#ifdef DEBUG_S
  Serial1.printf("receiving 128b data frame from computer.\n");
#endif

  Serial.readBytes(sector, 128);
  while (Serial.available()==0) { delayMicroseconds(200); }
  ck = Serial.read(); // Read checksum
  //delayMicroseconds(350);
  Serial.write('A'); // Write ACK

  if (ck == sio_checksum(sector, 128))
  {
    if (load_config==true)
    {
      atr.seek(offset, SeekSet);
      atr.write(sector, 128);
      atr.flush();
    }
    else
    {
      tnfs_seek(deviceSlot,offset);
      tnfs_write(deviceSlot);
      firstCachedSector[cmdFrame.devic-0x31]=65535; // invalidate cache
    }
    Serial.write('C');
    yield();
  }
  else
  {
    Serial.write('E');
    yield();  
  }
}

/**
   format (fake)
*/
void sio_format()
{
  byte ck;

  for (int i = 0; i < 128; i++)
    sector[i] = 0;

  sector[0] = 0xFF; // no bad sectors.
  sector[1] = 0xFF;

  ck = sio_checksum((byte *)&sector, 128);

  delayMicroseconds(DELAY_T5); // t5 delay
  Serial.write('C'); // Completed command
  Serial.flush();

  // Write data frame
  Serial.write(sector, 128);

  // Write data frame checksum
  Serial.write(ck);
  Serial.flush();
  delayMicroseconds(200);
#ifdef DEBUG_S
  Serial1.printf("We faked a format.\n");
#endif
}

/**
   SIO TNFS server mount
*/
void sio_mount_host()
{
  byte ck;
  unsigned char hostSlot=cmdFrame.aux1;
  
  Serial.write('A'); // Write ACK

#ifdef DEBUG
  Debug_printf("Mounting host in slot #%d",hostSlot);
#endif

  tnfs_mount(hostSlot);

  Serial.write('C');
  Serial.flush();
}

/**
   SIO Mount
*/
void sio_mount_image()
{
  byte ck;
  unsigned char deviceSlot=cmdFrame.aux1;

#ifdef DEBUG
  Debug_printf("Opening image in drive slot #%d",deviceSlot);
#endif

  tnfs_open(deviceSlot);
  Serial.write('C');
}

/**
   SIO Open TNFS Directory
*/
void sio_open_tnfs_directory()
{
  byte ck;
  unsigned char hostSlot=cmdFrame.aux1;
  
#ifdef DEBUG
  Debug_println("Receiving 256b frame from computer");
#endif

  Serial.readBytes(current_entry, 256);
  while (Serial.available()==0) { delayMicroseconds(200); }
  ck = Serial.read(); // Read checksum

  if (ck != sio_checksum((byte *)&current_entry, 256))
  {
    Serial.write('N'); // NAK
    return;
  }

  Serial.write('A');   // ACK

  tnfs_opendir(hostSlot);

  // And complete.
  Serial.write('C');
}

/**
   Read TNFS directory (next entry)
*/
void sio_read_tnfs_directory()
{
  byte ck;
  long offset;
  byte ret;

  memset(current_entry, 0x00, 256);

  ret = tnfs_readdir(cmdFrame.aux2);

  if (ret == false)
  {
    current_entry[0] = 0x7F; // end of dir
  }

  ck = sio_checksum((byte *)&current_entry, cmdFrame.aux1);

  delayMicroseconds(DELAY_T5); // t5 delay

  Serial.write('C'); // Command always completes.
  Serial.flush();

  delayMicroseconds(200);

  // Write data frame
  Serial.write((byte *)current_entry, cmdFrame.aux1);

  // Write checksum
  Serial.write(ck);
  Serial.flush();
  delayMicroseconds(200);
}

/**
   SIO close TNFS Directory
*/
void sio_close_tnfs_directory()
{
  delayMicroseconds(DELAY_T5);
  tnfs_closedir(cmdFrame.aux1);
  Serial.write('C'); // Completed command
  Serial.flush();
}

/**
   Process command
*/

void sio_process()
{
  switch (cmdFrame.comnd)
  {
    case 'P':
    case 'W':
      sio_write();
      break;
    case 'R':
      sio_read();
      break;
    case 'S':
      sio_status();
      break;
    case '!':
      sio_format();
      break;
    case 0xFD:
      sio_scan_networks();
      break;
    case 0xFC:
      sio_scan_result();
      break;
    case 0xFB:
      sio_set_ssid();
      break;
    case 0xFA:
      sio_get_wifi_status();
      break;
    case 0xF9:
      sio_mount_host();
      break;
    case 0xF8:
      sio_mount_image();
      break;
    case 0xF7:
      sio_open_tnfs_directory();
      break;
    case 0xF6:
      sio_read_tnfs_directory();
      break;
    case 0xF5:
      sio_close_tnfs_directory();
      break;
    case 0xF4:
      sio_read_hosts_slots();
      break;
    case 0xF3:
      sio_write_hosts_slots();
      break;
    case 0xF2:
      sio_read_drives_slots();
      break;
    case 0xF1:
      sio_write_drives_slots();
      break;
  }

  cmdState = WAIT;
  cmdTimer = 0;
}

/**
 * Write hosts slots
 */
void sio_write_hosts_slots()
{
  byte ck;

  Serial.readBytes(hostSlots.rawData, 256);
  while (Serial.available()==0) { delayMicroseconds(200); }
  ck = Serial.read(); // Read checksum
  Serial.write('A'); // Write ACK

  if (ck == sio_checksum(hostSlots.rawData, 256))
  {
    Serial.write('C');
    atr.seek(91792, SeekSet);
    atr.write(hostSlots.rawData,256);
    atr.flush();
#ifdef DEBUG
    for (int i=0;i<sizeof(hostSlots.rawData);i++)
    {
      Debug_printf("%c",hostSlots.rawData[i]);  
    }
    Debug_printf("\n\nCOMPLETE\n");
#endif
    yield();
  }
  else
  {
    Serial.write('E');
#ifdef DEBUG
    for (int i=0;i<sizeof(hostSlots.rawData);i++)
    {
      Debug_printf("%c",hostSlots.rawData[i]);  
    }
    Debug_printf("\n\nChecksum: calc: %02x recv: %02x - ERROR\n",sio_checksum(hostSlots.rawData,256),ck);
#endif
    yield();
  }
}

/**
 * Write drives slots
 */
void sio_write_drives_slots()
{
  byte ck;

  Serial.readBytes(deviceSlots.rawData, 296);
  while (Serial.available()==0) { delayMicroseconds(200); }
  ck = Serial.read(); // Read checksum
  Serial.write('A'); // Write ACK

  if (ck == sio_checksum(hostSlots.rawData, 296))
  {
    Serial.write('C');
    atr.seek(91408, SeekSet);
    atr.write(hostSlots.rawData,296);
    atr.flush();
#ifdef DEBUG
    for (int i=0;i<sizeof(hostSlots.rawData);i++)
    {
      Debug_printf("%c",hostSlots.rawData[i]);  
    }
    Debug_printf("\n\nCOMPLETE\n");
#endif
    yield();
  }
  else
  {
    Serial.write('E');
#ifdef DEBUG
    for (int i=0;i<sizeof(hostSlots.rawData);i++)
    {
      Debug_printf("%c",hostSlots.rawData[i]);  
    }
    Debug_printf("\n\nChecksum: calc: %02x recv: %02x - ERROR\n",sio_checksum(hostSlots.rawData,296),ck);
#endif
    yield();
  }
}

/**
 * Read hosts slots
 */
void sio_read_hosts_slots()
{
  byte ck;

  ck = sio_checksum((byte *)&hostSlots.rawData, 256);

  delayMicroseconds(DELAY_T5); // t5 delay
  Serial.write('C'); // Command always completes.
  Serial.flush();
  delayMicroseconds(200);

  // Write data frame
  for (int i = 0; i < 256; i++)
    Serial.write(hostSlots.rawData[i]);

  // Write checksum
  Serial.write(ck);
  Serial.flush();
  delayMicroseconds(200);
  
}

/**
 * Read hosts slots
 */
void sio_read_drives_slots()
{
  byte ck;
  
  load_config=false;
  ck = sio_checksum((byte *)&deviceSlots.rawData, 296);

  delayMicroseconds(DELAY_T5); // t5 delay
  Serial.write('C'); // Command always completes.
  Serial.flush();
  delayMicroseconds(200);

  // Write data frame
  for (int i = 0; i < 296; i++)
    Serial.write(deviceSlots.rawData[i]);

  // Write checksum
  Serial.write(ck);
  Serial.flush();
  delayMicroseconds(200);
}

/**
   Read
*/
void sio_read()
{
  byte ck;
  unsigned char deviceSlot=cmdFrame.devic-0x31;
  int sectorNum = (256 * cmdFrame.aux2) + cmdFrame.aux1;
  int cacheOffset = 0;
  int offset;

  if (load_config==true) // no TNFS ATR mounted.
  {
    offset = sectorNum;
    offset *= 128;
    offset -= 128;
    offset += 16;
    atr.seek(offset, SeekSet);
    atr.read(sector, 128);
  }
  else // TNFS ATR mounted and opened...
  {
    if ((sectorNum > (firstCachedSector[deviceSlot] + 19)) || (sectorNum < firstCachedSector[deviceSlot])) // cache miss
    {
      firstCachedSector[deviceSlot] = sectorNum;
      cacheOffset = 0;
      offset = sectorNum;
      offset *= 128;
      offset -= 128;
      offset += 16;
#ifdef DEBUG
      Debug_printf("firstCachedSector: %d\n", firstCachedSector);
      Debug_printf("cacheOffset: %d\n", cacheOffset);
      Debug_printf("offset: %d\n", offset);
#endif
      tnfs_seek(deviceSlot, offset);
      tnfs_read(deviceSlot);
      yield();
      for (int i = 0; i < 256; i++)
        sectorCache[deviceSlot][cacheOffset + i] = tnfsPacket.data[i + 3];
      cacheOffset += 256;
      tnfs_read(deviceSlot);
      yield();
      for (int i = 0; i < 256; i++)
        sectorCache[deviceSlot][cacheOffset + i] = tnfsPacket.data[i + 3];
      cacheOffset += 256;
      tnfs_read(deviceSlot);
      yield();
      for (int i = 0; i < 256; i++)
        sectorCache[deviceSlot][cacheOffset + i] = tnfsPacket.data[i + 3];
      cacheOffset += 256;
      tnfs_read(deviceSlot);
      yield();
      for (int i = 0; i < 256; i++)
        sectorCache[deviceSlot][cacheOffset + i] = tnfsPacket.data[i + 3];
      cacheOffset += 256;
      tnfs_read(deviceSlot);
      yield();
      for (int i = 0; i < 256; i++)
        sectorCache[deviceSlot][cacheOffset + i] = tnfsPacket.data[i + 3];
      cacheOffset += 256;
      tnfs_read(deviceSlot);
      yield();
      for (int i = 0; i < 256; i++)
        sectorCache[deviceSlot][cacheOffset + i] = tnfsPacket.data[i + 3];
      cacheOffset += 256;
      tnfs_read(deviceSlot);
      yield();
      for (int i = 0; i < 256; i++)
        sectorCache[deviceSlot][cacheOffset + i] = tnfsPacket.data[i + 3];
      cacheOffset += 256;
      tnfs_read(deviceSlot);
      yield();
      for (int i = 0; i < 256; i++)
        sectorCache[deviceSlot][cacheOffset + i] = tnfsPacket.data[i + 3];
      cacheOffset += 256;
      tnfs_read(deviceSlot);
      yield();
      for (int i = 0; i < 256; i++)
        sectorCache[deviceSlot][cacheOffset + i] = tnfsPacket.data[i + 3];
      cacheOffset += 256;
      tnfs_read(deviceSlot);
      yield();
      for (int i = 0; i < 256; i++)
        sectorCache[deviceSlot][cacheOffset + i] = tnfsPacket.data[i + 3];
      cacheOffset = 0;
    }
    else // cache hit, adjust offset
    {
      cacheOffset = ((sectorNum - firstCachedSector[deviceSlot]) * 128);
#ifdef DEBUG
      Debug_printf("cacheOffset: %d\n", cacheOffset);
#endif
    }
    for (int i = 0; i < 128; i++)
      sector[i] = sectorCache[deviceSlot][(i + cacheOffset)];
  }

  ck = sio_checksum((byte *)&sector, 128);

  Serial.write('C'); // Completed command
  Serial.flush();

  // Write data frame
  Serial.write(sector, 128);
  Serial.flush();

  // Write data frame checksum
  delayMicroseconds(200);  
  Serial.write(ck);
  Serial.flush();
#ifdef DEBUG
  Debug_print("SIO READ OFFSET: ");
  Debug_print(offset);
  Debug_print(" - ");
  Debug_println((offset + 128));
#endif
}

/**
   Status
*/
void sio_status()
{
  byte status[4] = {0x00, 0xFF, 0xFE, 0x00};
  byte ck;

  ck = sio_checksum((byte *)&status, 4);

  delayMicroseconds(DELAY_T5); // t5 delay
  Serial.write('C'); // Command always completes.
  Serial.flush();
  delayMicroseconds(200);
  //delay(1);

  // Write data frame
  for (int i = 0; i < 4; i++)
    Serial.write(status[i]);

  // Write checksum
  Serial.write(ck);
  Serial.flush();
  delayMicroseconds(200);
}

/**
   Send an acknowledgement
*/
void sio_ack()
{
  delayMicroseconds(500);
  Serial.write('A');
  Serial.flush();
  //cmdState = PROCESS;
  sio_process();
}

/**
   Send a non-acknowledgement
*/
void sio_nak()
{
  delayMicroseconds(500);
  Serial.write('N');
  Serial.flush();
  cmdState = WAIT;
  cmdTimer = 0;
}

void sio_incoming() {
  switch (cmdState)
  {
    case ID:
      sio_get_id();
      break;
    case COMMAND:
      sio_get_command();
      break;
    case AUX1:
      sio_get_aux1();
      break;
    case AUX2:
      sio_get_aux2();
      break;
    case CHECKSUM:
      sio_get_checksum();
      break;
    case ACK:
      sio_ack();
      break;
    case NAK:
      sio_nak();
      break;
    case PROCESS:
      sio_process();
      break;
    case WAIT:
      Serial.read(); // Toss it for now
      cmdTimer = 0;
      break;
  }
}

/**
   Mount the TNFS server
*/
void tnfs_mount(unsigned char hostSlot)
{
  int start = millis();
  int dur = millis() - start;
  
  memset(tnfsPacket.rawData, 0, sizeof(tnfsPacket.rawData));

  // Do not mount, if we already have a session ID, just bail.
  if (tnfsSessionIDs[hostSlot].session_idl!=0 && tnfsSessionIDs[hostSlot].session_idh!=0)
    return;
    
  tnfsPacket.session_idl = 0;
  tnfsPacket.session_idh = 0;
  tnfsPacket.retryCount = 0;
  tnfsPacket.command = 0;
  tnfsPacket.data[0] = 0x01; // vers
  tnfsPacket.data[1] = 0x00; // "  "
  tnfsPacket.data[2] = 0x2F; // /
  tnfsPacket.data[3] = 0x00; // nul
  tnfsPacket.data[4] = 0x00; // no username
  tnfsPacket.data[5] = 0x00; // no password

#ifdef DEBUG
  Debug_print("Mounting / from ");
  Debug_println((char*)hostSlots.host[hostSlot]);
  for (int i=0;i<32;i++)
    Debug_printf("%02x ",hostSlots.host[hostSlot][i]);
  Debug_printf("\n\n");
  Debug_print("Req Packet: ");
  for (int i = 0; i < 10; i++)
  {
    Debug_print(tnfsPacket.rawData[i], HEX);
    Debug_print(" ");
  }
  Debug_println("");
#endif /* DEBUG_S */

  UDP.beginPacket(String(hostSlots.host[hostSlot]).c_str(), 16384);
  UDP.write(tnfsPacket.rawData, 10);
  UDP.endPacket();

#ifdef DEBUG
  Debug_println("Wrote the packet");
#endif

  while (dur < 5000)
  {
    yield();
    if (UDP.parsePacket())
    {
      int l = UDP.read(tnfsPacket.rawData, 516);
#ifdef DEBUG
      Debug_print("Resp Packet: ");
      for (int i = 0; i < l; i++)
      {
        Debug_print(tnfsPacket.rawData[i], HEX);
        Debug_print(" ");
      }
      Debug_println("");
#endif /* DEBUG_S */
      if (tnfsPacket.data[0] == 0x00)
      {
        // Successful
#ifdef DEBUG
        Debug_print("Successful, Session ID: ");
        Debug_print(tnfsPacket.session_idl, HEX);
        Debug_println(tnfsPacket.session_idh, HEX);
#endif /* DEBUG_S */
        // Persist the session ID.
        tnfsSessionIDs[hostSlot].session_idl=tnfsPacket.session_idl;
        tnfsSessionIDs[hostSlot].session_idh=tnfsPacket.session_idh;
        return;
      }
      else
      {
        // Error
#ifdef DEBUG
        Debug_print("Error #");
        Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S */
        return;
      }
    }
  }
  // Otherwise we timed out.
#ifdef DEBUG
  Debug_println("Timeout after 5000ms");
#endif /* DEBUG_S */
}

/**
   Open 'autorun.atr'
*/
void tnfs_open(unsigned char deviceSlot)
{
  int start = millis();
  int dur = millis() - start;
  int c = 0;
  strcpy(mountPath,deviceSlots.slot[deviceSlot].file);
  tnfsPacket.session_idl=tnfsSessionIDs[deviceSlots.slot[deviceSlot].hostSlot].session_idl;
  tnfsPacket.session_idh=tnfsSessionIDs[deviceSlots.slot[deviceSlot].hostSlot].session_idh;
  tnfsPacket.retryCount++;  // increase sequence #
  tnfsPacket.command = 0x29; // OPEN
  tnfsPacket.data[c++] = 0x03; // R/W
  tnfsPacket.data[c++] = 0x00; //
  tnfsPacket.data[c++] = 0x00; // Flags
  tnfsPacket.data[c++] = 0x00; //
  tnfsPacket.data[c++] = '/'; // Filename start

  for (int i = 0; i < strlen(mountPath); i++)
  {
    tnfsPacket.data[i + 5] = mountPath[i];
    c++;
  }

  tnfsPacket.data[c++] = 0x00;
  tnfsPacket.data[c++] = 0x00;
  tnfsPacket.data[c++] = 0x00;

#ifdef DEBUG
  Debug_printf("Opening /%s\n", mountPath);
  Debug_println("");
  Debug_print("Req Packet: ");
  for (int i = 0; i < c + 4; i++)
  {
    Debug_print(tnfsPacket.rawData[i], HEX);
    Debug_print(" ");
  }
#endif /* DEBUG_S */

  UDP.beginPacket(hostSlots.host[deviceSlots.slot[deviceSlot].hostSlot], 16384);
  UDP.write(tnfsPacket.rawData, c + 4);
  UDP.endPacket();

  while (dur < 5000)
  {
    yield();
    if (UDP.parsePacket())
    {
      int l = UDP.read(tnfsPacket.rawData, 516);
#ifdef DEBUG
      Debug_print("Resp packet: ");
      for (int i = 0; i < l; i++)
      {
        Debug_print(tnfsPacket.rawData[i], HEX);
        Debug_print(" ");
      }
      Debug_println("");
#endif DEBUG_S
      if (tnfsPacket.data[0] == 0x00)
      {
        // Successful
        tnfs_fds[deviceSlot] = tnfsPacket.data[1];
#ifdef DEBUG
        Debug_print("Successful, file descriptor: #");
        Debug_println(tnfs_fds[deviceSlot], HEX);
#endif /* DEBUG_S */
        return;
      }
      else
      {
        // unsuccessful
#ifdef DEBUG
        Debug_print("Error code #");
        Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S*/
        return;
      }
    }
  }
  // Otherwise, we timed out.
#ifdef DEBUG
  Debug_println("Timeout after 5000ms.");
#endif /* DEBUG_S */
}

/**
   TNFS Open Directory
*/
void tnfs_opendir(unsigned char hostSlot)
{
  int start = millis();
  int dur = millis() - start;
  tnfsPacket.session_idl=tnfsSessionIDs[hostSlot].session_idl;
  tnfsPacket.session_idh=tnfsSessionIDs[hostSlot].session_idh;
  tnfsPacket.retryCount++;  // increase sequence #
  tnfsPacket.command = 0x10; // OPENDIR
  tnfsPacket.data[0] = '/'; // Open root dir
  tnfsPacket.data[1] = 0x00; // nul terminated

#ifdef DEBUG
  Debug_println("TNFS Open directory /");
#endif

  UDP.beginPacket(String(hostSlots.host[hostSlot]).c_str(), 16384);
  UDP.write(tnfsPacket.rawData, 2 + 4);
  UDP.endPacket();

  while (dur < 5000)
  {
    yield();
    if (UDP.parsePacket())
    {
      int l = UDP.read(tnfsPacket.rawData, 516);
      if (tnfsPacket.data[0] == 0x00)
      {
        // Successful
        tnfs_dir_fds[hostSlot] = tnfsPacket.data[1];
#ifdef DEBUG
        Debug_printf("Opened dir on slot #%d - fd = %02x\n",hostSlot,tnfs_dir_fds[hostSlot]);
#endif
        return;
      }
      else
      {
        // Unsuccessful
      }
    }
  }
  // Otherwise, we timed out.
#ifdef DEBUG
  Debug_println("Timeout after 5000ms.");
#endif
}

/**
   TNFS Read Directory
   Reads the next directory entry
*/
bool tnfs_readdir(unsigned char hostSlot)
{
  int start = millis();
  int dur = millis() - start;
  tnfsPacket.session_idl=tnfsSessionIDs[hostSlot].session_idl;
  tnfsPacket.session_idh=tnfsSessionIDs[hostSlot].session_idh;
  tnfsPacket.retryCount++;  // increase sequence #
  tnfsPacket.command = 0x11; // READDIR
  tnfsPacket.data[0] = tnfs_dir_fds[hostSlot]; // Open root dir

#ifdef DEBUG
  Debug_printf("TNFS Read next dir entry, slot #%d - fd %02x\n\n",hostSlot,tnfs_dir_fds[hostSlot]);
#endif

  UDP.beginPacket(String(hostSlots.host[hostSlot]).c_str(), 16384);
  UDP.write(tnfsPacket.rawData, 1 + 4);
  UDP.endPacket();

  while (dur < 5000)
  {
    yield();
    if (UDP.parsePacket())
    {
      int l = UDP.read(tnfsPacket.rawData, 516);
      if (tnfsPacket.data[0] == 0x00)
      {
        // Successful
        strcpy((char*)&current_entry, (char *)&tnfsPacket.data[1]);
        return true;
      }
      else
      {
        // Unsuccessful
        return false;
      }
    }
  }
  // Otherwise, we timed out.
#ifdef DEBUG
  Debug_println("Timeout after 5000ms.");
#endif /* DEBUG_S */
}

/**
   TNFS Close Directory
*/
void tnfs_closedir(unsigned char hostSlot)
{
  int start = millis();
  int dur = millis() - start;
  tnfsPacket.session_idl=tnfsSessionIDs[hostSlot].session_idl;
  tnfsPacket.session_idh=tnfsSessionIDs[hostSlot].session_idh;
  tnfsPacket.retryCount++;  // increase sequence #
  tnfsPacket.command = 0x12; // CLOSEDIR
  tnfsPacket.data[0] = tnfs_dir_fds[hostSlot]; // Open root dir

#ifdef DEBUG
  Debug_println("TNFS dir close");
#endif

  UDP.beginPacket(hostSlots.host[hostSlot], 16384);
  UDP.write(tnfsPacket.rawData, 1 + 4);
  UDP.endPacket();

  while (dur < 5000)
  {
    yield();
    if (UDP.parsePacket())
    {
      int l = UDP.read(tnfsPacket.rawData, 516);
      if (tnfsPacket.data[0] == 0x00)
      {
        // Successful
        return;
      }
      else
      {
        // Unsuccessful
        return;
      }
    }
  }
  // Otherwise, we timed out.
#ifdef DEBUG
  Debug_println("Timeout after 5000ms.");
#endif /* DEBUG_S */
}

/**
   TNFS write
*/
void tnfs_write(unsigned char deviceSlot)
{
  int start = millis();
  int dur = millis() - start;
  tnfsPacket.session_idl=tnfsSessionIDs[deviceSlots.slot[deviceSlot].hostSlot].session_idl;
  tnfsPacket.session_idh=tnfsSessionIDs[deviceSlots.slot[deviceSlot].hostSlot].session_idh;  
  tnfsPacket.retryCount++;  // Increase sequence
  tnfsPacket.command = 0x22; // READ
  tnfsPacket.data[0] = tnfs_fds[deviceSlot]; // returned file descriptor
  tnfsPacket.data[1] = 0x80; // 128 bytes
  tnfsPacket.data[2] = 0x00; //

#ifdef DEBUG
  Debug_print("Writing to File descriptor: ");
  Debug_println(tnfs_fds[deviceSlot]);
  Debug_print("Req Packet: ");
  for (int i = 0; i < 7; i++)
  {
    Debug_print(tnfsPacket.rawData[i], HEX);
    Debug_print(" ");
  }
  Debug_println("");
#endif /* DEBUG_S */

  UDP.beginPacket(hostSlots.host[deviceSlots.slot[deviceSlot].hostSlot], 16384);
  UDP.write(tnfsPacket.rawData, 4 + 3);
  UDP.write(sector, 128);
  UDP.endPacket();

  while (dur < 5000)
  {
    yield();
    if (UDP.parsePacket())
    {
      int l = UDP.read(tnfsPacket.rawData, sizeof(tnfsPacket.rawData));
#ifdef DEBUG
      Debug_print("Resp packet: ");
      for (int i = 0; i < l; i++)
      {
        Debug_print(tnfsPacket.rawData[i], HEX);
        Debug_print(" ");
      }
      Debug_println("");
#endif /* DEBUG_S */
      if (tnfsPacket.data[0] == 0x00)
      {
        // Successful
#ifdef DEBUG
        Debug_println("Successful.");
#endif /* DEBUG_S */
        return;
      }
      else
      {
        // Error
#ifdef DEBUG
        Debug_print("Error code #");
        Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S*/
        return;
      }
    }
  }
#ifdef DEBUG
  Debug_println("Timeout after 5000ms.");
#endif /* DEBUG_S */
}

/**
   TNFS read
*/
void tnfs_read(unsigned char deviceSlot)
{
  int start = millis();
  int dur = millis() - start;
  tnfsPacket.session_idl=tnfsSessionIDs[deviceSlots.slot[deviceSlot].hostSlot].session_idl;
  tnfsPacket.session_idh=tnfsSessionIDs[deviceSlots.slot[deviceSlot].hostSlot].session_idh;
  tnfsPacket.retryCount++;  // Increase sequence
  tnfsPacket.command = 0x21; // READ
  tnfsPacket.data[0] = tnfs_fds[deviceSlot]; // returned file descriptor
  tnfsPacket.data[1] = 0x00; // 256 bytes
  tnfsPacket.data[2] = 0x01; //

#ifdef DEBUG
  Debug_print("Reading from File descriptor: ");
  Debug_println(tnfs_fds[deviceSlot]);
  Debug_print("Req Packet: ");
  for (int i = 0; i < 7; i++)
  {
    Debug_print(tnfsPacket.rawData[i], HEX);
    Debug_print(" ");
  }
  Debug_println("");
#endif /* DEBUG_S */

  UDP.beginPacket(hostSlots.host[deviceSlots.slot[deviceSlot].hostSlot], 16384);
  UDP.write(tnfsPacket.rawData, 4 + 3);
  UDP.endPacket();

  while (dur < 5000)
  {
    yield();
    if (UDP.parsePacket())
    {
      int l = UDP.read(tnfsPacket.rawData, sizeof(tnfsPacket.rawData));
#ifdef DEBUG
      Debug_print("Resp packet: ");
      for (int i = 0; i < l; i++)
      {
        Debug_print(tnfsPacket.rawData[i], HEX);
        Debug_print(" ");
      }
      Debug_println("");
#endif /* DEBUG_S */
      if (tnfsPacket.data[0] == 0x00)
      {
        // Successful
#ifdef DEBUG
        Debug_println("Successful.");
#endif /* DEBUG_S */
        return;
      }
      else
      {
        // Error
#ifdef DEBUG
        Debug_print("Error code #");
        Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S*/
        return;
      }
    }
  }
#ifdef DEBUG
  Debug_println("Timeout after 5000ms.");
#endif /* DEBUG_S */
}

/**
   TNFS seek
*/
void tnfs_seek(unsigned char deviceSlot, long offset)
{
  int start = millis();
  int dur = millis() - start;
  byte offsetVal[4];

  offsetVal[0] = (int)((offset & 0xFF000000) >> 24 );
  offsetVal[1] = (int)((offset & 0x00FF0000) >> 16 );
  offsetVal[2] = (int)((offset & 0x0000FF00) >> 8 );
  offsetVal[3] = (int)((offset & 0X000000FF));

  tnfsPacket.retryCount++;
  tnfsPacket.session_idl=tnfsSessionIDs[deviceSlots.slot[deviceSlot].hostSlot].session_idl;
  tnfsPacket.session_idh=tnfsSessionIDs[deviceSlots.slot[deviceSlot].hostSlot].session_idh;
  tnfsPacket.command = 0x25; // LSEEK
  tnfsPacket.data[0] = tnfs_fds[deviceSlot];
  tnfsPacket.data[1] = 0x00; // SEEK_SET
  tnfsPacket.data[2] = offsetVal[3];
  tnfsPacket.data[3] = offsetVal[2];
  tnfsPacket.data[4] = offsetVal[1];
  tnfsPacket.data[5] = offsetVal[0];

#ifdef DEBUG
  Debug_print("Seek requested to offset: ");
  Debug_println(offset);
  Debug_print("Req packet: ");
  for (int i = 0; i < 10; i++)
  {
    Debug_print(tnfsPacket.rawData[i], HEX);
    Debug_print(" ");
  }
  Debug_println("");
#endif /* DEBUG_S*/

  UDP.beginPacket(hostSlots.host[deviceSlots.slot[deviceSlot].hostSlot], 16384);
  UDP.write(tnfsPacket.rawData, 6 + 4);
  UDP.endPacket();

  while (dur < 5000)
  {
    yield();
    if (UDP.parsePacket())
    {
      int l = UDP.read(tnfsPacket.rawData, sizeof(tnfsPacket.rawData));
#ifdef DEBUG
      Debug_print("Resp packet: ");
      for (int i = 0; i < l; i++)
      {
        Debug_print(tnfsPacket.rawData[i], HEX);
        Debug_print(" ");
      }
      Debug_println("");
#endif /* DEBUG_S */

      if (tnfsPacket.data[0] == 0)
      {
        // Success.
#ifdef DEBUG
        Debug_println("Successful.");
#endif /* DEBUG_S */
        return;
      }
      else
      {
        // Error.
#ifdef DEBUG
        Debug_print("Error code #");
        Debug_println(tnfsPacket.data[0], HEX);
#endif /* DEBUG_S*/
        return;
      }
    }
  }
#ifdef DEBUG
  Debug_println("Timeout after 5000ms.");
#endif /* DEBUG_S */
}

void setup()
{
  UDP.begin(16384);
  SPIFFS.begin();
  atr = SPIFFS.open("/autorun.atr", "r+");

  // Go ahead and read the host slots from disk
  atr.seek(91792,SeekSet);
  atr.read(hostSlots.rawData,256);

  // And populate the device slots
  atr.seek(91408, SeekSet);
  atr.read(deviceSlots.rawData,296);

  // Go ahead and mark all device slots local
  for (int i=0;i<8;i++)
  {
    if (deviceSlots.slot[i].file[0]==0x00)
    {
      deviceSlots.slot[i].hostSlot=0xFF;  
    }
  }
  
  // Set up pins
#ifdef DEBUG_S
  Serial1.begin(19200);
  Debug_println();
  Debug_println("#FujiNet Multilator");
#else
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, HIGH);
#endif
  pinMode(PIN_INT, OUTPUT); // thanks AtariGeezer.
  pinMode(PIN_PROC, OUTPUT);
  pinMode(PIN_MTR, INPUT);
  pinMode(PIN_CMD, INPUT);

#ifdef DEBUG_N
  wificlient.connect(DEBUG_HOST, 6502);
  wificlient.println("#FujiNet Caching Test");
#endif

  // Set up serial
  Serial.begin(19200);
  Serial.swap();

  // Attach COMMAND interrupt.
  attachInterrupt(digitalPinToInterrupt(PIN_CMD), sio_isr_cmd, FALLING);
  cmdState = WAIT; // Start in wait state
}

void loop()
{
#ifdef DEBUG_N
  /* Connect to debug server if we aren't and WiFi is connected */
  if ( !wificlient.connected() && WiFi.status() == WL_CONNECTED )
  {
    wificlient.connect(DEBUG_HOST, 6502);
    wificlient.println("#FujiNet Multi-lator Test");
  }
#endif
  
  if (Serial.available() > 0)
  {
    sio_incoming();
  }

  if (millis() - cmdTimer > CMD_TIMEOUT && cmdState != WAIT)
  {
#ifdef DEBUG
    Debug_print("SIO CMD TIMEOUT: ");
    Debug_println(cmdState);
#endif
    cmdState = WAIT;
    cmdTimer = 0;
  }
}
