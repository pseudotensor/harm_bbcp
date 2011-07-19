#ifndef __BBCP_EMSG_H__
#define __BBCP_EMSG_H__
/******************************************************************************/
/*                                                                            */
/*                           b b c p _ E m s g . h                            */
/*                                                                            */
/* (c) 2002 by the Board of Trustees of the Leland Stanford, Jr., University  */
/*      All Rights Reserved. See bbcp_Version.C for complete License Terms    *//*                            All Rights Reserved                             */
/*   Produced by Andrew Hanushevsky for Stanford University under contract    */
/*              DE-AC03-76-SFO0515 with the Department of Energy              */
/******************************************************************************/

extern int bbcp_Emsg(const char *sfx, int ecode, const char *txt1,
                                      char *txt2 = 0, char *txt3 = 0);
  
extern int bbcp_Fmsg(const char *sfx, const char *txt1,     char *txt2 = 0,
                                            char *txt3 = 0, char *txt4 = 0,
                                            char *txt5 = 0, char *txt6 = 0);
#endif
