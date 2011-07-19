# These are the default value for all archetures. The makefile appears
# cumbersome but only because we wanted to have one makefile that would
# work in a variety of disparate architecture and various release levels.
# Additionally, this make file is easily modifiable for test environments.
#

BINDIR     = ../bin/$(OSVER)

OBJDIR     = ../obj/$(OSVER)

ENVCFLAGS  =    -Dunix -D_BSD -D_ALL_SOURCE

ENVINCLUDE = -I. -I/usr/local/include -I/usr/include

AIXZ       =  -q64 -L/usr/lib -lz
MACZSO     =  -dynamic -L/usr/lib -lz
LIBZSO     =  -L/usr/local/lib -lz
#LIBZ       =  /usr/local/lib/libz.a
LIBZ       =  /usr/lib64/libz.a

MACLIBS    =  $(MACZSO)                         -lpthread
SUNLIBS    =  $(LIBZ)   -lposix4 -lsocket -lnsl -lpthread
SUNLIBSO   =  $(LIBZSO) -lposix4 -lsocket -lnsl -lpthread

LNXLIBS    =  $(LIBZ) -lnsl -lpthread -lrt

BSDLIBS    =  $(LIBZ) -lc_r -pthread

AIXLIBS    =  $(AIXZ) -lpthreads

MKPARMS  = doitall

NLGR       = -DNL_THREADSAFE

SUN64      = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
SUNMT      = -D_REENTRANT -DOO_STD -mt
SUNOPT     = -O2 -fsimple -fast
SUNOPT     = -g  -fsimple $(SUNMT) $(SUN64) $(NLGR) -DSUN

SUNCC      = CC
SUNcc      = cc
#SUNCC      = /opt/SUNWspro.WS7/bin/CC

S86OPT     = -D_REENTRANT -DOO_STD $(NLGR) -DSUN -DSUNX86 -Wno-deprecated
S86OPT     = $(SUNOPT) -DSUNX86 

S86CC      = CC
S86cc      = gcc

LNXOPT     = $(SUN64) $(NLGR) -D_REENTRANT -DOO_STD -DLINUX -Wno-deprecated \
             -D_GNU_SOURCE -g
LNXOPT_B   = $(SUN64) $(NLGR) -D_REENTRANT -DOO_STD -DLINUX \
             -D_GNU_SOURCE -g

LNXCC      = g++
LNXcc      = gcc

BSDOPT     = $(SUN64) $(NLGR) -D_REENTRANT -DOO_STD -DFREEBSD -pthread

BSDCC      = g++
BSDcc      = gcc

MACOPT     = $(SUN64) $(NLGR) -D_REENTRANT -DOO_STD -DMACOS -Wno-deprecated -g

MACCC      = g++
MACcc      = gcc

AIXCC      = xlC
AIXcc      = xlc

AIXOPT     = -q64 $(SUN64) $(NLGR) -D_REENTRANT -DOO_STD -DAIX -g

SOURCE =      \
     bbcp.C \
     bbcp_Args.C \
     bbcp_BuffPool.C \
     bbcp_Config.C \
     bbcp_Emsg.C \
     bbcp_File.C \
     bbcp_FileSpec.C \
     bbcp_FileSystem.C \
     bbcp_FS_Null.C \
     bbcp_FS_Unix.C \
     bbcp_FS_VXFS.C \
     bbcp_IO.C \
     bbcp_IO_Null.C \
     bbcp_Link.C \
     bbcp_LogFile.C \
     bbcp_MD5.C \
     bbcp_NetLogger.C \
     bbcp_Network.C \
     bbcp_Node.C \
     bbcp_ProcMon.C \
     bbcp_ProgMon.C \
     bbcp_Protocol.C \
     bbcp_Pthread.C \
     bbcp_Stream.C \
     bbcp_System.C \
     bbcp_Timer.C \
     bbcp_Version.C \
     bbcp_ZCX.C \
     NetLogger.c

OBJECT =      \
     $(OBJDIR)/bbcp.o \
     $(OBJDIR)/bbcp_Args.o \
     $(OBJDIR)/bbcp_BuffPool.o \
     $(OBJDIR)/bbcp_Config.o \
     $(OBJDIR)/bbcp_Emsg.o \
     $(OBJDIR)/bbcp_File.o \
     $(OBJDIR)/bbcp_FileSpec.o \
     $(OBJDIR)/bbcp_FileSystem.o \
     $(OBJDIR)/bbcp_FS_Null.o \
     $(OBJDIR)/bbcp_FS_Unix.o \
     $(OBJDIR)/bbcp_FS_VXFS.o \
     $(OBJDIR)/bbcp_IO.o \
     $(OBJDIR)/bbcp_IO_Null.o \
     $(OBJDIR)/bbcp_Link.o \
     $(OBJDIR)/bbcp_LogFile.o \
     $(OBJDIR)/bbcp_MD5.o \
     $(OBJDIR)/bbcp_NetLogger.o \
     $(OBJDIR)/bbcp_Network.o \
     $(OBJDIR)/bbcp_Node.o \
     $(OBJDIR)/bbcp_ProcMon.o \
     $(OBJDIR)/bbcp_ProgMon.o \
     $(OBJDIR)/bbcp_Protocol.o \
     $(OBJDIR)/bbcp_Pthread.o \
     $(OBJDIR)/bbcp_Stream.o \
     $(OBJDIR)/bbcp_System.o \
     $(OBJDIR)/bbcp_Timer.o \
     $(OBJDIR)/bbcp_Version.o \
     $(OBJDIR)/bbcp_ZCX.o \
     $(OBJDIR)/NetLogger.o

TARGET  = $(BINDIR)/bbcp

all:
	@cd src;make make`/bin/uname` OSVER=`../MakeSname`
	@echo Make done.

doitall: $(TARGET)

clean:
	@cd src;make cleanall OSVER=`../MakeSname`

cleanall:
	@rm -f $(OBJECT) $(BINDIR)/core $(TARGET) $(OBJDIR)/*

usage:
	@echo "Usage: make [echo | usage] [OPT=-DSUN6] [{AIX|BSD|LNX|SUN|S86}CC=ccpath]"

echo:		
	@for f in $(SOURCE); do \
	echo $$f ;\
	done

makeAIX:
	@make $(MKPARMS) \
	CC=$(AIXCC) \
	BB=$(AIXcc) \
	BFLAGS="$(ENVCFLAGS) $(AIXOPT)" \
	CFLAGS="$(ENVCFLAGS) $(AIXOPT)" \
	INCLUDE="$(ENVINCLUDE)"  \
	LIBS="$(AIXLIBS)"

makeFreeBSD:
	@make $(MKPARMS)  \
	CC=$(BSDCC) \
	BB=$(BSDcc) \
	CFLAGS="$(ENVCFLAGS) $(BSDOPT) $(OPT)" \
	INCLUDE="$(ENVINCLUDE)" \
	LIBS="$(BSDLIBS)"

makeLinux:
	@make $(MKPARMS)  \
	CC=$(LNXCC) \
	BB=$(LNXcc) \
	CFLAGS="$(ENVCFLAGS) $(LNXOPT)" \
	BFLAGS="$(ENVCFLAGS) $(LNXOPT_B)" \
	INCLUDE="$(ENVINCLUDE)" \
	LIBS="$(LNXLIBS)"

makeDarwin:
	@make $(MKPARMS)  \
	CC=$(MACCC) \
	BB=$(MACcc) \
	CFLAGS="$(ENVCFLAGS) $(MACOPT) $(OPT)" \
	INCLUDE="$(ENVINCLUDE)" \
	LIBS="$(MACLIBS)"

makeUNICOS/mp:
	@make $(MKPARMS)  \
 	CC=$(LNXCC) \
 	BB=$(LNXcc) \
 	CFLAGS="$(ENVCFLAGS) $(LNXOPT)" \
	BFLAGS="$(ENVCFLAGS) $(LNXOPT_B)" \
 	INCLUDE="$(ENVINCLUDE)" \
 	LIBS="$(LNXLIBS)"

makeSunOS:
	@make $(MKPARMS)  \
	CC=$(SUNCC) \
	BB=$(SUNcc) \
	CFLAGS="$(ENVCFLAGS) $(SUNOPT) $(OPT) -DBBCP_VXFS" \
	INCLUDE="$(ENVINCLUDE)" \
	LIBS="$(SUNLIBS)"

makeSunX86:
	@make $(MKPARMS)  \
	CC=$(S86CC) \
	BB=$(S86cc) \
	CFLAGS="$(ENVCFLAGS) $(S86OPT) $(OPT)" \
	INCLUDE="$(ENVINCLUDE)" \
	LIBS="$(SUNLIBSO)"

$(TARGET): $(OBJECT)
	@echo Creating executable $(TARGET) ...
	@$(CC) $(OBJECT) $(LIBS) -o $(TARGET)

$(OBJDIR)/bbcp.o: bbcp.C bbcp_Args.h bbcp_Config.h bbcp_Debug.h bbcp_FileSpec.h \
                  bbcp_LogFile.h bbcp_Node.h bbcp_Protocol.h bbcp_System.h \
                  bbcp_Headers.h
	@echo Compiling bbcp.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_Args.o: bbcp_Args.C bbcp_Args.h bbcp_Config.h bbcp_Stream.h
	@echo Compiling bbcp_Args.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_BuffPool.o: bbcp_BuffPool.C bbcp_BuffPool.h bbcp_Debug.h \
                           bbcp_Emsg.h bbcp_Pthread.h bbcp_Headers.h
	@echo Compiling bbcp_BuffPool.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_Config.o: bbcp_Config.C bbcp_Config.h bbcp_Args.h bbcp_BuffPool.h \
                         bbcp_Debug.h bbcp_Emsg.h bbcp_FileSpec.h bbcp_LogFile.h \
                         bbcp_NetLogger.h bbcp_Network.h bbcp_Platform.h \
                         bbcp_Stream.h bbcp_System.h bbcp_Version.h \
                         bbcp_Headers.h
	@echo Compiling bbcp_Config.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_Emsg.o: bbcp_Emsg.C bbcp_Emsg.h bbcp_Debug.h bbcp_Platform.h \
                       bbcp_Headers.h
	@echo Compiling bbcp_Emsg.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_File.o: bbcp_File.C bbcp_File.h bbcp_IO.h bbcp_BuffPool.h \
                       bbcp_Config.h bbcp_Debug.h bbcp_Emsg.h bbcp_Pthread.h \
                       bbcp_Platform.h bbcp_Headers.h
	@echo Compiling bbcp_File.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_FileSpec.o: bbcp_FileSpec.C bbcp_FileSpec.h bbcp_Config.h \
                           bbcp_Emsg.h bbcp_FileSystem.h bbcp_FS_Unix.h \
                           bbcp_Pthread.h
	@echo Compiling bbcp_FileSpec.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_FileSystem.o: bbcp_FileSystem.C bbcp_FileSystem.h \
                             bbcp_FS_Null.h bbcp_FS_Unix.h bbcp_FS_VXFS.h
	@echo Compiling bbcp_FileSystem.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_FS_Null.o: bbcp_FS_Null.C bbcp_FS_Null.h \
                          bbcp_IO_Null.h bbcp_FileSystem.h bbcp_System.h \
                          bbcp_Platform.h bbcp_Pthread.h
	@echo Compiling bbcp_FS_Null.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_FS_Unix.o: bbcp_FS_Unix.C bbcp_FS_Unix.h bbcp_FileSystem.h \
                          bbcp_Platform.h bbcp_System.h bbcp_Pthread.h
	@echo Compiling bbcp_FS_Unix.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_FS_VXFS.o: bbcp_FS_VXFS.C bbcp_FS_VXFS.h bbcp_FS_Unix.h \
                          bbcp_Debug.h bbcp_File.h \
                          bbcp_FileSystem.h bbcp_Platform.h bbcp_System.h
	@echo Compiling bbcp_FS_VXFS.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_IO.o: bbcp_IO.C bbcp_IO.h bbcp_NetLogger.h bbcp_Timer.h
	@echo Compiling bbcp_IO.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_IO_Null.o: bbcp_IO_Null.C bbcp_IO_Null.h bbcp_IO.h
	@echo Compiling bbcp_IO_Null.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_Link.o:  bbcp_Link.C bbcp_Link.h bbcp_File.h bbcp_IO.h \
                        bbcp_BuffPool.h bbcp_Config.h bbcp_Debug.h bbcp_Emsg.h \
                        bbcp_Link.h bbcp_MD5.h bbcp_Network.h bbcp_Platform.h \
                        bbcp_Timer.h
	@echo Compiling bbcp_Link.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_LogFile.o:  bbcp_LogFile.C bbcp_LogFile.h bbcp_Pthread.h \
                           bbcp_Debug.h bbcp_Emsg.h bbcp_Platform.h \
                           bbcp_Headers.h bbcp_Timer.h
	@echo Compiling bbcp_LogFile.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_MD5.o: bbcp_MD5.C bbcp_MD5.h
	@echo Compiling bbcp_MD5.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_NetLogger.o:  bbcp_NetLogger.C bbcp_NetLogger.h NetLogger.h
	@echo Compiling bbcp_Netlogger.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_Network.o:  bbcp_Network.C bbcp_Network.h bbcp_Config.h \
                           bbcp_Emsg.h bbcp_Link.h bbcp_Pthread.h bbcp_Platform.h
	@echo Compiling bbcp_Network.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_Node.o: bbcp_Node.C bbcp_Node.h bbcp_Config.h bbcp_Emsg.h \
                       bbcp_Debug.h bbcp_File.h bbcp_FileSpec.h bbcp_Link.h \
                       bbcp_BuffPool.h bbcp_Network.h bbcp_Protocol.h \
                       bbcp_ProcMon.h bbcp_ProgMon.h bbcp_Stream.h \
                       bbcp_System.h bbcp_ZCX.h bbcp_Headers.h
	@echo Compiling bbcp_Node.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_ProcMon.o: bbcp_ProcMon.C bbcp_ProcMon.h bbcp_Debug.h \
                          bbcp_Emsg.h bbcp_File.h bbcp_Pthread.h bbcp_Headers.h
	@echo Compiling bbcp_ProcMon.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_ProgMon.o: bbcp_ProgMon.C bbcp_ProgMon.h bbcp_Config.h \
                          bbcp_Debug.h bbcp_File.h bbcp_Headers.h \
                          bbcp_Pthread.h bbcp_Timer.h bbcp_ZCX.h
	@echo Compiling bbcp_ProgMon.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_Protocol.o: bbcp_Protocol.C bbcp_Protocol.h bbcp_Config.h  \
                           bbcp_Node.h bbcp_FileSpec.h bbcp_FileSystem.h  \
                           bbcp_IO.h bbcp_File.h bbcp_Link.h bbcp_Debug.h \
                           bbcp_Pthread.h bbcp_Stream.h bbcp_Emsg.h \
                           bbcp_Network.h bbcp_Version.h bbcp_Headers.h
	@echo Compiling bbcp_Protocol.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_Pthread.o: bbcp_Pthread.C bbcp_Pthread.h
	@echo Compiling bbcp_Pthread.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_Stream.o: bbcp_Stream.C bbcp_Stream.h bbcp_Config.h bbcp_Debug.h \
                         bbcp_Emsg.h bbcp_System.h
	@echo Compiling bbcp_Stream.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_System.o: bbcp_System.C bbcp_System.h bbcp_Config.h \
                         bbcp_Debug.h bbcp_Emsg.h bbcp_Platform.h bbcp_Pthread.h
	@echo Compiling bbcp_System.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_Timer.o: bbcp_Timer.C bbcp_Timer.h
	@echo Compiling bbcp_Timer.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_Version.o: bbcp_Version.C bbcp_Version.h bbcp_Config.h bbcp_Emsg.h
	@echo Compiling bbcp_Version.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/bbcp_ZCX.o: bbcp_ZCX.C bbcp_ZCX.h bbcp_BuffPool.h bbcp_Emsg.h
	@echo Compiling bbcp_ZCX.C
	@$(CC) -c $(CFLAGS) $(INCLUDE) $(*F).C -o $(OBJDIR)/$(*F).o

$(OBJDIR)/NetLogger.o: NetLogger.c NetLogger.h
	@echo Compiling NetLogger.c
	@$(BB) -c $(BFLAGS) $(INCLUDE) $(*F).c -o $(OBJDIR)/$(*F).o
