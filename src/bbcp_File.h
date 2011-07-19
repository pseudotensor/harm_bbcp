#ifndef __BBCP_FILE_H__
#define __BBCP_FILE_H__
/******************************************************************************/
/*                                                                            */
/*                           b b c p _ F i l e . h                            */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/

#include <string.h>
#include "bbcp_BuffPool.h"
#include "bbcp_IO.h"
#include "bbcp_Pthread.h"

// The bbcp_File class describes the set of operations that are needed to copy
// a "file". The actual I/O are determined by the associated filesystem and
// are specified during instantiation.

class bbcp_FileSystem;

class bbcp_File
{
public:

// Close the file
//
int          Close();

// Return target file system
//
bbcp_FileSystem *Fsys() {return FSp;}

// Return FD number
//
int          ioFD() {return IOB->FD();}

// Write a record to a file
//
ssize_t      Get(char *buff, size_t blen) {return IOB->Read(buff, blen);}

// Return path to the file
//
char        *Path() {return iofn;}

// Write a record to a file
//
ssize_t      Put(char *buff, size_t blen) {return IOB->Write(buff, blen);}

// Read_All() reads the file until eof and returns 0 (success) or -errno.
//
int          Read_All(bbcp_BuffPool &buffpool, int blkf);

// Write_All() writes the file until eof and return 0 (success) or -errno.
//
int          Write_All(bbcp_BuffPool &buffpool, int nstrms);

// Sets the file pointer to read or write from an offset
//
int          Seek(long long offv) {nextoffset = offv; return IOB->Seek(offv);}

// Set buffer size for future I/O

void         setBuffSize(bbcp_BuffPool &buffpool, int  bsz);

// setSize() sets the expected file size
//
void         setSize(long long top) {lastoff = top;}

// setSN() sets the stream number to allow for multiplexed file I/O.
//
void         setSN(int sn) {snum = sn;}

// Stats() reports the i/o time and buffer wait time in milliseconds and
//         returns the total number of bytes transfered.
//
long long    Stats(double &iotime) {return IOB->ioStats(iotime);}

long long    Stats()               {return IOB->ioStats();}

             bbcp_File(const char *path, bbcp_IO *iox, bbcp_FileSystem *fsp);

            ~bbcp_File() {if (iofn) free(iofn);
                          if (IOB)  delete IOB;
                         }

int          bufreorders;
int          maxreorders;

private:

int              curq;
bbcp_Buffer     *nextbuff;
long long        nextoffset;
long long        lastoff;
short            snum;
bbcp_IO         *IOB;
bbcp_FileSystem *FSp;
char            *iofn;

int              newBuffSize;
int              curBuffSize;
bbcp_Mutex       ctlmutex;

int          getBuffSize();
bbcp_Buffer *getBuffer(long long offset);
};
#endif
