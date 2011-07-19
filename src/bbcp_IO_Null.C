/******************************************************************************/
/*                                                                            */
/*                        b b c p _ I O _ N u l l . C                         */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "bbcp_IO_Null.h"

/******************************************************************************/
/*                                 W r i t e                                  */
/******************************************************************************/
  
ssize_t bbcp_IO_Null::Write(char *buff, size_t wrsz)
{

// Just add up the bytes
//
   xfrtime.Start();
   xfrbytes += wrsz;
   xfrtime.Stop();

// All done
//
   return wrsz;
}

ssize_t bbcp_IO_Null::Write(const struct iovec *iovp, int iovn)
{
   ssize_t iolen = 0;

// Add up bytes
//
   xfrtime.Start();
   while(iovn--) {iolen += iovp->iov_len; iovp++;}
   xfrbytes += iolen;
   xfrtime.Stop();

// All done
//
   return iolen;
}
