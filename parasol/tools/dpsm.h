/* @(#)dpsm.h	1.2 08:08:03 8/29/95 */
/************************************************************************/
/*	dpsm.h - Header file for DPSM programs dpsm.c and dpss.c 	*/
/*									*/
/*	Copyright (C) 1995 School of Computer Science, 			*/
/*		Carleton University, Ottawa, Ont., Canada		*/
/*		Written by Patrick Morin				*/
/*		Based on ps_dist.c by Robert Davison.			*/
/*									*/
/*  This program is free software; you can redistribute it and/or modify*/
/*  it under the terms of the GNU General Public License as published by*/
/*  the Free Software Foundation; either version 2, or (at your option)	*/
/*  any later version.							*/
/*									*/
/*  This program is distributed in the hope that it will be useful,	*/
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of	*/
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the	*/
/*  GNU General Public License for more details.			*/
/*									*/
/*  You should have received a copy of the GNU General Public License	*/
/*  along with this program; if not, write to the Free Software		*/
/*  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.		*/
/*									*/
/*  The author may be reached at morin@scs.carleton.ca or through	*/
/*  the School of Computer Science at Carleton University.		*/
/*									*/
/*	Created: 11/08/95 (PRM)						*/
/*									*/
/************************************************************************/
#ifndef __DPSM_H
#define __DPSM_H

/************************************************************************/
/* 	Maximum message sizes.						*/
/************************************************************************/
#define MAXLINE 100				/* Maximum output line	*/
#define MAXINPUT 200				/* Maximum prog. input	*/
#define MAXPROGNAME 20				/* Maximum prog. name	*/

/************************************************************************/
/* 	Simple mnemonics						*/
/************************************************************************/
enum { FALSE, TRUE };

/************************************************************************/
/* 	Message types 							*/
/************************************************************************/
enum { 
	REQUEST,				/* request type		*/
	OUTPUT,					/* output line		*/
	ENDOUTPUT,				/* end of output	*/
	ABORT,					/* abort simulation	*/
	TERMINATE,				/* kill yourself	*/
	HUNG					/* simulation is hung	*/
};

#endif /* __DPSM_H */

