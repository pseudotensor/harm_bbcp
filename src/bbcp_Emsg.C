/******************************************************************************/
/*                                                                            */
/*                           b b c p _ E m s g . C                            */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/
  
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "bbcp_Platform.h"
#include "bbcp_Headers.h"
#include "bbcp_Debug.h"

/******************************************************************************/

int bbcp_Emsg(const char *sfx, int ecode, const char *txt1, 
                                    char *txt2, char *txt3)
{
    int xcode;
    char ebuff[16], *etxt;

    xcode = (ecode < 0 ? -ecode : ecode);
    etxt = (char *)strerror(xcode);
    if (!strncmp((const char *)etxt, "Unknown", 7))
       {snprintf(ebuff, sizeof(ebuff), "Error %d", ecode);
        etxt = ebuff;
       }

    if (sfx && bbcp_Debug.Trace) 
             cerr <<"bbcp_" <<bbcp_Debug.Who <<'.' <<sfx <<": " <<etxt;
       else  cerr <<"bbcp: " <<etxt;
    if (*txt1 == ';') cerr << txt1;
       else cerr <<' ' <<txt1;
    if (txt2) cerr <<' ' <<txt2;
    if (txt3) cerr <<' ' <<txt3 <<endl;
       else   cerr << endl;
    return (ecode ? ecode : -1);
}

/******************************************************************************/

int bbcp_Fmsg(const char *sfx, const char *txt1, char *txt2, char *txt3,
                                     char *txt4, char *txt5, char *txt6)
{
    if (sfx && bbcp_Debug.Trace) 
             cerr <<"bbcp_" <<bbcp_Debug.Who <<'.' <<sfx <<": " <<txt1;
       else  cerr <<"bbcp: " <<txt1;
    if (txt2) {cerr <<' ' <<txt2;
               if (txt3) {cerr <<' ' <<txt3;
                          if (txt4) {cerr <<' ' <<txt4;
                                     if (txt5) {cerr <<' ' <<txt5;
                                                if (txt6) {cerr <<' ' <<txt6;}
                                               }
                                    }
                         }
              }
    cerr <<endl;

    return -1;
}
