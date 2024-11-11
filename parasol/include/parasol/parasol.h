/* $Id: parasol.h 17453 2024-11-10 12:08:26Z greg $ */
/************************************************************************/
/*	parasol.h - main PARASOL header file				*/
/*									*/
/*	Copyright (C) 1993 School of Computer Science, 			*/
/*		Carleton University, Ottawa, Ont., Canada		*/
/*		Written by John Neilson					*/
/*									*/
/*  This library is free software; you can redistribute it and/or	*/
/*  modify it under the terms of the GNU Library General Public		*/
/*  License as published by the Free Software Foundation; either	*/
/*  version 2 of the License, or (at your option) any later version.	*/
/*									*/
/*  This library is distributed in the hope that it will be useful,	*/
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU	*/
/*  Library General Public License for more details.			*/
/*									*/
/*  You should have received a copy of the GNU Library General Public	*/
/*  License along with this library; if not, write to the Free		*/
/*  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.	*/
/*									*/
/*  The author may be reached at neilson@scs.carleton.ca or through	*/
/*  the School of Computer Science at Carleton University.		*/
/*									*/
/*	Created: 03/06/91 (JEN)						*/
/*									*/
/*	Revised: 06/08/91 (JEN) Removed "machine" from setjmp.h ref for	*/
/*				compatibility				*/
/*									*/
/*		 05/02/92 (PKC) Added version.h file.                  	*/
/*		 26/05/93 (JEN) Added malloc.h file.			*/
/*		 09/06/93 (JEN) Revised for Version 2			*/
/*		 07/18/95 (PRM)	Added ctype.h #include			*/
/************************************************************************/ 

#ifndef	_PARASOL
#define	_PARASOL

#if HAVE_CONFIG_H
#include	<config.h>
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include 	<string.h>
#include	<ctype.h>
#include	<setjmp.h>
#include	<signal.h>
#include	<math.h>
#include	<parasol/para_protos.h>
#if defined(HAVE_MALLOC_H)
#include	<malloc.h>
#endif

#endif	/* _PARASOL */
