/******************************************************************************/
/*                                                                            */
/*                         b b c p _ C o n f i g . C                          */
/*                                                                            */
/* (C) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*               DE-AC03-76-SFO0515 with the Deprtment of Energy              */
/******************************************************************************/

/*
   The routines in this file handle oofs() initialization. They get the
   configuration values either from configuration file or oofs_config.h (in that
   order of precedence).

   These routines are thread-safe if compiled with:
   AIX: -D_THREAD_SAFE
   SUN: -D_REENTRANT
*/
  
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/uio.h>

#define  BBCP_CONFIG_DEBUG
#define  BBCP_IOMANIP
#include "bbcp_Platform.h"
#include "bbcp_Args.h"
#include "bbcp_BuffPool.h"
#include "bbcp_Config.h"
#include "bbcp_Debug.h"
#include "bbcp_Emsg.h"
#include "bbcp_Headers.h"
#include "bbcp_NetLogger.h"
#include "bbcp_Network.h"
#include "bbcp_Platform.h"
#include "bbcp_Stream.h"
#include "bbcp_System.h"

#include "bbcp_Version.h"

/******************************************************************************/
/*                               d e f i n e s                                */
/******************************************************************************/

#define TS_Xeq(x,m)   if (!strcmp(x,var)) return m(Config);

#define TS_Str(x,m)   if (!strcmp(x,var)) {free(m); m = strdup(val); return 0;}

#define TS_Chr(x,m)   if (!strcmp(x,var)) {m = val[0]; return 0;}

#define TS_Bit(x,m,v) if (!strcmp(x,var)) {m |= v; return 0;}

/******************************************************************************/
/*                        G l o b a l   O b j e c t s                         */
/******************************************************************************/

extern bbcp_BuffPool  bbcp_BPool;

       bbcp_Config    bbcp_Config;

       bbcp_Debug     bbcp_Debug;

       bbcp_NetLogger bbcp_NetLog;

extern bbcp_Network   bbcp_Net;

extern bbcp_System    bbcp_OS;

extern bbcp_Version   bbcp_Version;
  
/******************************************************************************/
/*                           C o n s t r u c t o r                            */
/******************************************************************************/

bbcp_Config::bbcp_Config()
{

   SrcBuff   = 0;
   SrcBase   = 0;
   SrcUser   = 0;
   SrcHost   = 0;
   SrcBlen   = 0;
   srcPath   = 0;
   srcSpec   = 0;
   srcLast   = 0;
   snkSpec   = 0;
   CBhost    = 0;
   CBport    = 0;
   CopyOpts  = 0;
   LogSpec   = 0;
   RepSpec   = 0;
   accwait   = 30000;
   bindtries = 1;
   bindwait  = 0;
   Options   = 0;
   Mode      = 0644;
   Bfact     = 0;
   BNum      = 0;
   Streams   = 4;
   Xrate     = 0;
   Complvl   = 1;
   Progint   = 0;
   CXBsz     = 1024*1024;
// SrcXeq    = strdup("ssh -x -a -oFallBackToRsh=no -oServerAliveInterval=10 "
   SrcXeq    = strdup("ssh -x -a -oFallBackToRsh=no "
                      "%I -l %U %H bbcp");
// SnkXeq    = strdup("ssh -x -a -oFallBackToRsh=no -oServerAliveInterval=10 "
   SnkXeq    = strdup("ssh -x -a -oFallBackToRsh=no "
                      "%I -l %U %H bbcp");
   MinPort   = strdup("bbcpfirst");
   MaxPort   = strdup("bbcplast");
   Logurl    = 0;
   Logfn     = 0;
   MLog      = 0;
   MyAddr    = bbcp_Net.FullHostName((char *)0, 1);
   MyHost    = bbcp_Net.FullHostName((char *)0);
   MyUser    = bbcp_OS.UserName();
   MyProg    = 0;
   SecToken  = Rtoken();
   MaxSegSz  = bbcp_Net.MaxSSize();
   Hsize     = bbcp_BPool.goodWSZ(0, 65536);
   Wsize     = 65536+Hsize;
   MaxWindow = -1;
   lastseqno = 0;
   SrcArg    = "SRC";
   SnkArg    = "SNK";
   CKPdir    = 0;
   IDfn      = 0;
   TCPprotid = getProtID("tcp");
   NetQoS    = 0;
   TimeLimit = 0;
   MTLevel   = 0;
   Jitter    = 0;
}

/******************************************************************************/
/*                            D e s t r u c t o r                             */
/******************************************************************************/

bbcp_Config::~bbcp_Config()
{
   bbcp_FileSpec *nsp, *fsp;

// Delete all file spec objects chained from us
//
   fsp = srcPath;
   while(fsp) {nsp = fsp->next; delete fsp; fsp = nsp;}
   fsp = srcSpec;
   while(fsp) {nsp = fsp->next; delete fsp; fsp = nsp;}
   fsp = snkSpec;
   while(fsp) {nsp = fsp->next; delete fsp; fsp = nsp;}

// Delete all character strings
//
   if (SrcBuff)  free(SrcBuff);
   if (SrcXeq)   free(SrcXeq);
   if (SnkXeq)   free(SnkXeq);
   if (MinPort)  free(MinPort);
   if (MaxPort)  free(MaxPort);
   if (Logurl)   free(Logurl);
   if (Logfn)    free(Logfn);
   if (LogSpec)  free(LogSpec);
   if (CBhost)   free(CBhost);
   if (CopyOpts) free(CopyOpts);
   if (SecToken) free(SecToken);
   if (MyHost)   free(MyHost);
   if (MyUser)   free(MyUser);
   if (MyProg)   free(MyProg);
   if (CKPdir)   free(CKPdir);
}
/******************************************************************************/
/*                             A r g u m e n t s                              */
/******************************************************************************/

#define Add_Opt(x) {cbp[0]=' '; cbp[1]='-'; cbp[2]=x; cbp[3]='\0'; cbp+=3;}
#define Add_Num(x) {cbp[0]=' '; cbp=n2a(x,&cbp[1]);}
#define Add_Oct(x) {cbp[0]=' '; cbp=n2a(x,&cbp[1],"%o");}
#define Add_Str(x) {cbp[0]=' '; strcpy(&cbp[1], x); cbp+=strlen(x)+1;}

#define bbcp_VALIDOPTS (char *)"-a.B:b:C:c.d:DefFhi:I:kl:L:m:nopP:q:rs:S:t:T:U:vVw:W:x:z"
#define bbcp_SSOPTIONS bbcp_VALIDOPTS "MH:Y:"

#define Hmsg1(a)   {bbcp_Fmsg("Config", a);    help(1);}
#define Hmsg2(a,b) {bbcp_Fmsg("Config", a, b); help(1);}

/******************************************************************************/

void bbcp_Config::Arguments(int argc, char **argv, int cfgfd)
{
   bbcp_FileSpec *lfsp;
   int retc, infiles = 0, notctl = 0, cxbsz = 0;
   char *inFN=0, c, cbhname[MAXHOSTNAMELEN+1];
   bbcp_Args arglist((char *)"bbcp: ");
   int  tval, Uwindow= 0;

// Make sure we have at least one argument
//
   if (argc < 2) Hmsg1("Copy arguments not specified.");
   notctl = (Options & (bbcp_SRC | bbcp_SNK));

// Establish valid options and the source of those options
//
   if (notctl)            arglist.Options(bbcp_SSOPTIONS, STDIN_FILENO);
      else if (cfgfd < 0) arglist.Options(bbcp_VALIDOPTS, argc-1, ++argv);
              else        arglist.Options(bbcp_SSOPTIONS, cfgfd, 1);

// Process the options
//
   while (c=arglist.getopt())
     { switch(c)
       {
       case 'a': Options |= bbcp_APPEND | bbcp_ORDER;
                 if (arglist.argval)
                    {if (CKPdir) free(CKPdir);
                     CKPdir = strdup(arglist.argval);
                    }
                 break;
       case 'B': if (a2sz("window size", arglist.argval,
                         tval, 1024, ((int)1)<<30))
                    Cleanup(1, argv[0], cfgfd);
                 cxbsz = tval;
                 break;
       case 'b': if (a2n("blocking factor",arglist.argval,tval,1,IOV_MAX)) 
                    Cleanup(1, argv[0], cfgfd);
                 Bfact = tval;
                 break;
       case 'c': if (arglist.argval)
                    if (a2n("compression level",arglist.argval,tval,0,9))
                       Cleanup(1, argv[0], cfgfd);
                       else Complvl = tval;
                    else Complvl = 1;
                 if (Complvl) Options |=  bbcp_COMPRESS;
                    else      Options &= ~bbcp_COMPRESS;
                 break;
       case 'C': if (Configure((const char *)arglist.argval)) 
                    Cleanup(1, argv[0], cfgfd);
                 break;
       case 'd': if (SrcBuff) free(SrcBuff);
                 SrcBuff = strdup(arglist.argval);
                 if (Options & (bbcp_SRC | bbcp_SNK))
                    {SrcBase = SrcBuff;
                     SrcBlen = strlen(SrcBase);
                     SrcUser = SrcHost = 0;
                    } else ParseSB();
                 break;
       case 'D': Options |= bbcp_TRACE;
                 break;
       case 'e': Options |= bbcp_MD5CHK;
                 break;
       case 'f': Options |= bbcp_FORCE;
                 break;
       case 'F': Options |= bbcp_NOSPCHK;
                 break;
       case 'h': help(0);
                 break;
       case 'H': if (CBhost) free(CBhost);
                 if ((CBport = HostAndPort("callback",arglist.argval,
                                   cbhname, sizeof(cbhname))) < 0) exit(1);
                 CBhost = strdup(cbhname);
                 break;
       case 'i': if (IDfn) free(IDfn);
                 IDfn = strdup(arglist.argval);
                 break;
       case 'I': if (inFN) free(inFN);
                 inFN = strdup(arglist.argval);
                 break;
       case 'k': Options |= bbcp_KEEP | bbcp_ORDER;
                 break;
       case 'l': if (Logfn) free(Logfn);
                 Logfn = strdup(arglist.argval);
                 break;
       case 'L': if (LogOpts(arglist.argval)) Cleanup(1, argv[0], cfgfd);
                 break;
       case 'm': if (a2o("mode", arglist.argval, tval, 1, 07777)) 
                    Cleanup(1, argv[0], cfgfd);
                 Mode = tval;
                 break;
       case 'M': Options |= bbcp_OUTDIR;
                 break;
       case 'n': Options |= bbcp_NODNS;
                 break;
       case 'o': Options |= bbcp_ORDER;
                 break;
       case 'p': Options |= bbcp_PCOPY;
                 break;
       case 'P': if (a2n("seconds",arglist.argval,tval,BBCP_MINPMONSEC,-1))
                    Cleanup(1, argv[0], cfgfd);
                 Progint = tval;
                 break;
       case 'q': if (a2n("qos",arglist.argval,tval,0, 255))
                    Cleanup(1, argv[0], cfgfd);
                 NetQoS = tval;
                 break;
       case 'r': Options |= bbcp_RECURSE;
                 break;
       case 's': if (a2n("streams", arglist.argval,
                         tval,1, BBCP_MAXSTREAMS)) Cleanup(1, argv[0], cfgfd);
                 Streams = tval;
                 break;
       case 'S': if (SrcXeq) free(SrcXeq);
                 SrcXeq = strdup(arglist.argval);
                 break;
       case 't': if (a2sz("time limit", arglist.argval,
                         tval, 1, ((int)1)<<30)) Cleanup(1, argv[0], cfgfd);
                 TimeLimit = tval;
                 break;
       case 'T': if (SnkXeq) free(SnkXeq);
                 SnkXeq = strdup(arglist.argval);
                 break;
       case 'v': Options |= bbcp_VERBOSE;
                 break;
       case 'V': Options |= bbcp_BLAB;
                 break;
       case 'U': if (a2sz("forced window size", arglist.argval,
                         tval, 1024, ((int)1)<<30)) Cleanup(1, argv[0], cfgfd);
                 Wsize = MaxWindow = tval;
                 Uwindow = 1;
                 break;
       case 'w': if (a2sz("window size", arglist.argval,
                         tval, 1024, ((int)1)<<30)) Cleanup(1, argv[0], cfgfd);
                 Wsize = tval + Hsize;
                 break;
       case 'W': if (a2sz("window size", arglist.argval,
                         tval, 1024, ((int)1)<<30)) Cleanup(1, argv[0], cfgfd);
                 Wsize = tval;
                 break;
       case 'x': if (a2sz("transfer rate", arglist.argval,
                         tval, 1024, ((int)1)<<30)) Cleanup(1, argv[0], cfgfd);
                 Xrate = tval;
                 break;
       case 'Y': if (SecToken) free(SecToken);
                 SecToken = strdup(arglist.argval);
                 break;
       case 'z': Options |= bbcp_CON2SRC;
                 break;
       case '-': break;
       default:  if (cfgfd < 0) help(255);
                 Cleanup(1, argv[0], cfgfd);
       }
     }

// Set final option values
//
   if (cxbsz && cxbsz != CXBsz) CXBsz = cxbsz;
      else cxbsz = 0;

// Get maximum window size if not specified
//
   if (MaxWindow < 0) MaxWindow = WAdjust(bbcp_Net.MaxWSize()-Hsize);

// If we were processing the configuration file, return
//
   if (!notctl && cfgfd >= 0) return;

// Check for mutually exclsuive options
//
   if ((Options & bbcp_APPEND) && (Options & bbcp_FORCE))
      {bbcp_Fmsg("Config", "-a and -f are mutually exclusive.");
       Cleanup(1, argv[0], cfgfd);
      }

// Get all of the filenames in the input file list
//
   if (inFN)
      {int infd;
       bbcp_Stream inStream;
       char *lp;
       if ((infd = open((const char *)inFN, O_RDONLY)) < 0)
          {bbcp_Emsg("Config", errno, "opening", inFN); exit(2);}
       inStream.Attach(infd);
       while ((lp = inStream.GetLine()) && (*lp != '\0'))
             {lfsp = srcLast;
              srcLast = new bbcp_FileSpec;
              if (lfsp) lfsp->next = srcLast;
                 else srcSpec = srcLast;
              srcLast->Parse(lp);
              if (srcLast->username && !srcLast->hostname)
                  Hmsg2("Missing host name for user", srcLast->username);
              if (srcLast->hostname && !srcLast->pathname)
                  Hmsg1("Piped I/O incompatible with hostname specification.");
              infiles++;
             }
       inStream.Close();
       free(inFN);
      }

// Get all of the file names
//
   while(arglist.getarg())
        {lfsp = srcLast;
         srcLast = new bbcp_FileSpec;
         if (lfsp) lfsp->next = srcLast;
            else srcSpec = srcLast;
         srcLast->Parse(arglist.argval);
         if (srcLast->username && !srcLast->hostname)
            Hmsg2("Missing host name for user", srcLast->username);
         if (srcLast->hostname && !srcLast->pathname)
            Hmsg1("Piped I/O incompatible with hostname specification.");
         infiles++;
        }

// Perform sanity checks based on who we are
//
        if (Options & bbcp_SRC)
           {if (!srcSpec)
               {bbcp_Fmsg("Config", "Source file not specified."); exit(3);}
           }
   else if (Options & bbcp_SNK)
           {if (infiles > 1)
               {bbcp_Fmsg("Config", "Ambiguous sink data target."); exit(3);}
            if (!(snkSpec = srcSpec))
               {bbcp_Fmsg("Config", "Target file not specified."); exit(3);}
            srcSpec = srcLast = 0;
           }
   else    {     if (infiles == 0) Hmsg1("Copy source not specified.")
            else if (infiles == 1) Hmsg1("Copy target not specified.")
            else if (infiles >  2) Options |= bbcp_OUTDIR;
            snkSpec  = srcLast;
            lfsp->next = 0;
            srcLast = lfsp;
           }
// Start up the logging if we need to
//
   if (Options & bbcp_LOGGING && notctl && !bbcp_NetLog.Open("bbcp", Logurl))
      {bbcp_Fmsg("Config", "Unable to initialize netlogger.");
       exit(4);
      }

// Check if we have exceeded memory limitations
//
   if (Options & (bbcp_SRC | bbcp_SNK))
      {long long memreq;
       BNum = (Streams > Bfact ? Streams : Bfact) * 2 + Streams;
       memreq = Wsize * BNum + (Options & bbcp_COMPRESS ? Bfact * CXBsz : 0);
       if (memreq > bbcp_OS.FreeRAM/4)
          {char mbuff[80];
           sprintf(mbuff, "I/O buffers (%" FMTLL "dK) > 25%% of available free memory (%" FMTLL "dK);",
                          memreq/1024, bbcp_OS.FreeRAM/1024);
           bbcp_Fmsg("Config", (Options & bbcp_SRC ? "Source" : "Sink"), mbuff,
                               (char *)"copy may be slow");
          }
       if (Wsize < MaxWindow) MaxWindow = Wsize;
          else if (Wsize > MaxWindow) 
                  {Wsize = MaxWindow;
                   if (Options & (bbcp_BLAB | bbcp_VERBOSE)
                   &&  Options & bbcp_SRC) WAMsg("Config", "reduced", Wsize);
                  }
       }  else Wsize = WAdjust(Wsize, "Config", 
                               Options & (bbcp_BLAB | bbcp_VERBOSE));

// Assemble the final options
//
   if (!notctl)
  {
   char cbuff[4096], *cbp = cbuff;
   if (Options & bbcp_APPEND)    Add_Opt('a');
   if (CKPdir)                                 Add_Str(CKPdir);
   if (Bfact)                   {Add_Opt('b'); Add_Num(Bfact);}
   if (Options & bbcp_COMPRESS) {Add_Opt('c'); Add_Num(Complvl);
                                 if (cxbsz) {Add_Opt('B'); Add_Num(cxbsz);}
                                }
   if (SrcBase)                 {Add_Opt('d'); Add_Str(SrcBase);}
   if (Options & bbcp_TRACE)     Add_Opt('D');
   if (Options & bbcp_FORCE)     Add_Opt('f');
   if (Options & bbcp_NOSPCHK)   Add_Opt('F');
   if (Options & bbcp_KEEP)      Add_Opt('k');
   if (LogSpec)                 {Add_Opt('L'); Add_Str(LogSpec);}
                                 Add_Opt('m'); Add_Oct(Mode);
   if (Options & bbcp_OUTDIR || (Options & bbcp_RELATIVE && SrcBase))
                                 Add_Opt('M');
// if (Options & bbcp_NODNS)     Add_Opt('n');
   if (Options & bbcp_ORDER)     Add_Opt('o');
   if (Options & bbcp_PCOPY)     Add_Opt('p');
   if (Progint)                 {Add_Opt('P'); Add_Num(Progint);}
   if (NetQoS)                  {Add_Opt('q'); Add_Num(NetQoS); }
   if (Options & bbcp_RECURSE)   Add_Opt('r');
   if (Streams)                 {Add_Opt('s'); Add_Num(Streams);}
   if (TimeLimit)               {Add_Opt('t'); Add_Num(TimeLimit);}
   if (Options & bbcp_VERBOSE)   Add_Opt('v');
   if (Options & bbcp_BLAB)      Add_Opt('V');
   if (Uwindow)                 {Add_Opt('U'); Add_Num(MaxWindow);}
   else
   if (Wsize)                   {Add_Opt('W'); Add_Num(Wsize);}
                                 Add_Opt('Y'); Add_Str(SecToken);
   if (Xrate)                   {Add_Opt('x'); Add_Num(Xrate);}
   if (Options & bbcp_CON2SRC)   Add_Opt('z');
   CopyOpts = strdup(cbuff);
  }
   if (!Bfact)
      if (Options & bbcp_COMPRESS) Bfact = 3;
         else Bfact = (Streams > IOV_MAX ? IOV_MAX : Streams);

// Set appropriate debugging level
//
   if (Options & bbcp_TRACE)
      if (Options & bbcp_BLAB) bbcp_Debug.Trace = 3;
         else if (Options & bbcp_VERBOSE) bbcp_Debug.Trace = 2;
                 else bbcp_Debug.Trace = 1;

// Set checkpoint directory if we do not have one here and must have one
//
   if (!CKPdir && Options & bbcp_APPEND && Options & bbcp_SNK)
      {const char *ckpsfx = "/.bbcp";
       char *homedir = bbcp_OS.getHomeDir();
       CKPdir = (char *)malloc(strlen(homedir) + sizeof(ckpsfx) + 1);
       strcpy(CKPdir, homedir); strcat(CKPdir, ckpsfx);
       if (mkdir((const char *)CKPdir, 0644) && errno != EEXIST)
          {bbcp_Emsg("Config",errno,"creating restart directory",CKPdir);
           exit(100);
          }
       DEBUG("Restart directory is " <<CKPdir);
      }

// Compute proper MT level
//
   MTLevel = Streams + (Complvl ?  (Options & bbcp_SNK  ? 3 : 2) : 1)
                     + (Progint && (Options & bbcp_SNK) ? 1 : 0) + 2;
   DEBUG("Optimum MT level is " <<MTLevel);

// All done
//
   return;
}

/******************************************************************************/
/*                                  h e l p                                   */
/******************************************************************************/

#define H(x)    cerr <<x <<endl;
#define I(x)    cerr <<'\n' <<x <<endl;

/******************************************************************************/
  
void bbcp_Config::help(int rc)
{
H("Usage:   bbcp [Options] [Inspec] Outspec")
I("Options: [-a [dir]] [-b bf] [-B bsz] [-c [lvl]] [-C cfn] [-D] [-d path] [-e]")
H("         [-f] [-F] [-h] [-i fn] [-I fn] [-k] [-L opts[@logurl]] [-m mode] [-p]")
H("         [-P sec] [-r] [-q qos] [-s snum] [-S srcxeq] [-T trgxeq] [-t sec]")
H("         [-v] [-V] [-U wsz] [-w wsz] [-W wsz] [-x rate] [-z] [--]")
I("I/Ospec: [user@][host:]file")
if (rc) exit(rc);
I("Function: Secure and fast copy utility.")
I("-a dir  append mode to restart a previously failed copy.")
H("-b bf   sets the read blocking factor (default is 1).")
H("-B bsz  sets the read/write compression I/O buffer size (default is 1M).")
H("-c lvl  compress data before sending across the network (default lvl is 1).")
H("-C cfn  process the named configuration file at time of encounter.")
H("-d path requests relative source path addressing and target path creation.")
H("-D      turns on debugging.")
H("-e      error check the data using md5 checksums.")
H("-f      forces the copy by first unlinking the target file before copying.")
H("-F      does not check to see if there is enough space on the target node.")
H("-h      print help information.")
H("-i fn   is the name of the ssh identify file for source and target.")
H("-I fn   is the name of the file that holds the list of files to be copied.")
H("-k      keep the destination file even when the copy fails.")
H("-l      logs standard error to the specified file.")
H("-L args sets the logginng level and log message destination.")
H("-m mode target file mode (default is 0644 or comes via -p option).")
H("-s snum number of network streams to use (default is 4).")
H("-p      preserve source mode, ownership, and dates.")
H("-P sec  produce a progress message every sec seconds.")
H("-q lvl  specifies the quality of service for routers that support QOS.")
H("-r      copy subdirectories and their contents.")
H("-S cmd  command to start bbcp on the source node.")
H("-T cmd  command to start bbcp on the target node.")
H("-t sec  sets the time limit for the copy to complete.")
H("-v      verbose mode (provides per file transfer rate).")
H("-V      very verbose mode (excruciating detail).")
H("-U wsz  unnegotiated window size (sets maximum and default for all parties).")
H("-w wsz  window size for transmission (the default is 64K).")
H("-w wsz  same as -w but uses the precise value even when non-optimum.")
H("-x rate is the maximum transfer rate allowed in bytes, K, M, or G.")
H("-z      use reverse connection protocol (i.e., sink to source).")
H("--      allows an option with a defaulted optional arg to appear last.")
I("user    the user under which the copy is to be performed. The default is")
H("        to use the current login name.")
H("host    the location of the file. The default is to use the current host.")
H("file    the name of the source file, or target file or directory.")
I("Note 1. If -i is specified, no source files need be specified on the command")
H("        line. At least one source file must be specified in some way.")
I("     2. The -a and -f options are mutually exclusive.")
I("     3. The smallest progress value allowed is 15 seconds.")
I("     4. By default, -b (blocking) is set to equal the -s (streams) value.")
I("     5. " <<bbcp_Version.Version)
exit(rc);
}
  
/******************************************************************************/
/*                            C o n f i g I n i t                             */
/******************************************************************************/
  
int bbcp_Config::ConfigInit(int argc, char **argv)
{
/*
  Function: Establish default values using a configuration file.

  Input:    None.

  Output:   0 upon success or !0 otherwise.
*/
   char *homedir, *cfn = (char *)"/.bbcp.cf";
   int  retc;
   char *ConfigFN;

// Make sure we have at least one argument to determine who we are
//
   if (argc >= 2)
           if (!strcmp(argv[1], SrcArg))
            {Options |= bbcp_SRC; bbcp_Debug.Who = (char *)"SRC"; return 0;}
      else if (!strcmp(argv[1], SnkArg))
            {Options |= bbcp_SNK; bbcp_Debug.Who = (char *)"SNK"; return 0;}
      else MyProg = strdup(argv[0]);

// Use the config file, if present
//
   if (ConfigFN = getenv("bbcp_CONFIGFN"))
      {if (retc = Configure((const char *)ConfigFN)) return retc;}
      else {
      // Use configuration file in the home directory, if any
      //
      struct stat buf;
      homedir = bbcp_OS.getHomeDir();
      ConfigFN = (char *)malloc(strlen(homedir) + strlen(cfn) + 1);
      strcpy(ConfigFN, homedir); strcat(ConfigFN, cfn);
      if (stat((const char *)ConfigFN, &buf)) 
         {retc = 0; free(ConfigFN); ConfigFN = 0;}
         else if (retc = Configure((const char *)ConfigFN)) return retc;
     }

// Establish the FD limit
//
   {struct rlimit rlim;
    if (getrlimit(RLIMIT_NOFILE, &rlim) < 0)
       bbcp_Emsg("Config",-errno,"getting FD limit");
       else {rlim.rlim_cur = rlim.rlim_max;
             if (setrlimit(RLIMIT_NOFILE, &rlim) < 0)
                 bbcp_Emsg("config", errno, "setting FD limit");
            }
   }
   return retc;
}
  
/******************************************************************************/
/*                             C o n f i g u r e                              */
/******************************************************************************/
  
int bbcp_Config::Configure(const char *cfgFN)
{
/*
  Function: Establish default values using a configuration file.

  Input:    None.

  Output:   0 upon success or !0 otherwise.
*/
   static int depth = 3;
          int  cfgFD;

// Check if we are recursing too much
//
   if (!(depth--))
      return bbcp_Fmsg("Config", "Too many embedded configuration files.");

// Try to open the configuration file.
//
   if ( (cfgFD = open(cfgFN, O_RDONLY, 0)) < 0)
      return bbcp_Emsg("Config",errno,"opening config file",(char *)cfgFN);

// Process the arguments from the config file
//
   Arguments(2, (char **)&cfgFN, cfgFD);
   return 0;
}

/******************************************************************************/
/*                               D i s p l a y                                */
/******************************************************************************/
  
void bbcp_Config::Display()
{

// Display the option values (used during debugging)
//
   cerr <<"accwait " <<accwait <<endl;
   cerr <<"bfact " <<Bfact <<endl;
   cerr <<"bindtries " <<bindtries <<" bindwait " <<bindwait <<endl;
   if (CBhost)
   cerr <<"cbhost:port " <<CBhost <<":" <<CBport <<endl;
   if (CKPdir)
   cerr <<"ckpdir " <<CKPdir <<endl;
   if (Complvl)
   cerr <<"compress " <<Complvl <<endl;
   if (CopyOpts)
   cerr <<"copyopts " <<CopyOpts <<endl;
   if (IDfn)
   cerr <<"idfn " <<IDfn <<endl;
   if (Logfn)
   cerr <<"logfn " <<Logfn <<endl;
   cerr <<"min/maxport " <<MinPort <<" / " <<MaxPort <<endl;
   cerr <<"mode " <<oct <<Mode <<dec <<endl;
   cerr <<"myhost " <<MyHost <<endl;
   cerr <<"myuser " <<MyUser <<endl;
   cerr <<"ssectoken " <<SecToken <<endl;
   cerr <<"streams " <<Streams <<endl;
   cerr <<"wsize " <<Wsize <<" maxwindow " <<MaxWindow  <<" maxss " <<MaxSegSz <<endl;
   cerr <<"srcxeq " <<SrcXeq <<endl;
   cerr <<"trgxeq " <<SnkXeq <<endl;
   if (Xrate)
   cerr <<"xfr " <<Xrate <<endl;
}
  
/******************************************************************************/
/*                               W A d j u s t                                */
/******************************************************************************/
  
int bbcp_Config::WAdjust(int mywsize, const char *who, int blab)
{
    int wmult, newsz;
    char buff[64];

    if (mywsize > 131072) wmult = 65536;
       else if (mywsize > 65536) wmult = 32768;
               else if (mywsize > 32768) wmult = 16384;
                       else if (mywsize > 16384) wmult = 8192;
                               else return mywsize;
   if ((newsz = mywsize / wmult * wmult + Hsize) != mywsize && blab)
      WAMsg(who, "adjusted", newsz);

   return newsz;
}
 
/******************************************************************************/
/*                                 W A M s g                                  */
/******************************************************************************/
  
void bbcp_Config::WAMsg(const char *who, const char *act, int newsz)
{
    char buff[128];
    sprintf(buff, "%s to %d", act, newsz);
    bbcp_Fmsg(who, "Window size", buff, (char *)"bytes.");
}

/******************************************************************************/
/*                     p r i v a t e   f u n c t i o n s                      */
/******************************************************************************/
/******************************************************************************/
/*                                R t o k e n                                 */
/******************************************************************************/
  
char *bbcp_Config::Rtoken()
{
   int mynum = (int)getpid();
   struct timeval tod;
   char mybuff[sizeof(mynum)*2+1];

// Get current time of day and add into the random equation
//
   gettimeofday(&tod, 0);
   mynum = mynum ^ tod.tv_sec ^ tod.tv_usec ^ (tod.tv_usec<<((mynum & 0x0f)+1));

// Convert to a printable string
//
   tohex((char *)&mynum, sizeof(mynum), mybuff);
   return strdup(mybuff);
}

/******************************************************************************/
/*                                  a 2 x x                                   */
/******************************************************************************/

int bbcp_Config::a2sz(const char *etxt, char *item, int  &result,
                      int  minv, int  maxv)
{   int i = strlen(item)-1; char cmult = item[i];
    int  val, qmult = 1;
    char *endC;
         if (cmult == 'k' || cmult == 'K') qmult = 1024;
    else if (cmult == 'm' || cmult == 'M') qmult = 1024*1024;
    else if (cmult == 'g' || cmult == 'G') qmult = 1024*1024*1024;
    if (qmult > 1) item[i] = '\0';
    val  = strtol(item, &endC, 10) * qmult;
    if (*endC || (maxv != -1 && (val > maxv || val < 1)) || val < minv)
       {if (qmult > 1) item[i] = cmult;
        return bbcp_Fmsg("Config", "Invalid", (char *)etxt, (char *)" - ",item);
       }
    if (qmult > 1) item[i] = cmult;
    result = val;
    return 0;
}

int bbcp_Config::a2tm(const char *etxt, char *item, int  &result,
                      int  minv, int  maxv)
{   int i = strlen(item)-1; char cmult = item[i];
    int  val, qmult = 1;
    char *endC;
         if (cmult == 's' || cmult == 'S') qmult =  1;
    else if (cmult == 'm' || cmult == 'M') qmult = 60;
    else if (cmult == 'h' || cmult == 'H') qmult = 60*60;
    if (qmult > 1) item[i] = '\0';
    val  = strtol(item, &endC, 10) * qmult;
    if (*endC || (maxv != -1 && (val > maxv || val < 1)) || val < minv)
       {if (qmult > 1) item[i] = cmult;
        return bbcp_Fmsg("Config", "Invalid", (char *)etxt, (char *)" - ",item);
       }
    if (qmult > 1) item[i] = cmult;
    result = val;
    return 0;
}

int bbcp_Config::a2ll(const char *etxt, char *item, long long &result,
                      long long minv, long long maxv)
{   long long val;
    char *endC;
    val  = strtoll(item, &endC, 10);
    if (*endC || (maxv != -1 && val > maxv) || val  < minv)
       return bbcp_Fmsg("Config", "Invalid", (char *)etxt, (char *)" - ", item);
    result = val;
    return 0;
}

int bbcp_Config::a2n(const char *etxt, char *item, int  &result,
                     int  minv, int  maxv)
{   int  val;
    char *endC;
    val  = strtol(item, &endC, 10);
    if (*endC || (maxv != -1 && val > maxv) || val  < minv)
       return bbcp_Fmsg("Config", "Invalid", (char *)etxt, (char *)" - ", item);
    result = val;
    return 0;
}

int bbcp_Config::a2o(const char *etxt, char *item, int  &result,
                     int  minv, int  maxv)
{   int  val;
    char *endC;
    val  = strtol(item, &endC, 8);
    if (*endC || (maxv != -1 && val > maxv) || val < minv)
       return bbcp_Fmsg("Config", "Invalid", (char *)etxt, (char *)" - ", item);
    result = val;
    return 0;
}

/******************************************************************************/
/*                                   n 2 a                                    */
/******************************************************************************/

char *bbcp_Config::n2a(int  val, char *buff, const char *fmt)
      {return buff + sprintf(buff, fmt, val);}

char *bbcp_Config::n2a(long long  val, char *buff, const char *fmt)
      {return buff + sprintf(buff, fmt, val);}
 
/******************************************************************************/
/*                               P a r s e S B                                */
/******************************************************************************/
  
void bbcp_Config::ParseSB()
{
   char *up = 0, *hp = 0, *cp, *sp;
   int i;

// Prepare to parse the spec
//
   sp = cp = SrcBuff;
   while(*cp)
      {     if (*cp == '@' && !SrcUser)
               {SrcUser = sp; *cp = '\0'; sp = cp+1;}
       else if (*cp == ':' && !SrcHost)
               {SrcHost = sp; *cp = '\0'; sp = cp+1;}
       cp++;
      }
   SrcBase = sp;

// Ignore null users and hosts
//
   if (SrcUser && !(*SrcUser)) SrcUser = 0;
   if (SrcHost && !(*SrcHost)) SrcHost = 0;
}
  
/******************************************************************************/
/*                             g e t P r o t I D                              */
/******************************************************************************/

int bbcp_Config::getProtID(const char *pname)
{
    struct protoent *pp;

    if (!(pp = getprotobyname(pname))) return BBCP_IPPROTO_TCP;
       else return pp->p_proto;
}
  
/******************************************************************************/
/*                               L o g O p t s                                */
/******************************************************************************/
  
int bbcp_Config::LogOpts(char *opts)
{
    char *opt = opts;
    int myopts = 0, nlopts = 0;

    while(*opt && *opt != '@')
         {switch(*opt)
                {case 'a': nlopts |= NL_APPEND;    break;
                 case 'b': nlopts |= NL_MEM;       break;
                 case 'c': myopts |= bbcp_LOGCMP;  break;
                 case 'i': myopts |= bbcp_LOGIN;   break;
                 case 'o': myopts |= bbcp_LOGOUT;  break;
                 case 'r': myopts |= bbcp_LOGRD;   break;
                 case 'w': myopts |= bbcp_LOGWR;   break;
                 case 'x': myopts |= bbcp_LOGEXP;  break;
                 case '@':                         break;
                 default:  bbcp_Fmsg("Config","Invalid logging options -",opts);
                           return -1;
                }
          opt++;
         }

    if (!myopts) {bbcp_Fmsg("Config", "no logging events selected.");
                  return -1;
                 }

    Options &= ~bbcp_LOGGING;
    Options |= myopts;
    bbcp_NetLog.setOpts(nlopts);
    if (Logurl) {free(Logurl); Logurl = 0;}
    LogSpec = strdup(opts);
    if (*opt == '@' && opt[1]) {opt++; Logurl = strdup(opt);}

    return 0;
}

/******************************************************************************/
/*                          H o s t A n d   P o r t                           */
/******************************************************************************/
  
int bbcp_Config::HostAndPort(const char *what, char *path, char *buff, int bsz)
{   int hlen, pnum = 0;
    char *hn = path;

 // Extract the host name from the path
 //
    while (*hn && *hn != ':') hn++;
    if (!(hlen = hn - path))
       {bbcp_Fmsg("Config", what, (char *)"host not specified in", path); 
        return -1;
       }
    if (hlen >= bsz)
       {bbcp_Fmsg("Config", what, (char *)"hostname too long in", path);
        return -1;
       }
    if (path != buff) strncpy(buff, path, hlen);

 // Extract the port number from the path
 //
    if (*hn == ':' && hn++ && *hn)
       {errno = 0;
        pnum = strtol((const char *)hn, (char **)NULL, 10);
        if (!pnum && errno || pnum > 65535)
           {bbcp_Fmsg("Config",what,(char *)"port invalid -", hn); return -1;}
       }

// All done.
//
    buff[hlen] = '\0';
    return pnum;
}

/******************************************************************************/
/*                                 t o h e x                                  */
/******************************************************************************/

char *bbcp_Config::tohex(char *inbuff, int inlen, char *outbuff) {
     static char hv[] = "0123456789abcdef";
     unsigned int hnum, i, j = 0;
     for (i = 0; i < inlen; i++) {
         outbuff[j++] = hv[(inbuff[i] >> 4) & 0x0f];
         outbuff[j++] = hv[ inbuff[i]       & 0x0f];
         }
     outbuff[j] = '\0';
     return outbuff;
     }

void bbcp_Config::Cleanup(int rc, char *cfgfn, int cfgfd)
{
if (cfgfd >= 0 && cfgfn)
   if (rc > 1)
        bbcp_Fmsg("Config","Check config file",cfgfn,(char *)"for conflicts.");
   else bbcp_Fmsg("Config", "Error occured processing config file", cfgfn);
exit(rc);
}
