/******************************************************************************/
/*                                                                            */
/*                        b b c p _ L o g F i l e . C                         */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/
  
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bbcp_Debug.h"
#include "bbcp_Emsg.h"
#include "bbcp_Headers.h"
#include "bbcp_LogFile.h"
#include "bbcp_Platform.h"

#ifdef MACOS
#define O_DSYNC 0
#endif

/******************************************************************************/
/*           E x t e r n a l   I n t e r f a c e   R o u t i n e s            */
/******************************************************************************/
  
extern "C"
{
void *bbcp_FileLog(void *op)
{
     bbcp_LogFile *lfp = (bbcp_LogFile *)op;
     lfp->Record();
     return (void *)0;
}
}

/******************************************************************************/
/*                           C o n s t r u c t o r                            */
/******************************************************************************/
  
bbcp_LogFile::bbcp_LogFile()
{
   fdcnt   = 0;
   Logfd   = 0;
   Logfn   = 0;
   Logtid  = 0;
   Sigfd   = 0;
   notdone = 1;
}

/******************************************************************************/
/*                            D e s t r u c t o r                             */
/******************************************************************************/
  
bbcp_LogFile::~bbcp_LogFile()
{
   int i;

// If the monitor thread is active, stop it
//
   if (Logtid)
      {notdone = 0;
       write(Sigfd, "x", 1);
       bbcp_Thread_Wait(Logtid);
      }

// Close the log file and signal pipe
//
   if (Logfd) close(Logfd);
   if (Logfn) free(Logfn);
   if (Sigfd) close(Sigfd);

// Cleanup all other file descriptors
//
   for (i = 0; i < fdcnt; i++)
       {close(fdmon[i]);
        if (fdid[i]) free(fdid[i]);
        if (fdstr[i]) {fdstr[i]->Detach(); delete fdstr[i];}
       }

// All done
//
   return;
}

/******************************************************************************/
/*                                  O p e n                                   */
/******************************************************************************/
  
int bbcp_LogFile::Open(const char *fname)
{
   int fildes[2], logfd;

// Check if we have a logfile already
//
   if (Logfd) return -ETXTBSY;

// Open the log file
//
   if ((logfd = open(fname, O_WRONLY | O_CREAT | O_APPEND | O_DSYNC, 0644)) < 0)
      return bbcp_Emsg("LogFile", -errno, "opening", (char *)fname);

// Set up for logging
//
   Logfd   = logfd;
   Logfn   = strdup(fname);

// Get a pipe to signal the monitoring thread of any changes
//
   if (pipe(fildes))
      return bbcp_Emsg("LogFile", -errno, "allocating a signal pipe.");
   fdmon[0] = fildes[0];
   fdid[0]  = strdup("Signal pipe");
   fdstr[0] = 0;
   Sigfd    = fildes[1];
   fdcnt    = 1;

// All done
//
   return 0;
}
  
/******************************************************************************/
/*                               M o n i t o r                                */
/******************************************************************************/
  
void bbcp_LogFile::Monitor(int fdnum, char *fdname)
{
   int retc = 0;

   Flog.Lock();
   if (fdcnt < BBCP_LOGFILE_MAX) 
      {fdid[fdcnt] = strdup(fdname); fdmon[fdcnt++] = fdnum;}
      else retc = bbcp_Fmsg("LogFile", "Logfile monitoring capacity exceeded;",
                                       fdname, (char *)"not being logged.");
   Flog.UnLock();
   if (retc) return;

// If we have a monitoring thread, signal it otherwise start a new thread
//
   if (Logtid) write(Sigfd, "x", 1);
      else {retc = bbcp_Thread_Start(bbcp_FileLog, (void *)this, &Logtid);
            DEBUG("Thread " <<Logtid <<" assigned to logging,");
           }

// Create a stream for this file descriptor to avoid line breakage
//
   fdstr[fdcnt-1] = new bbcp_Stream();
   fdstr[fdcnt-1]->Attach(fdnum);

// All done
//
   return;
}
 
/******************************************************************************/
/*                                R e c o r d                                 */
/******************************************************************************/

void bbcp_LogFile::Record()
{
   int i, pollcnt;
   struct pollfd pollv[BBCP_LOGFILE_MAX];

// In a big loop, wait for any activity from monitored fd's and write to the
// log file as well as to standard error.
//
while (notdone)
      {
      // Lock against simultaneous access to get the fd's
      //
      Flog.Lock();
      for (pollcnt = 0; pollcnt < fdcnt; pollcnt++)
          {pollv[pollcnt].fd      = fdmon[pollcnt];
           pollv[pollcnt].events  = POLLIN | POLLRDNORM;
           pollv[pollcnt].revents = 0;
          }
      Flog.UnLock();

      // Issue the poll
      //
      pollcnt--;
      if (poll(pollv, pollcnt, -1) < 0)
         {if (errno == EINTR || errno == EAGAIN) continue;
          bbcp_Emsg("LogFile", errno, "polling file descriptors.");
         }

      // Process all events
      //
      for (i = 0; i < pollcnt; i++)
          {     if (pollv[i].revents & (POLLIN  | POLLRDNORM)) Process(i);
           else if (pollv[i].revents & POLLHUP)                Remove(i, 0);
           else if (pollv[i].revents & (POLLERR | POLLNVAL))   Remove(i, 1);
          }
      }

DEBUG("Logging terminated.");
return;
}
  
/******************************************************************************/
/*                       P r i v a t e   M e t h o d s                        */
/******************************************************************************/
/******************************************************************************/
/*                               P r o c e s s                                */
/******************************************************************************/
  
void bbcp_LogFile::Process(int fdi)
{
   int FD = fdmon[fdi];
   char         tbuff[24];
   struct iovec iolist[3];
   static bbcp_Mutex logMutex;

// Read and toss data if we don't have a stream or it's the 1st fd
//
   if (!fdi || !fdstr[fdi])
      {char xbuff[16];
       read(fdmon[fdi], (void *)xbuff, (size_t)sizeof(xbuff));
       return;
      }

// Get a full line from the stream to avoid line splittage in the log
//
   if (!(iolist[1].iov_base = fdstr[fdi]->GetLine()))
      {Remove(fdi, 0);
       return;
      }

// Construct the message to be sent
//
    tbuff[0] = '\0'; 
    iolist[0].iov_base  = (caddr_t)tbuff;
    iolist[0].iov_len   = Mytime.Format(tbuff);
    iolist[1].iov_len   = strlen((char *)iolist[1].iov_base);
    iolist[2].iov_base  = (char *)"\n";
    iolist[2].iov_len   = 1;

// Write the data to the log
//
   logMutex.Lock();
   if (Logfd && (writev(Logfd, (const struct iovec *)&iolist, 3) < 0))
      {bbcp_Emsg("LogFile", errno, "writing log to", Logfn);
       close(Logfd);
       Logfd = 0;
      }

// Write the message to standard error and return
//
   cerr <<iolist[1].iov_base <<endl;
   logMutex.UnLock();
   return;
}
 
/******************************************************************************/
/*                                R e m o v e                                 */
/******************************************************************************/
  
void bbcp_LogFile::Remove(int fdi, int emsg)
{
   int i, j=0;

// If and error message wanted, issue message
//
   if (emsg) bbcp_Fmsg("LogFile","Polling error occured monitoring",fdid[fdi]);

// Close and free up the description
//
   close(fdmon[fdi]);
   free(fdid[fdi]);

// Remove the offending fd
//
   Flog.Lock();
   for (i = 0; i < fdcnt-1; i++)
       if (i != fdi && j != i) {fdmon[j] = fdmon[i]; fdid[j++] = fdid[i];}
   fdcnt = fdcnt-1;
   Flog.UnLock();

// All done
//
   return;
}
