/**
 * Config for SpartaDOS/DOS XL
 *
 * Author: Thom Cherryhomes
 *  <thom.cherryhomes@gmail.com>
 *
 * Licensed under GPL v. 3
 * See COPYING for details
 */

#include <atari.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"
#include "sio.h"
#include "error.h"
#include "host.h"
#include "disk.h"

extern unsigned char path[256];

extern union
{
  char host[8][32];
  unsigned char rawData[256];
} hostSlots;

extern union
{
  struct
  {
    unsigned char hostSlot;
    unsigned char mode;
    char file[36];
  } slot[8];
  unsigned char rawData[304];
} deviceSlots;

/**
 * List host slots
 */
unsigned char list_host_slots(void)
{
  unsigned char i;

  host_read();
  
  if (OS.dcb.dstats==1)
    {
      for (i=0;i<8;i++)
	{
	  if (hostSlots.host[i][0]==0x00)
	    printf("%d: Empty\n",i+1);
	  else
	    printf("%d: %s\n",i+1,hostSlots.host[i]);
	}
    }
  else
    {
      sio_error();
      return 1;
    }

  return 0;
}

/**
 * List device slots
 */
unsigned char list_device_slots(void)
{
  unsigned char i;

  disk_read();
  
  if (OS.dcb.dstats==1)
    {
      for (i=0;i<8;i++)
	{
	  if (deviceSlots.slot[i].hostSlot==0xFF)
	    printf("D%d: Empty\n",i+1);
	  else
	    printf("D%d: (%d) (%s) %s\n",i+1,deviceSlots.slot[i].hostSlot+1,(deviceSlots.slot[i].mode==0x03 ? "R/W" : "R/O"),deviceSlots.slot[i].file);
	}
    }
  else
    {
      sio_error();
      return 1;
    }
  
  return 0;
}

/**
 * List TNFS directory on host slot
 */
unsigned char list_directory(int argc, char* argv[])
{
  unsigned char s=atoi(argv[2]);

  if (argc<3)
    {
      printf("\n%s LS <HS#>\n",argv[0]);
      return 1;
    }
  else if ((s<1) || (s>8))
    {
      printf("\nINVALID SLOT NUMBER.\n");
      return 1;
    }

  // 0 index
  s-=1;

  // read host slots
  host_read();

  // Open TNFS directory
  OS.dcb.ddevic=0x70;
  OS.dcb.dunit=1;
  OS.dcb.dcomnd=0xF7;
  OS.dcb.dstats=0x80;
  OS.dcb.dbuf=&path;
  OS.dcb.dtimlo=0x0F;
  OS.dcb.dbyt=256;
  OS.dcb.daux=s;
  siov();

  if (OS.dcb.dstats!=1)
    {
      sio_error();
      exit(OS.dcb.dstats);
    }

  // Read directory
  while (path[0]!=0x7F)
    {
      memset(path,0,sizeof(path));
      path[0]=0x7f;
      OS.dcb.dcomnd=0xF6;
      OS.dcb.dstats=0x40;
      OS.dcb.dbuf=&path;
      OS.dcb.dbyt=36;
      OS.dcb.daux1=36;
      OS.dcb.daux2=s;
      siov();
      
      if (OS.dcb.dstats!=1)
	{
	  sio_error();
	  exit(OS.dcb.dstats);
	}
      else if (path[0]=='.')
	continue;
      else if (path[0]==0x7F)
	break;
      else
	printf("%s\n",path);
    }

  // Close directory
  OS.dcb.dcomnd=0xF5;
  OS.dcb.dstats=0x00;
  OS.dcb.dbuf=NULL;
  OS.dcb.dtimlo=0x01;
  OS.dcb.dbyt=0;
  OS.dcb.daux=s;
  siov();

  if (OS.dcb.dstats!=1)
    {
      sio_error();
      exit(OS.dcb.dstats);
    }

  return 0;
}
