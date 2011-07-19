/******************************************************************************/
/*                                                                            */
/*                           b b c p _ F i l e . C                            */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/
  
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/uio.h>
#include "bbcp_Platform.h"
#include "bbcp_Config.h"
#include "bbcp_Debug.h"
#include "bbcp_Emsg.h"
#include "bbcp_File.h"
#include "bbcp_Headers.h"

/******************************************************************************/
/*                         G l o b a l   V a l u e s                          */
/******************************************************************************/
  
extern bbcp_BuffPool bbcp_CPool;

extern bbcp_Config   bbcp_Config;

/******************************************************************************/
/*                           C o n s t r u c t o r                            */
/******************************************************************************/

bbcp_File::bbcp_File(const char *path, bbcp_IO *iox, bbcp_FileSystem *fsp)
{
   nextbuff    = 0;
   nextoffset  = 0;
   lastoff     = 0;
   curq        = 0;
   snum        = 0;
   iofn        = strdup(path);
   newBuffSize = 0; 
   curBuffSize = 8192;
   IOB         = iox;
   FSp         = fsp;
   bufreorders = 0;
   maxreorders = 0;
}
  
/******************************************************************************/
/*                                 C l o s e                                  */
/******************************************************************************/
  
int bbcp_File::Close()
{

// Prevent infinite loops
//
   if (IOB->FD() >= 0) IOB->Close();
   return 0;
}

/******************************************************************************/
/*                           g e t B u f f S i z e                            */
/******************************************************************************/
  
int  bbcp_File::getBuffSize()
{
   int  bsz;
   ctlmutex.Lock(); 
   bsz = curBuffSize;
   newBuffSize = 0;
   ctlmutex.UnLock();
   return bsz;
}
 
/******************************************************************************/
/*                           s e t B u f f S i z e                            */
/******************************************************************************/
  
void bbcp_File::setBuffSize(bbcp_BuffPool &buffpool, int  bsz)
{
   int  datasz = buffpool.DataSize();

// Establish maximum (it cannot be greater than the actual data buffer
//
   if (bsz > datasz)
      {bsz = datasz;
       DEBUG("setBuffSize reduced buff size from " <<bsz <<" to " <<datasz);
      }

// Establish new buffer size for the current read requests
//
   ctlmutex.Lock(); 
   curBuffSize = bsz;
   newBuffSize = 1;
   ctlmutex.UnLock();
}

/******************************************************************************/
/*                              R e a d _ A l l                               */
/******************************************************************************/
  
int bbcp_File::Read_All(bbcp_BuffPool &buffpool, int blkf)
{
    int  rcvrdsz = bbcp_Config.MaxWindow - sizeof(bbcp_Header);
    int  rdsz = buffpool.DataSize();
    struct iovec iovector[IOV_MAX];
    bbcp_Buffer  *ibp, *inbuff[IOV_MAX];
    ssize_t blen, rlen = 0;
    int iovp, eof = 0, retc = 0;

// Establish logging options
//
   if (rdsz > rcvrdsz) rdsz = rcvrdsz;
   if (bbcp_Config.Options & bbcp_LOGRD) IOB->Log("DISK", 0);

// If the blocking is one, do a more simple reading arrangement
//
   if (blkf == 1)
   while(!eof)
      {
      // Get new read size if it has changed
      //
         if (newBuffSize) rdsz = getBuffSize();

      // Obtain buffers
      //
         if (!(ibp = buffpool.getEmptyBuff())) return 132;

      // Read data into the buffer
      //
         if ((rlen = IOB->Read(ibp->data, rdsz)) <= 0)
            {buffpool.putEmptyBuff(ibp);
             eof = !rlen;
             break;
            }

      // Queue a filled buffer for further processing
      //
         ibp->boff = nextoffset; nextoffset += rlen;
         ibp->blen = rlen;
         ibp->snum = snum;
         buffpool.putFullBuff(ibp);

      } else

// Read all of the data until eof (note that we are single threaded)
//
   while(!eof)
      {
      // Get new read size if it has changed
      //
         if (newBuffSize) rdsz = getBuffSize();

      // Obtain buffers
      //
         for (iovp = 0; iovp < blkf; iovp++)
             {if (!(inbuff[iovp] = buffpool.getEmptyBuff())) return 132;
              iovector[iovp].iov_base = (caddr_t)(inbuff[iovp]->data);
              iovector[iovp].iov_len  = rdsz;
             }

      // Read data into the buffer
      //
         if ((rlen = IOB->Read((const struct iovec *)&iovector, blkf))
                  <= 0) {for (iovp = 0; iovp < blkf; iovp++)
                             buffpool.putEmptyBuff(inbuff[iovp]);
                         eof = !rlen;
                         break;
                        }

      // Queue a filled buffers for further processing
      //
         for (iovp = 0; iovp < blkf && rlen; iovp++)
             {blen = (rlen >= rdsz ? rdsz : rlen);
              inbuff[iovp]->boff = nextoffset; nextoffset += blen;
              inbuff[iovp]->blen = blen;       rlen       -= blen;
              inbuff[iovp]->snum = snum;
              buffpool.putFullBuff(inbuff[iovp]);
             }

      // Return any empty buffers
      //
         eof = iovp < blkf;
         while(iovp < blkf) buffpool.putEmptyBuff(inbuff[iovp++]);
      }

// Check if we ended because of an error or end of file. If normal end
//
   if (rlen < 0 || !eof)
      {nextoffset = -1;
       if (rlen < 0) retc = bbcp_Emsg("Read", -rlen, "reading", iofn);
          else retc = 255;
      }
      else if (eof) IOB->Close();

// queue an empty buffer to signal end of input data the offset indicates
// how much data should have been sent and received. A negative offset
// indicates that we encountered an error.
//
   ibp = buffpool.getEmptyBuff();
   ibp->blen = 0;
   ibp->boff = nextoffset;
   buffpool.putFullBuff(ibp);
   DEBUG("EOF offset=" <<nextoffset <<" retc=" <<retc <<" fn=" <<iofn);
   return retc;
}

/******************************************************************************/
/*                             W r i t e _ A l l                              */
/******************************************************************************/
  
int bbcp_File::Write_All(bbcp_BuffPool &buffpool, int nstrms)
{
    bbcp_Buffer *outbuff;
    ssize_t wlen = 1;
    int numadd, maxbufs, maxadds = nstrms;
    int unordered = !(bbcp_Config.Options & bbcp_ORDER);

// Establish logging options
//
   if (bbcp_Config.Options & bbcp_LOGRD && IOB) IOB->Log(0, "DISK");

// Record the maximum number of buffers we have here
//
   maxbufs = buffpool.BuffCount();
   if (!(numadd = nstrms)) numadd = nstrms;

// Read all of the data until eof (note that we are single threaded)
//
   while(nstrms && wlen > 0)
      {
      // Obtain a full buffer
      //
         if (!(outbuff = buffpool.getFullBuff())) return 132;


     // Check if this is an eof marker
     //
        if (!(outbuff->blen))
           {buffpool.putEmptyBuff(outbuff); nstrms--; continue;}

      // Do an unordered write if allowed
      //
         if (unordered)
            {if (IOB)
                {if ((wlen=IOB->Write((char *)outbuff->data,(size_t)outbuff->blen,
                                     (off_t)outbuff->boff)) <= 0) break;
                 buffpool.putEmptyBuff(outbuff);
                } else bbcp_CPool.putFullBuff(outbuff);
             continue;
            }

      // Check if this buffer is in the correct sequence
      //
         if (outbuff->boff != nextoffset)
            {if (outbuff->boff < 0) {wlen = -ESPIPE; break;}
             outbuff->next = nextbuff;
             nextbuff = outbuff;
             bufreorders++;
             if (++curq > maxreorders) 
                {maxreorders = curq;
                 DEBUG("Buff disorder " <<curq <<" rcvd " <<outbuff->boff <<" want " <<nextoffset);
                }
             if (curq >= maxbufs)
                {if (!(--maxadds)) {wlen = -ENOBUFS; break;}
                 DEBUG("Too few buffs; adding " <<numadd <<" more.");
                 buffpool.Allocate(numadd, 0);
                 maxbufs += numadd;
                }
             continue;
            }

      // Write out this buffer and any queued buffers
      //
      do {if (IOB)
             {if ((wlen=IOB->Write((char *)outbuff->data,
                                   (size_t)outbuff->blen)) <= 0) break;
              nextoffset += wlen;
              buffpool.putEmptyBuff(outbuff);
             } else {
              nextoffset += outbuff->blen;
              bbcp_CPool.putFullBuff(outbuff);
             }
         } while(nextbuff && (outbuff = getBuffer(nextoffset)));
      }

// Check if we ended because of an error or end of file
//
   if (wlen >= 0 && nextbuff) wlen = -EILSEQ;
   if (wlen < 0) return bbcp_Emsg("Write", -wlen, "writing", iofn);
   if (IOB) IOB->Close();
      else {if (!(outbuff = buffpool.getEmptyBuff())) return 132;
            outbuff->blen = 0; outbuff->boff = nextoffset;
            bbcp_CPool.putFullBuff(outbuff);
           }
   return (nstrms && nextoffset < lastoff);
}

/******************************************************************************/
/*                       P r i v a t e   M e t h o d s                        */
/******************************************************************************/
/******************************************************************************/
/*                             g e t B u f f e r                              */
/******************************************************************************/

bbcp_Buffer *bbcp_File::getBuffer(long long offset)
{
   bbcp_Buffer *bp, *pp=0;

// Find a buffer
//
   if (bp = nextbuff)
      while(bp && bp->boff != offset) {pp = bp; bp = bp->next;}

// If we found a buffer, unchain it
//
   if (bp) {curq--;
            if (pp) pp->next = bp->next;
                else nextbuff = bp->next;
//if (!curq) {DEBUG("Queue has been emptied at offset " <<offset);}
           }

// Return what we have
//
   return bp;
}
