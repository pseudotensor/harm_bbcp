#ifndef __BBCP_BUFFPOOL_H__
#define __BBCP_BUFFPOOL_H__
/******************************************************************************/
/*                                                                            */
/*                       b b c p _ B u f f P o o l . h                        */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/

#include <stdlib.h>
#include <strings.h>
#include "bbcp_Pthread.h"

struct bbcp_Header
       {char cmnd;          // Command
        char args;          // Command arguments
        char snum[2];       // Stream number
        char blen[4];       // int       buffer length
        char boff[8];       // Long long buffer offset
        char cksm[16];      // MD5 check sum (optional) also args
       bbcp_Header() {bzero(cksm, sizeof(cksm));}
      ~bbcp_Header() {}
       };

// Valid commands
//
#define BBCP_IO       0x00
#define BBCP_CLOSE    0x04
#define BBCP_SETWS    0x05
#define BBCP_ABORT    0xFF

// Arguments
//
#define BBCP_MD5CHK   0x01

struct bbcp_Buffer
       {struct bbcp_Buffer *next;
        int                 blen;
        long long           boff;
        bbcp_Header         bHdr;
        char               *data;
        short               snum;

        bbcp_Buffer() {next=0; blen=0; boff=0; data=0; snum=0;}
       ~bbcp_Buffer() {if (data) free(data);}
       };
  
class bbcp_BuffPool
{
public:
void         Abort();

int          Allocate(int buffnum, int bsize);

int          BuffCount() {return numbuf;}

int          BuffSize()  {return buffsz;}

int          DataSize() {return datasz;}

void         Encode(bbcp_Buffer *bp, char xcmnd, char xargs);

void         Decode(bbcp_Buffer *bp);

int          goodWSZ(int  wsz, int  maxsz);

bbcp_Buffer *getEmptyBuff();

void         putEmptyBuff(bbcp_Buffer *buff);

bbcp_Buffer *getFullBuff();

void         putFullBuff(bbcp_Buffer *buff);

             bbcp_BuffPool(const char *id="net");
            ~bbcp_BuffPool();

private:

bbcp_Mutex EmptyPool;
bbcp_Mutex FullPool;
bbcp_Semaphore EmptyBuffs;
bbcp_Semaphore FullBuffs;

int         numbuf;
int         buffsz;
int         datasz;
int         RU486;
const char *pname;

bbcp_Buffer *next_full;
bbcp_Buffer *last_full;
bbcp_Buffer *last_empty;
};
#endif
