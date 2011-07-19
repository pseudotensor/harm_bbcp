#ifndef __BBCP_LOGFILE_H__
#define __BBCP_LOGFILE_H__
/******************************************************************************/
/*                                                                            */
/*                        b b c p _ L o g F i l e . h                         */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/
  
#include <unistd.h>
#include <sys/uio.h>
#include "bbcp_Timer.h"
#include "bbcp_Stream.h"
#include "bbcp_Pthread.h"

#define  BBCP_LOGFILE_MAX 4

class bbcp_LogFile
{
public:

int     Open(const char *fname);

void    Monitor(int fdnum, char *fdname);

void    Record();

        bbcp_LogFile();
       ~bbcp_LogFile();

private:

bbcp_Timer   Mytime;
bbcp_Mutex   Flog;
bbcp_Stream *fdstr[BBCP_LOGFILE_MAX];
int          fdmon[BBCP_LOGFILE_MAX];
char        *fdid[BBCP_LOGFILE_MAX];
int          fdcnt;
int          Logfd;
char        *Logfn;
pthread_t    Logtid;
int          Sigfd;
int          notdone;

int  fmtTime(char *tbuff);
void Remove(int fd, int emsg);
void Process(int fd);
};
#endif
