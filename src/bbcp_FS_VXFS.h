#ifndef __BBCP_FS_VXFS_H__
#define __BBCP_FS_VXFS_H__
/******************************************************************************/
/*                                                                            */
/*                        b b c p _ F S _ V X F S . h                         */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/
  
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "bbcp_FS_Unix.h"

class bbcp_FS_VXFS : public bbcp_FS_Unix
{
public:

int        Applicable(const char *path);

bbcp_File *Open(const char *fn, int opts, int mode=0);

      bbcp_FS_VXFS() {}
     ~bbcp_FS_VXFS() {}

private:
};
#endif
