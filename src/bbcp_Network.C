/******************************************************************************/
/*                                                                            */
/*                        b b c p _ N e t w o r k . C                         */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/

// Contributions:

// 15-Oct-02: Corrections to findPort() contributed by Thomas Schenk @ ea.com
  
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "bbcp_Config.h"
#include "bbcp_Emsg.h"
#include "bbcp_Network.h"
#include "bbcp_Platform.h"

/******************************************************************************/
/*                      E x t e r n a l   O b j e c t s                       */
/******************************************************************************/
  
extern bbcp_Config  bbcp_Config;

       bbcp_Network bbcp_Net;

/******************************************************************************/
/*                                A c c e p t                                 */
/******************************************************************************/

bbcp_Link *bbcp_Network::Accept()
{
   char *newfn;
   int newfd, retc;
   bbcp_Link *newconn;
   struct pollfd sfd[1];

// Make sure we are bound to a port
//
   if (iofd < 0) {bbcp_Fmsg("Accept", "Network not bound to a port.");
                  return (bbcp_Link *)0;
                 }

// Setup up the poll structure to wait for new connections
//
   do {if (bbcp_Config.accwait >= 0)
          {sfd[0].fd     = iofd;
           sfd[0].events = POLLIN|POLLRDNORM|POLLRDBAND|POLLPRI|POLLHUP;
           sfd[0].revents = 0;
           do {retc = poll(sfd, 1, bbcp_Config.accwait);}
                      while(retc < 0 && (errno == EAGAIN || errno == EINTR));
           if (!sfd[0].revents)
              {char buff[16];
               sprintf(buff,"%d", Portnum);
               bbcp_Fmsg("Accept", "Accept timed out on port", buff);
               return (bbcp_Link *)0;
              }
          }

       do {newfd = accept(iofd, (struct sockaddr *)0, (size_t)0);}
          while(newfd < 0 && errno == EINTR);

       if (newfd < 0)
          {char buff[16];
           sprintf(buff,"%d", Portnum);
           bbcp_Emsg("Accept", errno, "performing accept on port", buff);
           continue;
          }

    // Get the peer name and set as the pathname
    //
       if (!(newfn = Peername(newfd)))
          {bbcp_Fmsg("Accept", "Unable to determine peer name.");
           close(newfd); continue;
          }
       setOpts("accept", newfd, bbcp_Config.Wsize);

    // Allocate a new network object
    //
       if (!(newconn = new bbcp_Link(newfd, (const char *)newfn,
                                bbcp_Config.Options & bbcp_MD5CHK)))
          {bbcp_Fmsg("Accept", "Unable to allocate new link.");
           close(newfd); newfd = 0;
          }
    } while(newfd < 0);

// Return new net object
//
   delete newfn;
   return newconn;
}
  
/******************************************************************************/
/*                                  B i n d                                   */
/******************************************************************************/
  
int bbcp_Network::Bind(int minport, int maxport, int tries, int timeout)
{
    struct sockaddr_in InetAddr;
    socklen_t iln = sizeof(InetAddr);
    struct sockaddr *SockAddr = (struct sockaddr *)&InetAddr;
    int SockSize = sizeof(InetAddr);
    char *action;
    int retc, One = 1, btry = 1;
    unsigned short port = minport;

// Close any open socket here
//
   unBind();

// Allocate a socket descriptor and set the options
//
   if ((iofd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
      return bbcp_Emsg("Bind", -errno, "creating inet socket");
   setOpts("bind", iofd, bbcp_Config.Wsize);

// Attempt to do a bind
//
   action = (char *)"binding";
   do {if (port > maxport && timeout > 0) sleep(timeout);
       port = minport;
       do {
           InetAddr.sin_family = AF_INET;
           InetAddr.sin_addr.s_addr = INADDR_ANY;
           InetAddr.sin_port = htons(port);
           if (!(retc = bind(iofd, SockAddr, SockSize))) break;
          } while(++port <= maxport);
      } while(++btry <= tries);

// If we bound, put up a listen
//
   if (!retc)
      {action = (char *)"listening on";
       retc = listen(iofd, 8);
      }

// Check for any errors and return.
//
   if (retc) 
      {if (timeout < 0) return -1;
       return bbcp_Emsg("Bind",-errno,(const char *)action,(char *)"socket");
      }

// Return the port number being used
//
   Portnum = port;
   if (port) return (int)port;
   if (getsockname(iofd, SockAddr, &iln) < 0)
      return bbcp_Emsg("Bind",-errno, "determing bound port number.");
   Portnum = static_cast<int>(ntohs(InetAddr.sin_port));
   return Portnum;
}

/******************************************************************************/
/*                               C o n n e c t                                */
/******************************************************************************/

bbcp_Link *bbcp_Network::Connect(char *host, int port, int  wsize,
                                 int retries, int rwait)
{
    struct sockaddr_in InetAddr;
    struct sockaddr *SockAddr = (struct sockaddr *)&InetAddr;
    int SockSize = sizeof(InetAddr);
    int newfd, retc;
    bbcp_Link *newlink;

// Determine the host address
//
   if (getHostAddr(host, InetAddr))
      {bbcp_Emsg("Connect", EHOSTUNREACH, "unable to find", host);
       return (bbcp_Link *)0;
      }
   InetAddr.sin_port = htons(port);

// Allocate a socket descriptor
//
   if ((newfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
      {bbcp_Emsg("Connect", errno, "creating inet socket to", host);
       return (bbcp_Link *)0;
      }

// Allocate a new lnk for this socket and set the options
//
   setOpts("connect", newfd, wsize);
   newlink = new bbcp_Link(newfd, host, bbcp_Config.Options & bbcp_MD5CHK);

// Connect to the host
//
   do {retc = connect(newfd, SockAddr, SockSize);}
      while(retc < 0 && (errno == EINTR || 
           (errno == ECONNREFUSED && (retries = Retry(retries, rwait)))));
   if (retc)
      {char buff[16];
       delete newlink;
       sprintf(buff, "port %d", port);
       bbcp_Emsg("Connect", errno, "unable to connect to", host, buff);
       return (bbcp_Link *)0;
      }

// Return the link
//
   return newlink;
}

/******************************************************************************/
/*                              f i n d P o r t                               */
/******************************************************************************/


void bbcp_Network::findPort(int &minport, int &maxport)
{
   struct servent sent, *sp;
   char sbuff[1024];

// Try to find minimum port number
//
   minport = maxport = 0;
   if (GETSERVBYNAME((const char *)bbcp_Config.MinPort, "tcp", &sent,
                     sbuff, sizeof(sbuff), sp))
      {minport = ntohs(sp->s_port);
       if (GETSERVBYNAME((const char *)bbcp_Config.MaxPort, "tcp", &sent,
                         sbuff, sizeof(sbuff), sp)) maxport = ntohs(sp->s_port);
          else maxport = 0;
      }

// Do final port check and return
//
   if (minport > maxport) minport = maxport = 0;
}

/******************************************************************************/
/*                          F u l l H o s t N a m e                           */
/******************************************************************************/

char *bbcp_Network::FullHostName(char *host, int asipadr)
{
   char myname[260], *hp;
   struct sockaddr_in InetAddr;
  
// Identify ourselves if we don't have a passed hostname
//
   if (host) hp = host;
      else if (gethostname(myname, sizeof(myname))) hp = (char *)"";
              else hp = myname;

// Get our address
//
   if (getHostAddr(hp, InetAddr)) return strdup(hp);

// Convert it to a fully qualified host name or IP address
//
   return (asipadr ? strdup(inet_ntoa(InetAddr.sin_addr))
                   : getHostName(InetAddr));
}

/******************************************************************************/
/*                              M a x W S i z e                               */
/******************************************************************************/

int bbcp_Network::MaxWSize()
{
   int sfd, cursz, nowwsz, maxwsz = 4*1024*1024, minwsz = 0;
   socklen_t szcur = (socklen_t)sizeof(cursz);
   socklen_t szseg = (socklen_t)sizeof(maxSegment);
   socklen_t sznow;
       int rcs, rcg;

// Define a socket for discovering the maximum window size
//
   if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
      {bbcp_Emsg("MaxWSize", errno, "creating inet socket."); return 0;}

// Get maximum segment size
//
   if (getsockopt(sfd, IPPROTO_TCP, TCP_MAXSEG, (gsval_t)&maxSegment, &szseg))
      bbcp_Emsg("MaxWSize", errno, "getting TCP maxseg.");

// Calculate the largest allowed quantity
//
   do {sznow = (socklen_t)sizeof(nowwsz);
       maxwsz = maxwsz<<1;
      } while(!(rcs=setsockopt(sfd,SOL_SOCKET,SO_SNDBUF,(ssval_t)&maxwsz, szcur))
           && !     getsockopt(sfd,SOL_SOCKET,SO_SNDBUF,(gsval_t)&nowwsz,&sznow)
           && (maxwsz <= nowwsz) && !(maxwsz & 0x20000000));

// Perform binary search to find the largest buffer allowed
//
   if (!rcs) minwsz = nowwsz;
      else while(maxwsz - minwsz > 1024)
                {cursz = minwsz + (maxwsz - minwsz)/2;
                 if (setsockopt(sfd,SOL_SOCKET,SO_SNDBUF,(ssval_t)&cursz,szcur))
                         maxwsz = cursz;
                    else minwsz = cursz;
                }

// Close the socket and return the buffer size
//
   close(sfd);
   return minwsz;
}

/******************************************************************************/
/*                             g e t W B S i z e                              */
/******************************************************************************/
  
int bbcp_Network::getWBSize(int xfd, int srwant)
{
   int bsz;
   socklen_t szb = (socklen_t)sizeof(bsz);

   if (getsockopt(xfd,SOL_SOCKET,(srwant ? SO_SNDBUF:SO_RCVBUF),&bsz,&szb))
      return -errno;
   return bsz;
}

/******************************************************************************/
/*                             s e t W B S i z e                              */
/******************************************************************************/
  
int bbcp_Network::setWBSize(const char *who, int xfd, int wbsz)
{
   socklen_t szwb = (socklen_t)sizeof(wbsz);
   int rc = 0; int wbsz2 = wbsz;

   if (setsockopt(xfd, SOL_SOCKET, SO_SNDBUF, &wbsz, szwb))
      {bbcp_Emsg(who, errno, "setting sndbuf size"); rc = -1;}
   if (setsockopt(xfd, SOL_SOCKET, SO_RCVBUF, &wbsz2, szwb))
      {bbcp_Emsg(who, errno, "setting rcvbuf size"); rc = -1;}

   if DEBUGON
      {int sndsz, rcvsz;
       socklen_t szsr = (socklen_t)sizeof(sndsz);
       if (getsockopt(xfd, SOL_SOCKET, SO_SNDBUF, &sndsz, &szsr))
          sndsz = -errno;
       if (getsockopt(xfd, SOL_SOCKET, SO_RCVBUF, &rcvsz, &szsr))
          rcvsz = -errno;
       DEBUG(who <<" window set to " <<wbsz <<" (actual snd=" <<sndsz <<" rcv=" <<rcvsz <<" seg=" <<maxSegment <<")");
      }
   return rc;
}
 
/******************************************************************************/
/*                       P r i v a t e   M e t h o d s                        */
/******************************************************************************/
/******************************************************************************/
/*                           g e t H o s t A d d r                            */
/******************************************************************************/
  
int bbcp_Network::getHostAddr(char *hostname, struct sockaddr_in &InetAddr)
{
    unsigned int addr;
    struct hostent hent, *hp = 0;
    char hbuff[1024];
    int rc, byaddr = 0;

// Make sure we have something to lookup here
//
    InetAddr.sin_family = AF_INET;
    if (!hostname || !hostname[0]) 
       {InetAddr.sin_addr.s_addr = INADDR_ANY; return 0;}

// Try to resulve the name
//
    if (hostname[0] > '9' || hostname[0] < '0')
       GETHOSTBYNAME((const char *)hostname,&hent,hbuff,sizeof(hbuff),hp,&rc);
       else if ((int)(addr = inet_addr(hostname)) == -1) errno = EINVAL;
               else {GETHOSTBYADDR((const char *)&addr,sizeof(addr),AF_INET,
                                   &hent, hbuff, sizeof(hbuff), hp, &rc);
                     byaddr = 1;
                    }

// Check if we resolved the name
//
   if (hp) memcpy((void *)&InetAddr.sin_addr, (const void *)hp->h_addr,
                 hp->h_length);
      else if (byaddr) memcpy((void *)&InetAddr.sin_addr, (const void *)&addr,
                 sizeof(addr));
              else return bbcp_Emsg("getHostAddr", errno,
                                    "obtaining address for", hostname);
   return 0;
}

/******************************************************************************/
/*                           g e t H o s t N a m e                            */
/******************************************************************************/

char *bbcp_Network::getHostName(struct sockaddr_in &addr)
{
   struct hostent hent, *hp;
   char *hname, hbuff[1024];
   int rc;

// Convert it to a host name
//
   if (GETHOSTBYADDR((const char *)&addr.sin_addr, sizeof(addr.sin_addr),
                     AF_INET, &hent, hbuff, sizeof(hbuff), hp, &rc))
             hname = strdup(hp->h_name);
        else hname = strdup(inet_ntoa(addr.sin_addr));

// Return the name
//
   return hname;
}
  
/******************************************************************************/
/*                              P e e r n a m e                               */
/******************************************************************************/
  
char *bbcp_Network::Peername(int snum)
{
      struct sockaddr_in addr;
      socklen_t addrlen = sizeof(addr);
      char *hname;

// Get the address on the other side of this socket
//
   if (getpeername(snum, (struct sockaddr *)&addr, &addrlen) < 0)
      {bbcp_Emsg("Peername", errno, "obtaining peer name.");
       return (char *)0;
      }

// Convert it to a host name
//
   return getHostName(addr);
}

/******************************************************************************/
/*                                 R e t r y                                  */
/******************************************************************************/
  
int bbcp_Network::Retry(int retries, int rwait)
{

// Sleep if retry allowed
//
   if ((retries -= 1) >= 0) sleep(rwait);
   return retries;
}

/******************************************************************************/
/*                               s e t O p t s                                */
/******************************************************************************/

int bbcp_Network::setOpts(const char *who, int xfd, int wbsz)
{
   int one = 1, rc1, rc2, rc3, rc4, rc5;
// struct linger liopts;
// socklen_t szlio = (socklen_t)sizeof(liopts);
   socklen_t szone = (socklen_t)sizeof(one);

   if (rc1 = setsockopt(xfd,SOL_SOCKET,SO_REUSEADDR,(ssval_t)&one,szone))
      bbcp_Emsg(who, errno, "setting REUSEADDR");

// SO_LINGER is busted on most systems, so don't bother setting it since it
// will simply cause the close to delay much longer than necessary.
//
// liopts.l_onoff = 1; liopts.l_linger = 1;
// if (rc2 = setsockopt(xfd,SOL_SOCKET,SO_LINGER,(ssval_t)&liopts,szlio))
//    bbcp_Emsg(who, errno, "setting LINGER");

   if (rc3=setsockopt(xfd,bbcp_Config.TCPprotid,TCP_NODELAY,(ssval_t)&one,szone))
      bbcp_Emsg(who, errno, "setting NODELAY");

   if (!bbcp_Config.NetQoS) rc4 = 0;
      else if (rc4=setsockopt(xfd, IPPROTO_IP, IP_TOS,
              (ssval_t)&bbcp_Config.NetQoS, sizeof(bbcp_Config.NetQoS)))
      bbcp_Emsg(who, errno, "setting IP_TOS");

   if (wbsz) rc5 = setWBSize(who, xfd, wbsz);
      else   rc5 = 0;

   return rc1 |       rc3 | rc4 | rc5;
}
  
/******************************************************************************/
/*                              s e t S e g S z                               */
/******************************************************************************/

int bbcp_Network::setSegSz(const char *who, int xfd)
{
    int       segsz = 65535;
    socklen_t szseg = (socklen_t)sizeof(maxSegment);

    if (setsockopt(xfd, IPPROTO_TCP, TCP_MAXSEG, (ssval_t)&segsz, szseg)
       && !(errno == ENOPROTOOPT || errno == EINVAL))
       return bbcp_Emsg(who, -errno, "setting TCP maxseg.");
    return 0;
}
