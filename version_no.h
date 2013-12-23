/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 1, or (at your option)
** any later version.

** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.

** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
 * Author : Jonathan M. Gilligan <jonathan.gilligan@vanderbilt.edu> --- 23 June 2001
 */

/*
 * version_no.h -- Definitions for version numbering in cvsnt,
 *                 detailed documentation is below, see also, version_fu.h
 */

#ifndef VERSION_NO__H
#define VERSION_NO__H

// cvsnt version
#define CVSNT_PRODUCT_MAJOR 2
#define CVSNT_PRODUCT_MINOR 0
#define CVSNT_PRODUCT_PATCHLEVEL 51d

//
// Example preprocessor definitions.
//
// #define CVSNT_SPECIAL_BUILD "My special"
// note: no "build" at the end. This is added by
// version-fu.
//
#ifdef RC_INVOKED
//#define CVSNT_SPECIAL_BUILD " (Prerelease)"
#else
//#define CVSNT_SPECIAL_BUILD " (Prerelease "__DATE__")"
#endif
#ifdef SJIS
#define CVSNT_SPECIAL_BUILD "(SJIS-3)"
#endif

//////////////////////////////////////////////////////////////////////////
// Preprocessor definitions (for version_no.h)
//
// Preprocessor flags (empty #defines):
// These should be #defined in version_no.h
//
//
// Preprocessor values (#defines with contents):
// These should be #defined in version_no.h
//
// CVSNT_FILE_MAJOR,
// CVSNT_FILE_MINOR,
// CVSNT_FILE_PATCHLEVEL:
//
//			            These macros all indicate parts of the version
//                      number. *FILE* indicates specific numbers for the
//                      .exe files (cvs.exe, cvs95.exe). The *PRODUCT*
//                      numbers indicate the version of the package as a
//                      whole.
//
//                      At this time, cvs.exe is at version 1.11.0.x
//                      (cvs 1.11.0, build x).
//
//                      The build numbers for both files should be
//                      incremented (by hand editing version_no.h) at
//                      least once for every build posted publicly at
//                      cvsgui.org.
//
//                      EXAMPLE:
//
//                      #define CVSNT_FILE_MAJOR           1
//                      #define CVSNT_FILE_MINOR          11
//                      #define CVSNT_FILE_PATCHLEVEL      0
//
//
// CVSNT_SPECIAL_BUILD,
// CVSNT_PRIVATE_BUILD:    These should be #defined to a double-quoted
//                      string that will be used to create a prefix for
//                      the build description string. E.g., the following:
//
//                      #define CVSNT_SPECIAL_BUILD "anomalous"
//                      #define CVSNT_PRIVATE_BUILD "in house"
//
//                      Will produce the text "in house anomalous build"
//                      in the file and product description strings,
//                      "anomalous build" in the Special Build Description
//                      string, and "in house build" in the Private Build
//                      string in the VERSION_INFO resource.
//
// All other necessary preprocessor definitions are synthesized by
// version_fu.h
//
//////////////////////////////////////////////////////////////////////////
#endif
