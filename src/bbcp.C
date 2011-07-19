/******************************************************************************/
/*                                                                            */
/*                                b b c p . C                                 */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/
  
/* bbcp provides a secure fast parallel copy utility. The syntax is:

   bbcp [options] inspec outspec

   options: [-a [dir]] [-b num] [-c [lvl]] [-C <fname>] [-d <path>] [-D] [-e]
            [-f] [-i <fname>] [-I <fname>] [-k] [-l fn]
            [-L <spec>[@<host>[:<port>]]
            [-m mode] [-p] [-P sec] [-s strms] [-S <srccmd>] [-T <trgcmd>]
            [-v] [-V] [-w wsz] [-W wsz] [-x rate] [-z]

   inspec & oouspec: [username@][hostname:]path

Where:
   -a     Appends data  That is, data that has not been copied from a previous
          copy operation is appended to the file. This is useful when
          restarting a failed copy. An optional checkpoint directory may be
          specified with this option. The -a option is not allowed when data
          comes from standard in or goes to standard out.

   -b     sets the read block factor (i.e., number of network buffers filled
          at once from source before being sent). The default is 1.

   -d     Indicates that source files are relative to the specified path and
          that relative source paths must be created at the target relative
          to the target path.

   -D     Turns on debugging to standard error.

   -c     Compress the data before sending it. A compression level, 1 to 9,
          may be specified (default is 5).

   -C     Specifies the configuration file. The file is processed at the time
          it is encountered on the command line. The default is to look for the
          filename in env variable bbcp_CONFIGFN and then for <home>/.bbcp.cf.

   -e     Error checking. Use MD5 checksums to make sure file data was correctly
          sent. This introduces significant overhead.

   -f     Forces the copy. If the destination file is unlinked prior to the
          copy operation. This overides the -a options.

   -i     specifies the ssh identity file to be used for source and target.

   -I     Specifies the the file that holds the list of source files to be
          copied. Each file specification is delimited by a newline. Files
          are copied in the order specified, in addition to any specified
          on the command line.

   -k     Keep the file. Normally, errors cause the destination file to be
          erased. This allows the file to be kept.

   -l     Writes standard error out to the indicated file.

   -L     Sets the logging options. For <spec>, specify a list of one or
          more of the following characters: i (log net input), o (log net
          output), r (log file reads), w (log file writes). For <host>
          specify the place where the log daemon runs, and for <port> the
          port on which is listens.

   -m     The destination mode for the file. The default is 644 (see -p).

   -p     Preserve the source mode, owner, group, and timestamps.

   -P     Produce a progress message every sec seconds (minimum is 30).

   -s     The number of parallel network streams. Thee default is 4.

   -S     Is the command to be used to start the source node copy. Typically,
          this is the ssh command followed by the bbcp command. Use %H to
          mark ssh host substitution, %U to mark ssh user substitution, and
          %I to mark identify file substitution (which include -i as the
          option name preceeding the filename).

   -T     Is the command to be used to start the target node copy. All -S
          consideration apply to -T.

   -v     Verbose mode.

   -V     Very verbose mode.

   -w     Window size. The prefered window size. The default is 8k. Specify
          any value < 2G (the value may be suffixed k, m, or g).

   -W     Window size. Same as -w but forces the window to be sized exactly
          to the specified value which will result in a smaller data buffer.

   -x     Is teh maximum transfer rate allowed in bytes, K, M, or G.

   -z     Reverse connect protocol to be used (i.e., source connects to sink
          as opposed to sink connects to source).
*/

/******************************************************************************/
/*                         i n c l u d e   f i l e s                          */
/******************************************************************************/
  
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/param.h>

#include "bbcp_Args.h"
#include "bbcp_Config.h"
#include "bbcp_Debug.h"
#include "bbcp_FileSpec.h"
#include "bbcp_Headers.h"
#include "bbcp_LogFile.h"
#include "bbcp_Node.h"
#include "bbcp_ProcMon.h"
#include "bbcp_Protocol.h"
#include "bbcp_System.h"
#include "bbcp_Timer.h"
  
/******************************************************************************/
/*                    L O C A L   D E F I N I T I O N S                       */
/******************************************************************************/

#define Same(s1, s2) (s1 == s2 || (s1 && s2 && !strcmp(s1, s2)))

/******************************************************************************/
/*                      G l o b a l   V a r i a b l e s                       */
/******************************************************************************/

extern bbcp_Config   bbcp_Config;

extern bbcp_BuffPool bbcp_BuffPool;
  
extern bbcp_System   bbcp_OS;

/******************************************************************************/
/*                                  m a i n                                   */
/******************************************************************************/
  
main(int argc, char *argv[], char *envp[])
{
   bbcp_Node     *Source, *Sink;
   bbcp_Protocol  Protocol;
   bbcp_FileSpec *fsp, *psp, *sfs, *tfs;
   int retc;
   int        TotFiles = 0;
   long long  TotBytes = 0;
   bbcp_Timer Elapsed_Timer;

// Process configuration file
//
   bbcp_OS.EnvP = envp;
   if (bbcp_Config.ConfigInit(argc, argv)) exit(1);

// Process the arguments
//
   bbcp_Config.Arguments(argc, argv);

// Process final source/sink actions here
//
   if (bbcp_Config.Options & (bbcp_SRC | bbcp_SNK))
      {int retc;
       bbcp_ProcMon theAgent;
       theAgent.Start(bbcp_OS.getGrandP());
           {bbcp_Node     SS_Node;
            retc = (bbcp_Config.Options & bbcp_SRC
                 ?  Protocol.Process(&SS_Node)
                 :  Protocol.Request(&SS_Node));
           }
       exit(retc);
      }

// Do some debugging here
//
   Elapsed_Timer.Start();
   if (bbcp_Debug.Trace > 2) bbcp_Config.Display();

// Allocate the source and sink node and common protocol
//
   Source = new bbcp_Node;
   Sink   = new bbcp_Node;
   tfs    = bbcp_Config.snkSpec;

// Allocate the log file
//
   if (bbcp_Config.Logfn)
      {bbcp_Config.MLog = new bbcp_LogFile();
       if (bbcp_Config.MLog->Open((const char *)bbcp_Config.Logfn)) exit(5);
      }

// Grab all source files for each particular user/host and copy them
//
   retc = 0;
   while(!retc && (psp = bbcp_Config.srcSpec))
      {fsp = psp->next;
       while(fsp && Same(fsp->username, psp->username)
             && Same(fsp->hostname, psp->hostname)) 
            {psp = fsp; fsp = fsp->next;}
       psp->next = 0;
       sfs = bbcp_Config.srcSpec;
       bbcp_Config.srcSpec = fsp;
         if (bbcp_Config.Options & bbcp_CON2SRC)
            retc = Protocol.Schedule(Source, sfs, 
                                     (char *)bbcp_Config.SrcXeq,
                                     (char *)bbcp_Config.SrcArg,
                                     Sink,   tfs,
                                     (char *)bbcp_Config.SnkXeq,
                                     (char *)bbcp_Config.SnkArg, Sink);
            else
            retc = Protocol.Schedule(Sink,   tfs,
                                     (char *)bbcp_Config.SnkXeq,
                                     (char *)bbcp_Config.SnkArg,
                                     Source, sfs,
                                     (char *)bbcp_Config.SrcXeq,
                                     (char *)bbcp_Config.SrcArg, Sink);
       TotFiles += Sink->TotFiles;
       TotBytes += Sink->TotBytes;
      }

// All done
//
   delete Source;
   delete Sink;

// Report final statistics if wanted
//
   DEBUG("Ending; rc=" <<retc <<" files=" <<TotFiles <<" bytes=" <<TotBytes);
   if (bbcp_Config.Options & (bbcp_BLAB | bbcp_VERBOSE) 
   && !retc && TotFiles && TotBytes)
      {double ttime;
       char buff[64];
       Elapsed_Timer.Stop();
       Elapsed_Timer.Report(ttime);
       sprintf(buff, "%.1f", (((double)TotBytes)/ttime)/1024.0);
       cerr <<TotFiles <<(TotFiles != 1 ? " files" : " file");
       cerr <<" copied at effectively " <<buff <<" KB/s" <<endl;
      }
   exit(retc);
}
