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
 * version_fu.h -- Macros used for version numbering
 */

#ifndef VERSION_FU__H
#define VERSION_FU__H

#pragma warning(disable:4003)

//////////////////////////////////////////////////////////////////////////
// Documentation
// 
// The macros below generate a number of strings that describe aspects of 
// the current build.
// 
// Principal aspects include a number of MS-specified flags:
//
// Debug build:         Self-explanatory.
//
// Private Build:       A non-standard build. We use the private build 
//                      flag to indicate that the build was not an 
//                  `   official release. As checked out with CVS, the
//                      sources are set up to build as a private build.
//
//
// Special Build:       Some other kind of non-standard build. We do not
//                      currently define special builds, but the plumbing
//                      is in place in version_fu to handle special builds
//                      if the flag CVSNT_SPECIAL_BUILD is defined.
//
//
// Strings and other macros generated by version_fu:
//
// CVSNT_PRODUCTVERSION:   Version tokens to be used in the VERSION_INFO 
//                      resource. These tokens have the form x,x,x,x where 
//                      x is a decimal number indicating 
//                      major,minor,patch,build.
//
//                      File version refers to the specific executable file
//                      (cvs.exe). Product version refers to the cvsnt package 
//                      as a whole.
//
// CVSNT_PRODUCTVERSION_STRING:    Double-quoted string equivalents of the 
//                      above. The format is:
//                      "N x.x.x.x r (s p u d Build x)",
//                      Where 
//                      N = name (of file or product)
//                      x.x.x.x = major.minor.patchlevel.build,
//                      r = prerelease build string,
//                      s = special build string,
//                      p = private build string,
//                      u = unicode build string,
//                      d = debug build string.
//
//                      Example:
//                      "cvsnt 1.11.0.7 Beta 2 (Special Private Unicode Debug Build 7)"
//
// CVSNT_PATCHED_FLAG,
// CVSNT_SPECIAL_BUILD_FLAG,
// CVSNT_PRIVATE_BUILD_FLAG,
// CVSNT_DEBUG_BUILD_FLAG:   These are flags (numbers, which can be OR'ed together
//                          for the FILEFLAGS member of VERSION_INFO.
//
// CVSNT_UNICODE_BUILD_STRING,
// CVSNT_DEBUG_BUILD_STRING,
// CVSNT_SPECIAL_BUILD_STRING,
// CVSNT_PRIVATE_BUILD_STRING,
//					        These are strings (ending with 
//                          a space) that can be concatenated together
//                          to produce a description of the various
//                          build conditions. 
//
//                          The order these fields appear in the File 
//                          Description and Product Description fields of 
//                          VERSION_INFO are: PRERELEASE, PRIVATE, SPECIAL, 
//                          UNICODE, and DEBUG.
//
// 

//////////////////////////////////////////////////////////////////////////

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
// CVSNT_PRODUCT_MAJOR,
// CVSNT_PRODUCT_MINOR,
// CVSNT_PRODUCT_PATCHLEVEL:
//
//			            These macros all indicate parts of the version 
//                      number. *FILE* indicates specific numbers for the 
//                      .exe files (cvs.exe, cvs95.exe). The *PRODUCT*
//                      numbers indicate the version of the package as a 
//                      whole.
//
//                      At this time, cvs.exe is at version 1.11.0
//                      (cvs 1.11.0).
//
//                      EXAMPLE:
//
//                      #define CVSNT_PRODUCT_MAJOR        1
//                      #define CVSNT_PRODUCT_MINOR        11
//                      #define CVSNT_PRODUCT_PATCHLEVEL   0
//
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

//////////////////////////////////////////////////////////////////////////
// We need two macros because token expansion occurs when PP_STRINGIZE 
// expands x when passing it to PP_PASTE.
#define PP_PASTE(x) #x
#define PP_TPASTE(x) _T(#x)
#define PP_STRINGIZE(x) PP_PASTE(x)
#define PP_TSTRINGIZE(x) PP_TPASTE(x)

#define CVSNT_PRODUCTVERSION CVSNT_PRODUCT_MAJOR,CVSNT_PRODUCT_MINOR,CVSNT_PRODUCT_PATCHLEVEL

#define CVSNT_UNICODE_BUILD_STRING
#define CVSNT_BRACKET_LEFT
#define CVSNT_BRACKET_RIGHT

#if defined(CVSNT_PATCH)
#define CVSNT_PATCHED_FLAG VS_FF_PATCHED
#else   // defined(CVSNT_PATCH)
#define CVSNT_PATCHED_FLAG 0
#endif  // defined(CVSNT_PATCH)

#if defined(CVSNT_SPECIAL_BUILD)
#define CVSNT_SPECIAL_BUILD_STRING CVSNT_SPECIAL_BUILD 
#define CVSNT_SPECIAL_BUILD_FLAG VS_FF_SPECIALBUILD
#else   // defined(CVSNT_SPECIAL_BUILD)
#define CVSNT_SPECIAL_BUILD_STRING
#define CVSNT_SPECIAL_BUILD_FLAG 0
#endif  // defined(CVSNT_SPECIAL_BUILD)

#if defined(CVSNT_PRIVATE_BUILD)
#define CVSNT_PRIVATE_BUILD_STRING CVSNT_PRIVATE_BUILD 
#define CVSNT_PRIVATE_BUILD_FLAG VS_FF_PRIVATEBUILD
#else   // defined(CVSNT_PRIVATE_BUILD)
#define CVSNT_PRIVATE_BUILD_STRING
#define CVSNT_PRIVATE_BUILD_FLAG 0
#endif  // defined(CVSNT_PRIVATE_BUILD)

#if defined(_DEBUG)
#define CVSNT_DEBUG_FLAG VS_FF_DEBUG
#define CVSNT_DEBUG_BUILD_STRING "Debug"
#undef CVSNT_BRACKET_LEFT
#undef CVSNT_BRACKET_RIGHT
#define CVSNT_BRACKET_LEFT " ("
#define CVSNT_BRACKET_RIGHT ")"
#else   // defined(_DEBUG)
#define CVSNT_DEBUG_BUILD_STRING
#define CVSNT_DEBUG_FLAG 0
#endif  // defined(_DEBUG)

//
//  Resource Compiler chokes if we break these long lines with line continuation characters '\'
//
#define CVSNT_PRODUCTVERSION_STRING PP_STRINGIZE(CVSNT_PRODUCT_MAJOR.CVSNT_PRODUCT_MINOR.CVSNT_PRODUCT_PATCHLEVEL) CVSNT_PRIVATE_BUILD_STRING CVSNT_SPECIAL_BUILD_STRING CVSNT_BRACKET_LEFT CVSNT_UNICODE_BUILD_STRING CVSNT_DEBUG_BUILD_STRING CVSNT_BRACKET_RIGHT
#define CVSNT_PRODUCTVERSION_SHORT PP_STRINGIZE(CVSNT_PRODUCT_MAJOR.CVSNT_PRODUCT_MINOR.CVSNT_PRODUCT_PATCHLEVEL) 
#define T_CVSNT_PRODUCTVERSION_SHORT PP_TSTRINGIZE(CVSNT_PRODUCT_MAJOR.CVSNT_PRODUCT_MINOR.CVSNT_PRODUCT_PATCHLEVEL) 
#define CVSNT_FILEVERSION CVSNT_PRODUCT_MAJOR,CVSNT_PRODUCT_MINOR,CVSNT_PRODUCT_PATCHLEVEL

#endif
