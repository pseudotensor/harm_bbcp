/******************************************************************************/
/*                                                                            */
/*                       b b c p _ B u f f P o o l . C                        */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#if defined(MACOS) || defined(AIX)
#define memalign(pgsz,amt) valloc(amt)
#else
#include <malloc.h>
#endif

#include <sys/types.h>
#include <netinet/in.h>
#include "bbcp_Debug.h"
#include "bbcp_Emsg.h"
#include "bbcp_Headers.h"
#include "bbcp_Pthread.h"
#include "bbcp_BuffPool.h"

/******************************************************************************/
/*                         G l o b a l   O b j e c t                          */
/******************************************************************************/

bbcp_BuffPool bbcp_APool("disk");
bbcp_BuffPool bbcp_BPool;
bbcp_BuffPool bbcp_CPool("C");
  
/******************************************************************************/
/*                           c o n s t r u c t o r                            */
/******************************************************************************/
  
bbcp_BuffPool::bbcp_BuffPool(const char *id) : EmptyBuffs(0), FullBuffs(0)
{

// Do gross initialization
//
   last_empty = 0;
   last_full  = 0;
   next_full  = 0;
   datasz     = 0;
   buffsz     = 0;
   numbuf     = 0;
   RU486      = 0;
   pname      = id;
}

/******************************************************************************/
/*                            D e s t r u c t o r                             */
/******************************************************************************/
  
bbcp_BuffPool::~bbcp_BuffPool()
{
   bbcp_Buffer *currp;
   int i=16, j = 1;

// Free all of the buffers in the empty queue
//
   while(currp = last_empty)
        {last_empty = last_empty->next; delete currp;}
//cerr <<bbcp_Debug.Who <<"Bdestroy num " <<j++ <<" @ " <<hex <<(int)currp <<dec <<endl;

// Free all full buffers
//
   FullPool.Lock();
   while(currp = next_full)
        {next_full = next_full->next; delete currp;}
   FullPool.UnLock();
}
  
/******************************************************************************/
/*                                 A b o r t                                  */
/******************************************************************************/

void bbcp_BuffPool::Abort()
{

// Set the abort flag and then post the empty buffer pool. This will cause
// anyone looking for an empty buffer to abort. They in turn will cascade
// that information on to the next person.
//
   EmptyPool.Lock();
   RU486 = 1;
   EmptyBuffs.Post();
   EmptyPool.UnLock();

// Do the same for the full buffer queue
//
   FullPool.Lock();
   FullBuffs.Post();
   FullPool.UnLock();
   DEBUG("Aborting the " <<pname <<" buffer pool.");
}
  
/******************************************************************************/
/*                              A l l o c a t e                               */
/******************************************************************************/
  
int bbcp_BuffPool::Allocate(int buffnum, int  bsize)
{
   bbcp_Buffer *new_empty;
   int bnum = buffnum, allocsz;
   char *buffaddr;
   typedef unsigned long long ull_t;

// Check if this is an extend call or an initialization call
//
   EmptyPool.Lock();
   if (!numbuf)
      {buffsz = bsize;
       datasz = bsize  - sizeof(bbcp_Header);
      }
   allocsz    = buffsz + sizeof(bbcp_Header); // This is simply sloppy here

// Allocate the appropriate number of buffers aligned on page boundaries
//
   while (bnum--)
         {if (!(new_empty = new bbcp_Buffer())
          ||  !(new_empty->data = (char *)memalign(sysconf(_SC_PAGESIZE),allocsz)))
             {bbcp_Emsg("BuffPool", ENOMEM, "allocating buffers.");
              if (new_empty) delete new_empty;
              break;
             }
          new_empty->next = last_empty;
          last_empty = new_empty;
          EmptyBuffs.Post();
         }
   numbuf += buffnum-(bnum+1);
   EmptyPool.UnLock();

// All done.
//
   DEBUG("Allocated " <<buffnum-(bnum+1) <<' ' <<buffsz <<" byte buffs in the " <<pname <<" pool.");
   return bnum+1;
}

/******************************************************************************/
/*                               g o o d W S Z                                */
/******************************************************************************/

int  bbcp_BuffPool::goodWSZ(int  wsz, int  maxsz)
{

// For now just return a window size that will correspond to the requested
// quantity plus the overhead. Eventually we will compute the optimum size.
//
   wsz += sizeof(bbcp_Header);
   return (wsz > maxsz ? maxsz : wsz);
}
  
/******************************************************************************/
/*                          g e t E m p t y B u f f                           */
/******************************************************************************/
  
bbcp_Buffer *bbcp_BuffPool::getEmptyBuff()
{
   bbcp_Buffer *buffp = 0;

// Do this until we get an empty buffer
//
   while(!buffp & !RU486)
        {
      // Wait for an empty buffer
      //
         EmptyBuffs.Wait();

      // Get the buffer
      //
         EmptyPool.Lock();
         if (!RU486 && (buffp = last_empty)) last_empty = buffp->next;
         EmptyPool.UnLock();
       }

// Return the buffer
//
   if (RU486) EmptyBuffs.Post();
   return buffp;
}

/******************************************************************************/
/*                          p u t E m p t y B u f f                           */
/******************************************************************************/
  
void bbcp_BuffPool::putEmptyBuff(bbcp_Buffer *buffp)
{

// Obtain the pool lock and hang the buffer
//
   EmptyPool.Lock();
   buffp->next = last_empty;
   last_empty  = buffp;
   EmptyPool.UnLock();

// Signal availability of an empty buffer
//
   EmptyBuffs.Post();
}
 
/******************************************************************************/
/*                           g e t F u l l B u f f                            */
/******************************************************************************/
  
bbcp_Buffer *bbcp_BuffPool::getFullBuff()
{
   bbcp_Buffer *buffp = 0;

// Do this until we get a full buffer
//
   while(!buffp & !RU486)
        {
      // Wait for a full buffer
      //
         FullBuffs.Wait();

      // Get the buffer
      //
         FullPool.Lock();
         if (!RU486 && (buffp = next_full))
            if (!(next_full = buffp->next)) last_full = 0;
         FullPool.UnLock();
        }

// Return the buffer
//
   if (RU486) FullBuffs.Post();
   return buffp;
}
 
/******************************************************************************/
/*                           p u t F u l l B u f f                            */
/******************************************************************************/
  
void bbcp_BuffPool::putFullBuff(bbcp_Buffer *buffp)
{

// Obtain the pool lock and hang the buffer
//
   FullPool.Lock();
   if (last_full) last_full->next = buffp;
      else next_full = buffp;
   last_full = buffp;
   buffp->next = 0;
   FullPool.UnLock();

// Signal availability of a full buffer
//
   FullBuffs.Post();
}
 
/******************************************************************************/
/*                       H e a d e r   H a n d l i n g                        */
/******************************************************************************/
/******************************************************************************/
/*  S e r a i l i z a t i o n / D e s e r i a l i z a t i o n   M a c r o s   */
/******************************************************************************/

#define USBUFF int  USbuffL, USbuff[2]; unsigned short USbuffS; char *UScp

/******************************************************************************/
  
#define Ser(x,y) memcpy((void *)y,(const void *)&x,sizeof(x));

#define SerC(x,y)  *y = x;

#define SerS(x,y)  USbuffS = htons(x); Ser(USbuffS,y)

#define SerL(x,y)  USbuffL = htonl(x); Ser(USbuffL,y)

#define SerLL(x,y) USbuff[0] = x >>32; USbuff[1] = x & 0xffffffff; \
                   SerL(USbuff[0],y); UScp = y+4; SerL(USbuff[1],UScp)

/******************************************************************************/

#define Uns(x,y) memcpy((void *)&x,(const void *)y,sizeof(x));

#define UnsC(x,y)  x = *y; y++

#define UnsS(x,y)  Uns(USbuffS,y); x = htons(USbuffS)

#define UnsL(x,y)  Uns(USbuffL,y); x = ntohl(USbuffL)

#define UnsLL(x,y) UnsL(USbuff[0],y); UScp = y+4; UnsL(USbuff[1],UScp); \
                   x = ((unsigned long long)USbuff[0]) << 32 | \
                        (unsigned      int )USbuff[1]

/******************************************************************************/
/*                                D e c o d e                                 */
/******************************************************************************/
  
void bbcp_BuffPool::Decode(bbcp_Buffer *bp)
{
     USBUFF;
     bbcp_Header *hp = &bp->bHdr;

// Extract out the stream number
//
   UnsS(bp->snum, hp->snum);

// Extract out the length
//
   UnsL(bp->blen, hp->blen);

// Extract out the offset
//
   UnsLL(bp->boff, hp->boff);
}
 
/******************************************************************************/
/*                                E n c o d e                                 */
/******************************************************************************/
  
void bbcp_BuffPool::Encode(bbcp_Buffer *bp, char xcmnd, char xargs)
{
     USBUFF;
     bbcp_Header *hp = &bp->bHdr;

// Set the command and argument
//
   hp->cmnd    = xcmnd;
   hp->args    = xargs;

// Set the stream number
//
   SerS(bp->snum, hp->snum);

// Set the length
//
   SerL(bp->blen, hp->blen);

// Set the offset
//
   SerLL(bp->boff, hp->boff);
}
