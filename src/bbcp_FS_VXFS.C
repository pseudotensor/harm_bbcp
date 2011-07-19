/******************************************************************************/
/*                                                                            */
/*                        b b c p _ F S _ V X F S . C                         */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/
  
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#ifndef FREEBSD
#include <sys/statvfs.h>
#endif
#ifdef BBCP_VXFS
// #include <sys/fs/vx_ioctl.h>
#include <vx_ioctl.h>
#endif
#include "bbcp_Debug.h"
#include "bbcp_File.h"
#include "bbcp_FS_VXFS.h"
#include "bbcp_Platform.h"
#include "bbcp_System.h"

/******************************************************************************/
/*                        G l o b a l   O b j e c t s                         */
/******************************************************************************/

extern bbcp_System bbcp_OS;
  
/******************************************************************************/
/*                            A p p l i c a b l e                             */
/******************************************************************************/
  
int bbcp_FS_VXFS::Applicable(const char *path)
{
#ifdef BBCP_VXFS
   struct statvfs buf;


// To find out whether or not we are applicable, simply do a statvfs on the
// incomming path. If we can do it, then we are a unix filesystem.
//
   if (statvfs(path,&buf) || strcmp("vxfs",(const char *)buf.f_basetype))
      return 0;

// Save the path to this filesystem if we don't have one. This is a real
// kludgy short-cut since in bbcp we only have a single output destination.
//
   if (!fs_path)
      {fs_path = strdup(path);
       memcpy((void *)&fs_id, (const void *)&buf.f_fsid, sizeof(fs_id));
      }
   return 1;
#else
   return 0;
#endif
}

/******************************************************************************/
/*                                  O p e n                                   */
/******************************************************************************/

bbcp_File *bbcp_FS_VXFS::Open(const char *fn, int opts, int mode)
{
    int i, FD;
    bbcp_IO *iob;

// Open the file
//
   if ((FD = (mode ? open(fn, opts, mode) : open(fn, opts))) < 0) 
      return (bbcp_File *)0;

// Set cache advisory based on type of i/o (read or r/w)
//
#ifdef BBCP_VXFS
   if (opts & (O_WRONLY | O_RDWR | O_APPEND))
      {i = ioctl(FD, VX_SETCACHE, VX_NOREUSE);
       DEBUG("Open SETCACHE NOREUSE rc=" <<(i ? errno : 0));
      }
      else 
      {i = ioctl(FD, VX_SETCACHE, VX_DIRECT);
       DEBUG("Open SETCACHE DIRECT  rc=" <<(i ? errno : 0));
      }
#endif

// Allocate a file object and return that
//
   iob =  new bbcp_IO(FD);
   return new bbcp_File(fn, iob, (bbcp_FileSystem *)this);
}
