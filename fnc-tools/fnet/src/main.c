/**
 * FujiNet Tools for CLI
 *
 * fnet - set Network SSID/Password
 *
 * usage:
 *  fnet <SSID> [PASSWORD]
 *
 * Author: Thomas Cherryhomes
 *  <thom.cherryhomes@gmail.com>
 *
 * Released under GPL, see COPYING
 * for details
 */

#include <atari.h>
#include <string.h>
#include <stdlib.h>
#include <peekpoke.h>
#include "sio.h"
#include "conio.h"
#include "err.h"

unsigned char buf[80];

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
 * Clear up to status bar for DOS 3
 */
void dos3_clear(void)
{
  print("\x1c\x1c\x1c\x1c\x1c\x1c\x1c\x1c\x1c\x1c");
  print("\xC3\xEF\xEE\xEE\xE5\xE3\xF4\xA0\xF4\xEF\xA0\xCE\xE5\xF4\xF7\xEF\xF2\xEB\x9b\x9b"); // Connect to Network
  print("\x9c\x9c\x9c\x9c\x9c\x9c\x9c\x9c\x9c\x9c");
}

/**
 * main
 */
int main(int argc, char* argv[])
{
  unsigned char wifiStatus=0;

  OS.soundr=0;
  OS.lmargn=2;

  if (_is_cmdline_dos())
    {
      if (argc<3)
	{
	  print(argv[0]);
	  print(" <SSID> <PASSWORD>\x9b");
	  exit(1);
	}
      
      strcpy(netConfig.ssid,argv[1]);
      strcpy(netConfig.password,argv[2]);
    }
  else
    {
      print("\x9b");
      if (PEEK(0x718)==53)
	dos3_clear();
      
      // Dos 2
      print("ENTER SSID: ");
      get_line(buf,sizeof(buf));
      strcpy(netConfig.ssid,buf);
      
      print("ENTER PASSWORD: ");
      get_line(buf,sizeof(buf));
      strcpy(netConfig.password,buf);      
    }
  
  print("Connecting to network: ");
  print(netConfig.ssid);
  print("...\x9b");
    
  OS.dcb.ddevic=0x70;
  OS.dcb.dunit=1;
  OS.dcb.dcomnd=0xFB; // Set SSID
  OS.dcb.dstats=0x80; // Computer->Peripheral
  OS.dcb.dbuf=&netConfig.rawData;
  OS.dcb.dtimlo=0x0f; // 15 second timeout
  OS.dcb.dbyt=sizeof(netConfig.rawData);
  OS.dcb.daux=0;
  siov();

  if (OS.dcb.dstats==1)
    {

      // Wait for connection
      OS.rtclok[0]=OS.rtclok[1]=OS.rtclok[2]=0;
      
      while (OS.rtclok[1]<1)
	{
	  OS.dcb.ddevic=0x70;
	  OS.dcb.dunit=1;
	  OS.dcb.dcomnd=0xFA; // Return wifi status
	  OS.dcb.dstats=0x40; // Peripheral->Computer
	  OS.dcb.dbuf=&wifiStatus;
	  OS.dcb.dtimlo=0x0F; // 15 second timeout
	  OS.dcb.dbyt=1;
	  OS.dcb.daux1=0;     
	  siov();
	  
	  if (wifiStatus==0x03) // IW_CONNECTED
	    {
	      print("OK!\x9b");
	      return 0;
	    }
	}
      
      print("Not Connected.\x9b");
    }
  else
    {
      err_sio();
    }

  OS.soundr=1;

  if (!_is_cmdline_dos())
    {
      print("\x9bPRESS \xA0\xD2\xC5\xD4\xD5\xD2\xCE\xA0 TO CONTINUE.\x9b");
      get_line(buf,sizeof(buf));
    }  
  
  return wifiStatus;  
}
