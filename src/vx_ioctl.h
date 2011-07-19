/* @(#)src/sol/kernel/base/vx_ioctl.h	3.27.19.1 04 May 1999 16:58:32 -  */
/* #ident "@(#)vxfs:src/sol/kernel/base/vx_ioctl.h	3.27.19.1" */

/*
 * Copyright (c) 1998 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 *
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 *
 *		RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *		VERITAS SOFTWARE
 * 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043
 */

#ifndef	_FS_VXFS_VX_IOCTL_H
#define	_FS_VXFS_VX_IOCTL_H

/*
 * VXFS ioctls are organized as follows:
 *
 *	  0 - 127	User group
 *	      128	Admin group
 *	      129	DMAPI group
 *	      130	Admin Test group
 *
 * Ioctls in the User group are assigned a number using the generic ioctl
 * numeric scheme. This is to provide compatibility with older releases. The
 * other groups enter the kernel with a single ioctl number. The ioctl
 * identifier is encoded as part of the structure passed into the kernel. This
 * identifier is a 32 bit quantity allowing us to 2^32 different ioctls per
 * group.
 */

#define VX_IOCTL		(('V' << 24) | ('X' << 16) | ('F' << 8))

#define VX_USER_GROUP		127
#define VX_ADMIN_GROUP		128
#define VX_DMAPI_GROUP		129
#define VX_TEST_GROUP		130
#define VX_LICENSE_GROUP	151

#define VX_GET_IOCTL_ID(x)	((x) & 0xff)

/*
 * User group ioctls
 */

#define	VX_SETCACHE	(VX_IOCTL | 1)		/* set cache advice */
#define	VX_GETCACHE	(VX_IOCTL | 2)		/* get cache advice */
#define	VX_GETFSOPT	(VX_IOCTL | 5)		/* get cache advice */

/*
 * For ioctls that directly pass a value in the arg, the handling is variable
 * across the porting lines. For solaris, we just use arg.
 */

#define	VX_GETIOCTLARG(arg)	arg

/*
 * The get and set extent ioctls are versioned so that reservations
 * can exceed 2^31 blocks in the large file environment.
 *
 * Applications should continue to use vx_ext and VX_SETEXT/VX_GETEXT.
 */

#ifdef _KERNEL

#define	VX_SETEXT32	(VX_IOCTL | 3)
#define	VX_GETEXT32	(VX_IOCTL | 4)
#define VX_FREEZE_ALL	(VX_IOCTL | 6)		/* freeze multiple fs */

#define	VX_SETEXT64	(VX_IOCTL | 39)
#define	VX_GETEXT64	(VX_IOCTL | 40)
#define VX_FREEZE_ALL32	(VX_IOCTL | 41)

struct vx_ext32 {
	off32_t	ext_size;		/* extent size in fs blocks */
	off32_t	reserve;		/* space reservation in fs blocks */
	int	a_flags;		/* allocation flags */
};

struct vx_ext64 {
	off64_t	ext_size;		/* extent size in fs blocks */
	off64_t	reserve;		/* space reservation in fs blocks */
	int	a_flags;		/* allocation flags */
};

struct vx_freezeall32	{
	int	num;			/* number of fd pointed to */
	int	timeout;		/* timeout value for the freeze all */
	int	fds;			/* buffer for file descriptor list */
};

#else	/* _KERNEL */

#if	_FILE_OFFSET_BITS==64
#define	VX_SETEXT	(VX_IOCTL | 39)
#define	VX_GETEXT	(VX_IOCTL | 40)
#else
#define	VX_SETEXT	(VX_IOCTL | 3)
#define	VX_GETEXT	(VX_IOCTL | 4)
#endif	/*_FILE_OFFSET_BITS==64*/

struct vx_ext {
	off_t	ext_size;		/* extent size in fs blocks */
	off_t	reserve;		/* space reservation in fs blocks */
	int	a_flags;		/* allocation flags */
};

#ifdef VX_LP64
#define VX_FREEZE_ALL	(VX_IOCTL | 6)
#else
#define VX_FREEZE_ALL	(VX_IOCTL | 41)
#endif

#endif	/* _KERNEL */

/*
 * Values for freeze and thaw ioctls.  These must match the volume manager
 * VOL_FREEZE and VOL_THAW ioctl values.
 *
 * These are in the user group as opposed to the admin group because we need
 * to maintain backward binary compatibility with VxVM.
 */

#ifndef	VOLIOC
#define	VOLIOC	(('V' << 24) | ('O' << 16) | ('L' << 8))
#endif	/* VOLIOC */

#define	VX_FREEZE	(VOLIOC | 100)	/* freeze the file system */
#define	VX_THAW		(VOLIOC | 101)	/* unfreeze the file system */

/*
 * values for a_flags in vx_ext
 */

#define	VX_AFLAGS	0x3f	/* valid flags for a_flags */
#define	VX_NOEXTEND	0x01	/* file is not to be extended */
#define	VX_TRIM		0x02	/* trim reservation to i_size on close */
#define	VX_CONTIGUOUS	0x04	/* file must be contiguously allocated */
#define	VX_ALIGN	0x08	/* extents allocated on extent boundaries */
#define	VX_NORESERVE	0x10	/* don't change i_reserve */
#define	VX_CHGSIZE	0x20	/* change i_size to match reservation */

/*
 * vx_setcache flags
 */

#define	VX_ADVFLAGS		0x0003f	/* valid advisory flags */
#define	VX_RANDOM		0x00001	/* file is accessed randomly */
#define	VX_SEQ			0x00002	/* file is accessed sequentially */
#define	VX_DIRECT		0x00004	/* perform direct (un-buffered) i/o */
#define	VX_NOREUSE		0x00008	/* do not cache file data */
#define	VX_DSYNC		0x00010	/* synchronous data i/o (not mtime) */
#define	VX_UNBUFFERED		0x00020	/* perform non-sync direct i/o */

/*
 * Flags for VX_GETFSOPT
 */

#define	VX_FSO_NOLOG		0x0000001 /* mounted with VX_MS_NOLOG */
#define	VX_FSO_BLKCLEAR		0x0000002 /* mounted with VX_MS_BLKCLEAR */
#define	VX_FSO_NODATAINLOG	0x0000004 /* mounted with VX_MS_NODATAINLOG */
#define	VX_FSO_SNAPSHOT		0x0000008 /* is a snapshot */
#define	VX_FSO_SNAPPED		0x0000010 /* is being snapped */
#define	VX_FSO_VJFS		0x0000020 /* the kernel is VJFS */
#define	VX_FSO_DELAYLOG		0x0000040 /* mounted with VX_MS_DELAYLOG */
#define	VX_FSO_TMPLOG		0x0000080 /* mounted with VX_MS_TMPLOG */
#define	VX_FSO_CACHE_DIRECT	0x0000100 /* mounted with VX_MS_CACHE_DIRECT */
#define	VX_FSO_CACHE_DSYNC	0x0000200 /* mounted with VX_MS_CACHE_DSYNC */
#define	VX_FSO_CACHE_CLOSESYNC	0x0000400 /* mnt'd with VX_MS_CACHE_CLOSESYNC */
#define	VX_FSO_OSYNC_DIRECT	0x0001000 /* mounted with VX_MS_OSYNC_DIRECT */
#define	VX_FSO_OSYNC_DSYNC	0x0002000 /* mounted with VX_MS_OSYNC_DSYNC */
#define	VX_FSO_OSYNC_CLOSESYNC	0x0004000 /* mnt'd with VX_MS_OSYNC_CLOSESYNC */
#define	VX_FSO_FILESET		0x0010000 /* mounted as a file set */
#define	VX_FSO_CACHE_TMPCACHE	0x0020000 /* mnt'd with VX_MS_CACHE_TMPCACHE */
#define	VX_FSO_OSYNC_DELAY	0x0040000 /* mounted with VX_MS_OSYNC_DELAY */
#define	VX_FSO_CACHE_UNBUFFERED	0x0080000 /* mnt'd w/ VX_MS_CACHE_UNBUFFERED */
#define	VX_FSO_OSYNC_UNBUFFERED	0x0100000 /* mounted with VX_MS_UNBUFFERED */
#define	VX_FSO_QIO_ON		0x0200000 /* mounted with VX_MS_QIO_ON */

/*
 * The VX_FREEZE_ALL ioctl uses the following structure
 */

struct vx_freezeall	{
	int	num;			/* number of fd pointed to */
	int	timeout;		/* timeout value for the freeze all */
	int	*fds;			/* buffer for file descriptor list */
};

/*
 * Other groups use the following generic structure
 */

struct vx_genioctl {
	int	ioc_cmd;
	void	*ioc_up;
};

/*
 * Other ioctl groups
 */

#define	VX_ADMIN_IOCTL		(VX_IOCTL | VX_ADMIN_GROUP)
#define	VX_DMAPI_IOCTL		(VX_IOCTL | VX_DMAPI_GROUP)
#define	VX_TEST_IOCTL		(VX_IOCTL | VX_TEST_GROUP)
#define	VX_LICENSE_IOCTL	(VX_IOCTL | VX_LICENSE_GROUP)

#endif /* _FS_VXFS_VX_IOCTL_H */
