/* $Id: mac_res.r,v 1.1.4.1 2004/08/04 12:04:02 tmh Exp $ */
/*
 * Copyright (c) 1999, 2002, 2003 Ben Harris
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* PuTTY resources */

/*
 * The space after the # for system includes is to stop mkfiles.pl
 * trying to chase them (Rez doesn't support the angle-bracket
 * syntax).
 */

# include "Types.r"
# include "Dialogs.r"
# include "Palettes.r"
# include "Script.r"

/* Get resource IDs we share with C code */
#include "macresid.h"

#include "version.r"

/*
 * Finder-related resources
 */

/* 'pTTY' is now registered with Apple as PuTTY's signature */

type 'pTTY' as 'STR ';

resource 'pTTY' (0, purgeable) {
    "PuTTY experimental Mac port"
};

resource 'SIZE' (-1) {
    reserved,
    acceptSuspendResumeEvents,
    reserved,
    canBackground,
    doesActivateOnFGSwitch,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreAppDiedEvents,
    is32BitCompatible,
    isHighLevelEventAware,
    localandRemoteHLEvents,
    isStationeryAware,
    dontUseTextEditServices,
    reserved,
    reserved,
    reserved,
    2048 * 1024,	/* Preferred size */
    1024 * 1024,	/* Minimum size */
};

#define FREF_APPL 128
#define FREF_Sess 129
#define FREF_Sesss 130
#define FREF_HKey 131
#define FREF_Seed 132

resource 'FREF' (FREF_APPL, purgeable) {
    /* The application itself */
    'APPL', FREF_APPL, ""
};

resource 'FREF' (FREF_Sess, purgeable) {
    /* Saved session */
    'Sess', FREF_Sess, ""
};

resource 'FREF' (FREF_Sesss, purgeable) {
    /* Saved session stationery pad*/
    'sess', FREF_Sesss, ""
};

resource 'FREF' (FREF_HKey, purgeable) {
    /* SSH host keys database */
    'HKey', FREF_HKey, ""
};

resource 'FREF' (FREF_Seed, purgeable) {
    /* Random seed */
    'Seed', FREF_Seed, ""
};

resource 'BNDL' (128, purgeable) {
    'pTTY', 0,
    {
	'ICN#', {
	    FREF_APPL, FREF_APPL,
	    FREF_Sess, FREF_Sess,
	    FREF_Sesss, FREF_Sesss
	},
	'FREF', {
	    FREF_APPL, FREF_APPL,
	    FREF_Sess, FREF_Sess,
	    FREF_Sesss, FREF_Sesss,
	};
    };
};

/* "Internal" file types, which can't be opened */
resource 'BNDL' (129, purgeable) {
    'pTTI', 0,
    {
	'ICN#', {
	    FREF_HKey, FREF_HKey,
	    FREF_Seed, FREF_Seed,
	},
	'FREF', {
	    FREF_HKey, FREF_HKey,
	    FREF_Seed, FREF_Seed,
	};
    };
};

/* Open resource, for the Translation Manager and Navigation Services */
resource 'open' (open_pTTY) {
    'pTTY',
    { 'Sess' }
};

/* Kind resources, for Navigation services etc. */
resource 'kind' (128) {
    'pTTY',
    verBritain,
    {
	'Sess', "PuTTY saved session",
    }
};

resource 'kind' (129) {
    'pTTI',
    verBritain,
    {
	'HKey', "PuTTY host key database",
	'Seed', "PuTTY random number seed",
    }
};

#if TARGET_API_MAC_CARBON
/*
 * Mac OS X Info.plist.
 * See Tech Note TN2013 for details.
 * We don't bother with things that Mac OS X seems to be able to get from
 * other resources.
 */
type 'plst' as 'TEXT';

resource 'plst' (0) {
    "<?xml version='1.0' encoding='UTF-8'?>\n"
    "<!DOCTYPE plist PUBLIC '-//Apple Computer//DTD PLIST 1.0//EN'\n"
    " 'http://www.apple.com/DTDs/PropertyList-1.0.dtd'>\n"
    "<plist version='1.0'>\n"
    "  <dict>\n"
    "    <key>CFBundleInfoDictionaryVersion</key> <string>6.0</string>\n"
    "    <key>CFBundleIdentifier</key>\n"
    "      <string>org.tartarus.projects.putty.putty</string>\n"
    "    <key>CFBundleName</key>                  <string>PuTTY</string>\n"
    "    <key>CFBundlePackageType</key>           <string>APPL</string>\n"
    "    <key>CFBundleSignature</key>             <string>pTTY</string>\n"
    "  </dict>\n"
    "</plist>\n"
};

/* Mac OS X doesn't use this, but Mac OS 9 does. */
type 'carb' as 'TEXT';
resource 'carb' (0) { "" };
#endif

/* Icons, courtesy of DeRez */

/* Application icon */
resource 'ICN#' (FREF_APPL, purgeable) {
	{	/* array: 2 elements */
		/* [1] */
		$"00003FFE 00004001 00004FF9 00005005"
		$"00005355 00004505 00005A05 00002405"
		$"00004A85 00019005 000223F9 00047C01"
		$"00180201 7FA00C7D 801F1001 9FE22001"
		$"A004DFFE AA892002 A0123FFE A82C0000"
		$"A0520000 AA6A0000 A00A0000 9FF20000"
		$"80020000 80020000 90FA0000 80020000"
		$"80020000 7FFC0000 40040000 7FFC",
		/* [2] */
		$"00003FFE 00007FFF 00007FFF 00007FFF"
		$"00007FFF 00007FFF 00007FFF 00007FFF"
		$"00007FFF 0001FFFF 0003FFFF 0007FFFF"
		$"001FFFFF 7FFFFFFF FFFFFFFF FFFFFFFF"
		$"FFFFFFFE FFFF3FFE FFFE3FFE FFFE0000"
		$"FFFE0000 FFFE0000 FFFE0000 FFFE0000"
		$"FFFE0000 FFFE0000 FFFE0000 FFFE0000"
		$"FFFE0000 7FFC0000 7FFC0000 7FFC"
	}
};
resource 'icl4' (FREF_APPL, purgeable) {
	$"000000000000000000FFFFFFFFFFFFF0"
	$"00000000000000000FCCCCCCCCCCCCCF"
	$"00000000000000000FCEEEEEEEEEEECF"
	$"00000000000000000FCE0D0D0D0D0CCF"
	$"00000000000000000FCED0FFD0D0D0CF"
	$"00000000000000000FCE0F1F0D0D0CCF"
	$"00000000000000000FCFF1F0D0D0D0CF"
	$"00000000000000000FF11F0D0D0D0CCF"
	$"00000000000000000F11F0D0D0D0D0CF"
	$"000000000000000FF11F0D0D0D0D0CCF"
	$"00000000000000F111FEC0C0C0C0C0CF"
	$"0000000000000F111FFFFFCCCCCCCCCF"
	$"00000000000FF111111111FCCCCCCCCF"
	$"0FFFFFFFFFF111111111FFCCCFFFFFCF"
	$"FCCCCCCCCCCFFFFF111F3CCCCCCCCCCF"
	$"FCEEEEEEEEEEECF111FCCCCCCCCCCCCF"
	$"FCE0D0D0D0D0CF11FFFFFFFFFFFFFFF0"
	$"FCED0D0D0D0DF11F00FCCCDDDEEEEAF0"
	$"FCE0D0D0D0DF11F000FFFFFFFFFFFFF0"
	$"FCED0D0D0DF1FFF00000000000000000"
	$"FCE0D0D0DF1FCCF00000000000000000"
	$"FCED0D0D0FFD0CF00000000000000000"
	$"FCE0D0D0D0D0CCF00000000000000000"
	$"FCEC0C0C0C0C0CF00000000000000000"
	$"FCCCCCCCCCCCCCF00000000000000000"
	$"FCCCCCCCCCCCCCF00000000000000000"
	$"FC88CCCCFFFFFCF00000000000000000"
	$"FC33CCCCCCCCCCF00000000000000000"
	$"FCCCCCCCCCCCCCF00000000000000000"
	$"0FFFFFFFFFFFFF000000000000000000"
	$"0FCCCDDDEEEEAF000000000000000000"
	$"0FFFFFFFFFFFFF"
};
resource 'icl8' (FREF_APPL, purgeable) {
	$"000000000000000000000000000000000000FFFFFFFFFFFFFFFFFFFFFFFFFF00"
	$"0000000000000000000000000000000000FF2B2B2B2B2B2B2B2B2B2B2B2B2BFF"
	$"0000000000000000000000000000000000FF2BFCFCFCFCFCFCFCFCFCFCFC2BFF"
	$"0000000000000000000000000000000000FF2BFC2A2A2A2A2A2A2A2A2A002BFF"
	$"0000000000000000000000000000000000FF2BFC2A2AFFFF2A2A2A2A2A002BFF"
	$"0000000000000000000000000000000000FF2BFC2AFF05FF2A2A2A2A2A002BFF"
	$"0000000000000000000000000000000000FF2BFFFF05FF2A2A2A2A2A2A002BFF"
	$"0000000000000000000000000000000000FFFF0505FF2A2A2A2A2A2A2A002BFF"
	$"0000000000000000000000000000000000FF0505FF2A2A2A2A2A2A2A2A002BFF"
	$"000000000000000000000000000000FFFF0505FF2A2A2A2A2A2A2A2A2A002BFF"
	$"0000000000000000000000000000FF050505FFFC000000000000000000002BFF"
	$"00000000000000000000000000FF050505FFFFFFFFFF2B2B2B2B2B2B2B2B2BFF"
	$"0000000000000000000000FFFF050505050505050505FF2B2B2B2B2B2B2B2BFF"
	$"00FFFFFFFFFFFFFFFFFFFF050505050505050505FFFF2B2B2BFFFFFFFFFF2BFF"
	$"FF2B2B2B2B2B2B2B2B2B2BFFFFFFFFFF050505FFD82B2B2B2B2B2B2B2B2B2BFF"
	$"FF2BFCFCFCFCFCFCFCFCFCFCFC2BFF050505FF2B2B2B2B2B2B2B2B2B2B2B2BFF"
	$"FF2BFC2A2A2A2A2A2A2A2A2A00FF0505FFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00"
	$"FF2BFC2A2A2A2A2A2A2A2A2AFF0505FF0000FF2BF7F8F9FAFAFBFBFCFCFDFF00"
	$"FF2BFC2A2A2A2A2A2A2A2AFF0505FF000000FFFFFFFFFFFFFFFFFFFFFFFFFF00"
	$"FF2BFC2A2A2A2A2A2A2AFF05FFFFFF0000000000000000000000000000000000"
	$"FF2BFC2A2A2A2A2A2AFF05FF002BFF0000000000000000000000000000000000"
	$"FF2BFC2A2A2A2A2A2AFFFF2A002BFF0000000000000000000000000000000000"
	$"FF2BFC2A2A2A2A2A2A2A2A2A002BFF0000000000000000000000000000000000"
	$"FF2BFC000000000000000000002BFF0000000000000000000000000000000000"
	$"FF2B2B2B2B2B2B2B2B2B2B2B2B2BFF0000000000000000000000000000000000"
	$"FF2B2B2B2B2B2B2B2B2B2B2B2B2BFF0000000000000000000000000000000000"
	$"FF2BE3E32B2B2B2BFFFFFFFFFF2BFF0000000000000000000000000000000000"
	$"FF2BD8D82B2B2B2B2B2B2B2B2B2BFF0000000000000000000000000000000000"
	$"FF2B2B2B2B2B2B2B2B2B2B2B2B2BFF0000000000000000000000000000000000"
	$"00FFFFFFFFFFFFFFFFFFFFFFFFFF000000000000000000000000000000000000"
	$"00FF2BF7F8F9FAFAFBFBFCFCFDFF000000000000000000000000000000000000"
	$"00FFFFFFFFFFFFFFFFFFFFFFFFFF"
};
resource 'ics#' (FREF_APPL, purgeable) {
	{	/* array: 2 elements */
		/* [1] */
		$"00FF 0081 008D 0035 00D5 0325 F441 822D"
		$"B4C1 AB3E AC00 B100 8100 8D00 8100 7E",
		/* [2] */
		$"00FF 00FF 00FF 00FF 00FF 03FF FFFF FFFF"
		$"FFFF FF7E FF00 FF00 FF00 FF00 FF00 7E"
	}
};
resource 'ics4' (FREF_APPL) {
	$"00000000FFFFFFFF"
	$"00000000FCCCCCCF"
	$"00000000FCEEEECF"
	$"00000000FCFFC0CF"
	$"00000000FF1FC0CF"
	$"000000FF11F000CF"
	$"FFFFFF111FCCCCCF"
	$"FCCCCCF111FCFFCF"
	$"FCEEEF11FFCCCCCF"
	$"FCECF1FF0FFFFFF0"
	$"FCECFFCF00000000"
	$"FCE000CF00000000"
	$"FCCCCCCF00000000"
	$"FCCCFFCF00000000"
	$"FCCCCCCF00000000"
	$"0FFFFFF0"
};
resource 'ics8' (FREF_APPL) {
	$"0000000000000000FFFFFFFFFFFFFFFF"
	$"0000000000000000FF2B2B2B2B2B2BFF"
	$"0000000000000000FF2BFCFCFCFC2BFF"
	$"0000000000000000FF2BFFFF2A002BFF"
	$"0000000000000000FFFF05FF2A002BFF"
	$"000000000000FFFF0505FF0000002BFF"
	$"FFFFFFFFFFFF050505FF2B2B2B2B2BFF"
	$"FF2B2B2B2B2BFF050505FF2BFFFF2BFF"
	$"FF2BFCFCFCFF0505FFFF2B2B2B2B2BFF"
	$"FF2BFC2AFF05FFFF00FFFFFFFFFFFF00"
	$"FF2BFC2AFFFF2BFF0000000000000000"
	$"FF2BFC0000002BFF0000000000000000"
	$"FF2B2B2B2B2B2BFF0000000000000000"
	$"FF2B2B2BFFFF2BFF0000000000000000"
	$"FF2B2B2B2B2B2BFF0000000000000000"
	$"00FFFFFFFFFFFF"
};

/* Saved-session icon */

resource 'ICN#' (FREF_Sess) {
	{	/* array: 2 elements */
		/* [1] */
		$"1FFFFC00 10000600 10200500 103FFC80"
		$"10200440 10000420 17AAAFF0 12000510"
		$"12201A10 12002510 12204810 12019510"
		$"12222210 12047FD0 12380290 12200D90"
		$"123F1090 12022190 1224C090 12090190"
		$"12128090 122C4190 12504090 177555D0"
		$"10000010 10400110 107FFF10 10400110"
		$"10000010 10000010 10000010 1FFFFFF0",
		/* [2] */
		$"1FFFFC00 1FFFFE00 1FFFFF00 1FFFFF80"
		$"1FFFFFC0 1FFFFFE0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
	}
};
resource 'icl4' (FREF_Sess) {
	$"000FFFFFFFFFFFFFFFFFFF0000000000"
	$"000F0C0C0C0C0C0C0C0C0FF000000000"
	$"000FC0C0C0F0C0C0C0C0CFCF00000000"
	$"000F0C0C0CFFFFFFFFFFFFCCF0000000"
	$"000FC0C0C0F0C0C0C0C0CFCCCF000000"
	$"000F0C0C0CDC0C0C0C0C0FCCCCF00000"
	$"000FCFFFDDDDDDDDDDDDDFFFFFFF0000"
	$"000F0CFC0CDC0C0C0C0C0F1F0C0F0000"
	$"000FC0F0C0D0C0C0C0CFF1FDC0CF0000"
	$"000F0CFC0CDC0C0C0CF11F0D0C0F0000"
	$"000FC0F0C0D0C0C0CF11F0CDC0CF0000"
	$"000F0CFC0CDC0C0FF11F0F0D0C0F0000"
	$"000FC0F0C0D0C0F111F0C0FDC0CF0000"
	$"000F0CFC0CDC0F111FFFFFFFFF0F0000"
	$"000FC0F0C0DFF111111111FDF0CF0000"
	$"000F0CFC0CF111111111FF0DFC0F0000"
	$"000FC0F0C0DFFFFF111FC0CDF0CF0000"
	$"000F0CFC0CDC0CF111FC0C0DFC0F0000"
	$"000FC0F0C0C0CF11FFC0C0CDF0CF0000"
	$"000F0CFC0C0CF11F0C0C0C0DFC0F0000"
	$"000FC0F0C0CF11F0F0C0C0CDF0CF0000"
	$"000F0CFC0CF1FF0C0F0C0C0DFC0F0000"
	$"000FC0F0CF1FC0C0CEC0C0CDF0CF0000"
	$"000F0FFFDFFDDDDDDEDDDDDFFF0F0000"
	$"000FC0C0CDC0C0C0C0C0C0CDC0CF0000"
	$"000F0C0C0F0C0C0C0C0C0C0F0C0F0000"
	$"000FC0C0CFFFFFFFFFFFFFFFC0CF0000"
	$"000F0C0C0F0C0C0C0C0C0C0F0C0F0000"
	$"000FC0C0C0C0C0C0C0C0C0C0C0CF0000"
	$"000F0C0C0C0C0C0C0C0C0C0C0C0F0000"
	$"000FC0C0C0C0C0C0C0C0C0C0C0CF0000"
	$"000FFFFFFFFFFFFFFFFFFFFFFFFF"
};
resource 'icl8' (FREF_Sess, purgeable) {
	$"000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000000000000000"
	$"000000FFF5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5FFFF000000000000000000"
	$"000000FFF5F5F5F5F5F5FFF5F5F5F5F5F5F5F5F5F5FF2BFF0000000000000000"
	$"000000FFF5F5F5F5F5F5FFFFFFFFFFFFFFFFFFFFFFFF2B2BFF00000000000000"
	$"000000FFF5F5F5F5F5F5FFF5F5F5F5F5F5F5F5F5F5FF2B2B2BFF000000000000"
	$"000000FFF5F5F5F5F5F5F9F5F5F5F5F5F5F5F5F5F5FF2B2B2B2BFF0000000000"
	$"000000FFF5FFFFFFF9F9F9F9F9F9F9F9F9F9F9F9F9FFFFFFFFFFFFFF00000000"
	$"000000FFF5F5FFF5F5F5F9F5F5F5F5F5F5F5F5F5F5FF05FFF5F5F5FF00000000"
	$"000000FFF5F5FFF5F5F5F9F5F5F5F5F5F5F5F5FFFF05FFF9F5F5F5FF00000000"
	$"000000FFF5F5FFF5F5F5F9F5F5F5F5F5F5F5FF0505FFF5F9F5F5F5FF00000000"
	$"000000FFF5F5FFF5F5F5F9F5F5F5F5F5F5FF0505FFF5F5F9F5F5F5FF00000000"
	$"000000FFF5F5FFF5F5F5F9F5F5F5F5FFFF0505FFF5FFF5F9F5F5F5FF00000000"
	$"000000FFF5F5FFF5F5F5F9F5F5F5FF050505FFF5F5F5FFF9F5F5F5FF00000000"
	$"000000FFF5F5FFF5F5F5F9F5F5FF050505FFFFFFFFFFFFFFFFFFF5FF00000000"
	$"000000FFF5F5FFF5F5F5F9FFFF050505050505050505FFF9FFF5F5FF00000000"
	$"000000FFF5F5FFF5F5F5FF050505050505050505FFFFF5F9FFF5F5FF00000000"
	$"000000FFF5F5FFF5F5F5F9FFFFFFFFFF050505FFF5F5F5F9FFF5F5FF00000000"
	$"000000FFF5F5FFF5F5F5F9F5F5F5FF050505FFF5F5F5F5F9FFF5F5FF00000000"
	$"000000FFF5F5FFF5F5F5F5F5F5FF0505FFFFF5F5F5F5F5F9FFF5F5FF00000000"
	$"000000FFF5F5FFF5F5F5F5F5FF0505FFF5F5F5F5F5F5F5F9FFF5F5FF00000000"
	$"000000FFF5F5FFF5F5F5F5FF0505FFF5FFF5F5F5F5F5F5F9FFF5F5FF00000000"
	$"000000FFF5F5FFF5F5F5FF05FFFFF5F5F5FCF5F5F5F5F5F9FFF5F5FF00000000"
	$"000000FFF5F5FFF5F5FF05FFF5F5F5F5F5FCF5F5F5F5F5F9FFF5F5FF00000000"
	$"000000FFF5FFFFFFF9FFFFF9F9F9F9F9F9FCF9F9F9F9F9FFFFFFF5FF00000000"
	$"000000FFF5F5F5F5F5F9F5F5F5F5F5F5F5F5F5F5F5F5F5F9F5F5F5FF00000000"
	$"000000FFF5F5F5F5F5FFF5F5F5F5F5F5F5F5F5F5F5F5F5FFF5F5F5FF00000000"
	$"000000FFF5F5F5F5F5FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5F5F5FF00000000"
	$"000000FFF5F5F5F5F5FFF5F5F5F5F5F5F5F5F5F5F5F5F5FFF5F5F5FF00000000"
	$"000000FFF5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5FF00000000"
	$"000000FFF5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5FF00000000"
	$"000000FFF5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5FF00000000"
	$"000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
};
resource 'ics#' (FREF_Sess, purgeable) {
	{	/* array: 2 elements */
		/* [1] */
		$"7FE0 4030 4028 403C 5AB4 50D4 5334 5444"
		$"5234 54C4 5B14 5544 4814 4FF4 4004 7FFC",
		/* [2] */
		$"7FE0 7FF0 7FF8 7FFC 7FFC 7FFC 7FFC 7FFC"
		$"7FFC 7FFC 7FFC 7FFC 7FFC 7FFC 7FFC 7FFC"
	}
};
resource 'ics4' (FREF_Sess) {
	$"0FFFFFFFFFF00000"
	$"0F0C0C0C0CFF0000"
	$"0FC0C0C0C0FCF000"
	$"0F0C0C0C0CFFFF00"
	$"0FCFDDDDDDFFCF00"
	$"0F0F0C0CFF1F0F00"
	$"0FCFC0FF11FDCF00"
	$"0F0F0F111F0D0F00"
	$"0FCFC0F111FDCF00"
	$"0F0F0F11FF0D0F00"
	$"0FCFF1FFC0CDCF00"
	$"0F0FCFDDDDDD0F00"
	$"0FC0F0C0C0CFCF00"
	$"0F0CFFFFFFFF0F00"
	$"0FC0C0C0C0C0CF00"
	$"0FFFFFFFFFFFFF"
};
resource 'ics8' (FREF_Sess) {
	$"00FFFFFFFFFFFFFFFFFFFF0000000000"
	$"00FFF5F5F5F5F5F5F5F5FFFF00000000"
	$"00FFF5F5F5F5F5F5F5F5FF2BFF000000"
	$"00FFF5F5F5F5F5F5F5F5FFFFFFFF0000"
	$"00FFF5FFF9F9F9F9F9F9FFFFF5FF0000"
	$"00FFF5FFF5F5F5F5FFFF05FFF5FF0000"
	$"00FFF5FFF5F5FFFF0505FFF9F5FF0000"
	$"00FFF5FFF5FF050505FFF5F9F5FF0000"
	$"00FFF5FFF5F5FF050505FFF9F5FF0000"
	$"00FFF5FFF5FF0505FFFFF5F9F5FF0000"
	$"00FFF5FFFF05FFFFF5F5F5F9F5FF0000"
	$"00FFF5FFF8FFF9F9F9F9F9F9F5FF0000"
	$"00FFF5F5FFF5F5F5F5F5F5FFF5FF0000"
	$"00FFF5F5FFFFFFFFFFFFFFFFF5FF0000"
	$"00FFF5F5F5F5F5F5F5F5F5F5F5FF0000"
	$"00FFFFFFFFFFFFFFFFFFFFFFFFFF"
};

/* Saved session stationery pad icon */
resource 'ICN#' (FREF_Sesss, purgeable) {
	{	/* array: 2 elements */
		/* [1] */
		$"3FFFFFE0 20000020 20400238 207FFE28"
		$"20400228 20000028 2F555628 24000A28"
		$"24403428 24004A28 24409028 24032A28"
		$"24444428 2408FFA8 24700528 24401B28"
		$"247E2128 24044328 24498128 24120328"
		$"24250128 24588328 24A08128 2EEAAFE8"
		$"20000848 20800888 20FFF908 20800A08"
		$"20000C08 3FFFF808 08000008 0FFFFFF8",
		/* [2] */
		$"3FFFFFE0 3FFFFFE0 3FFFFFF8 3FFFFFF8"
		$"3FFFFFF8 3FFFFFF8 3FFFFFF8 3FFFFFF8"
		$"3FFFFFF8 3FFFFFF8 3FFFFFF8 3FFFFFF8"
		$"3FFFFFF8 3FFFFFF8 3FFFFFF8 3FFFFFF8"
		$"3FFFFFF8 3FFFFFF8 3FFFFFF8 3FFFFFF8"
		$"3FFFFFF8 3FFFFFF8 3FFFFFF8 3FFFFFF8"
		$"3FFFFFF8 3FFFFFF8 3FFFFFF8 3FFFFFF8"
		$"3FFFFFF8 3FFFFFF8 0FFFFFF8 0FFFFFF8"
	}
};
resource 'icl4' (FREF_Sesss, purgeable) {
	$"00FFFFFFFFFFFFFFFFFFFFFFFFF00000"
	$"00FC0C0C0C0C0C0C0C0C0C0C0CF00000"
	$"00F0C0C0CFC0C0C0C0C0C0F0C0FFF000"
	$"00FC0C0C0FFFFFFFFFFFFFFC0CFDF000"
	$"00F0C0C0CFC0C0C0C0C0C0F0C0FDF000"
	$"00FC0C0C0D0C0C0C0C0C0CDC0CFDF000"
	$"00F0FFFDDDDDDDDDDDDDDFF0C0FDF000"
	$"00FC0F0C0D0C0C0C0C0CF1FC0CFDF000"
	$"00F0CFC0CDC0C0C0C0FF1FD0C0FDF000"
	$"00FC0F0C0D0C0C0C0F11FCDC0CFDF000"
	$"00F0CFC0CDC0C0C0F11FC0D0C0FDF000"
	$"00FC0F0C0D0C0CFF11FCFCDC0CFDF000"
	$"00F0CFC0CDC0CF111FC0CFD0C0FDF000"
	$"00FC0F0C0D0CF111FFFFFFFFFCFDF000"
	$"00F0CFC0CDFF111111111FDFC0FDF000"
	$"00FC0F0C0F111111111FFCDF0CFDF000"
	$"00F0CFC0CDFFFFF111F0C0DFC0FDF000"
	$"00FC0F0C0D0C0F111F0C0CDF0CFDF000"
	$"00F0CFC0C0C0F11FF0C0C0DFC0FDF000"
	$"00FC0F0C0C0F11FC0C0C0CDF0CFDF000"
	$"00F0CFC0C0F11FCFC0C0C0DFC0FDF000"
	$"00FC0F0C0F1FFC0CFC0C0CDF0CFDF000"
	$"00F0CFC0F1F0C0C0E0C0C0DFC0FDF000"
	$"00FCFFFDFFDDDDDDEDDDFFFFFFFDF000"
	$"00F0C0C0D0C0C0C0C0C0FCCCCFDDF000"
	$"00FC0C0CFC0C0C0C0C0CFCCCFDDCF000"
	$"00F0C0C0FFFFFFFFFFFFFCCFDDCCF000"
	$"00FC0C0CFC0C0C0C0C0CFCFDDCCCF000"
	$"00F0C0C0C0C0C0C0C0C0FFDDCCCCF000"
	$"00FFFFFFFFFFFFFFFFFFFDDCCCCCF000"
	$"0000FDDDDDDDDDDDDDDDDDCCCCCCF000"
	$"0000FFFFFFFFFFFFFFFFFFFFFFFFF0"
};
resource 'icl8' (FREF_Sesss, purgeable) {
	$"0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF0000000000"
	$"0000FFF5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5FF0000000000"
	$"0000FFF5F5F5F5F5F5FFF5F5F5F5F5F5F5F5F5F5F5F5FFF5F5F5FFFFFF000000"
	$"0000FFF5F5F5F5F5F5FFFFFFFFFFFFFFFFFFFFFFFFFFFFF5F5F5FFF9FF000000"
	$"0000FFF5F5F5F5F5F5FFF5F5F5F5F5F5F5F5F5F5F5F5FFF5F5F5FFF9FF000000"
	$"0000FFF5F5F5F5F5F5F9F5F5F5F5F5F5F5F5F5F5F5F5F9F5F5F5FFF9FF000000"
	$"0000FFF5FFFFFFF9F9F9F9F9F9F9F9F9F9F9F9F9F9FFFFF5F5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F9F5F5F5F5F5F5F5F5F5F5FF05FFF5F5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F9F5F5F5F5F5F5F5F5FFFF05FFF9F5F5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F9F5F5F5F5F5F5F5FF0505FFF5F9F5F5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F9F5F5F5F5F5F5FF0505FFF5F5F9F5F5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F9F5F5F5F5FFFF0505FFF5FFF5F9F5F5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F9F5F5F5FF050505FFF5F5F5FFF9F5F5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F9F5F5FF050505FFFFFFFFFFFFFFFFFFF5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F9FFFF050505050505050505FFF9FFF5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5FF050505050505050505FFFFF5F9FFF5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F9FFFFFFFFFF050505FFF5F5F5F9FFF5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F9F5F5F5FF050505FFF5F5F5F5F9FFF5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F5F5F5FF0505FFFFF5F5F5F5F5F9FFF5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F5F5FF0505FFF5F5F5F5F5F5F5F9FFF5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5F5FF0505FFF5FFF5F5F5F5F5F5F9FFF5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5F5FF05FFFFF5F5F5FCF5F5F5F5F5F9FFF5F5FFF9FF000000"
	$"0000FFF5F5FFF5F5FF05FFF5F5F5F5F5FCF5F5F5F5F5F9FFF5F5FFF9FF000000"
	$"0000FFF5FFFFFFF9FFFFF9F9F9F9F9F9FCF9F9F9FFFFFFFFFFFFFFF9FF000000"
	$"0000FFF5F5F5F5F5F9F5F5F5F5F5F5F5F5F5F5F5FF2B2B2BF7FFF9F7FF000000"
	$"0000FFF5F5F5F5F5FFF5F5F5F5F5F5F5F5F5F5F5FF2B2BF7FFF9F72BFF000000"
	$"0000FFF5F5F5F5F5FFFFFFFFFFFFFFFFFFFFFFFFFF2BF7FFF9F72BF6FF000000"
	$"0000FFF5F5F5F5F5FFF5F5F5F5F5F5F5F5F5F5F5FFF7FFF9F72BF6F6FF000000"
	$"0000FFF5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5FFFFF9F72BF6F6F6FF000000"
	$"0000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF9F72BF6F6F6F6FF000000"
	$"00000000FFF9F9F9F9F9F9F9F9F9F9F9F9F9F9F9F9F72BF6F6F6F6F6FF000000"
	$"00000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
};
resource 'ics#' (FREF_Sesss, purgeable) {
	{	/* array: 2 elements */
		/* [1] */
		$"7FF8 4008 756E 61AA 666A 688A 646A 698A"
		$"762A 6A8A 507A 5FD2 4062 7FC2 1002 1FFE",
		/* [2] */
		$"7FF8 7FF8 7FFE 7FFE 7FFE 7FFE 7FFE 7FFE"
		$"7FFE 7FFE 7FFE 7FFE 7FFE 7FFE 1FFE 1FFE"
	}
};
resource 'ics4' (FREF_Sesss, purgeable) {
	$"0FFFFFFFFFFFF000"
	$"0FC0C0C0C0C0F000"
	$"0FFDDDDDDFFCFFF0"
	$"0FF0C0CFF1F0FDF0"
	$"0FFC0FF11FDCFDF0"
	$"0FF0F111F0D0FDF0"
	$"0FFC0F111FDCFDF0"
	$"0FF0F11FF0D0FDF0"
	$"0FFF1FFC0CDCFDF0"
	$"0FFCFDDDDDD0FDF0"
	$"0F0F0C0C0FFFFDF0"
	$"0FCFFFFFFFCFDDF0"
	$"0F0C0C0C0FFDDCF0"
	$"0FFFFFFFFFDDC0F0"
	$"000FDDDDDDDC0CF0"
	$"000FFFFFFFFFFFF0"
};
resource 'ics8' (FREF_Sesss, purgeable) {
	$"00FFFFFFFFFFFFFFFFFFFFFFFF000000"
	$"00FFF5F5F5F5F5F5F5F5F5F5FF000000"
	$"00FFFFF9F9F9F9F9F9FFFFF5FFFFFF00"
	$"00FFFFF5F5F5F5FFFF05FFF5FFF9FF00"
	$"00FFFFF5F5FFFF0505FFF9F5FFF9FF00"
	$"00FFFFF5FF050505FFF5F9F5FFF9FF00"
	$"00FFFFF5F5FF050505FFF9F5FFF9FF00"
	$"00FFFFF5FF0505FFFFF5F9F5FFF9FF00"
	$"00FFFFFF05FFFFF5F5F5F9F5FFF9FF00"
	$"00FFFFF8FFF9F9F9F9F9F9F5FFF9FF00"
	$"00FFF5FFF5F5F5F5F5FFFFFFFFF9FF00"
	$"00FFF5FFFFFFFFFFFFFF2BFFF9F9FF00"
	$"00FFF5F5F5F5F5F5F5FFFFF9F9F5FF00"
	$"00FFFFFFFFFFFFFFFFFFF9F9F5F5FF00"
	$"000000FFF9F9F9F9F9F9F9F5F5F5FF00"
	$"000000FFFFFFFFFFFFFFFFFFFFFFFF"
};

/* Known hosts icon */
resource 'ICN#' (FREF_HKey, purgeable) {
	{	/* array: 2 elements */
		/* [1] */
		$"1FFFFC00 10000600 10000500 1FFFFC80"
		$"10000440 10000420 1FFFFFF0 10000010"
		$"13FC0F90 1C03F0F0 15FA8090 150A8090"
		$"1D0B80F0 150A8050 15FA8050 1C038070"
		$"143A8050 14028050 1FFFABF0 12048110"
		$"13FCFF10 1AAAAAB0 10000010 17FFFFD0"
		$"14000050 15252250 15555550 15252250"
		$"14000050 17FFFFD0 10000010 1FFFFFF0",
		/* [2] */
		$"1FFFFC00 1FFFFE00 1FFFFF00 1FFFFF80"
		$"1FFFFFC0 1FFFFFE0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
	}
};
resource 'icl4' (FREF_HKey, purgeable) {
	$"000FFFFFFFFFFFFFFFFFFF0000000000"
	$"000F00000000000000000FF000000000"
	$"000F00000000000000000FCF00000000"
	$"000FFFFFFFFFFFFFFFFFFFCCF0000000"
	$"000F00000000000000000FCCCF000000"
	$"000F00000000000000000FCCCCF00000"
	$"000FFFFFFFFFFFFFFFFFFFFFFFFF0000"
	$"000F00000000000000000000000F0000"
	$"000F00FFFFFFFF000000FFFFF00F0000"
	$"000FFFCCCCCCCCFFFFFFCCCCFFFF0000"
	$"000F0FCEEEEECCF0FCCCCCCCF00F0000"
	$"000F0FCE0D0D0CF0FCCCCCCCF00F0000"
	$"000FFFCED0D0CCFFFCCCCCCCFFFF0000"
	$"000F0FCE0D0D0CF0FCCCCCCCCF0F0000"
	$"000F0FCCC0C0CCF0FCCCCCCCCF0F0000"
	$"000FFFCCCCCCCCFFFCCCCCCCCFFF0000"
	$"000F0FCCCCFFFCF0FCCCCCCCCF0F0000"
	$"000F0FCCCCCCCCF0FCCCCCCCCF0F0000"
	$"000FFFFFFFFFFFFFFDDDDDDFFFFF0000"
	$"000F00FCCDDEEF00FDDDDDDF000F0000"
	$"000F00FFFFFFFF00FFFFFFFF000F0000"
	$"000F0C0C0C0C0C0C0C0C0C0C0C0F0000"
	$"000FC0C0C0C0C0C0C0C0C0C0C0CF0000"
	$"000F0FFFFFFFFFFFFFFFFFFFFF0F0000"
	$"000FCF0000000000000000000FCF0000"
	$"000F0F0F00F00F0F00F000F00F0F0000"
	$"000FCF0F0F0F0F0F0F0F0F0F0FCF0000"
	$"000F0F0F00F00F0F00F000F00F0F0000"
	$"000FCF0000000000000000000FCF0000"
	$"000F0FFFFFFFFFFFFFFFFFFFFF0F0000"
	$"000FC0C0C0C0C0C0C0C0C0C0C0CF0000"
	$"000FFFFFFFFFFFFFFFFFFFFFFFFF"
};
resource 'icl8' (FREF_HKey, purgeable) {
	$"000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000000000000000"
	$"000000FF0000000000000000000000000000000000FFFF000000000000000000"
	$"000000FF0000000000000000000000000000000000FFF6FF0000000000000000"
	$"000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF6F6FF00000000000000"
	$"000000FF0000000000000000000000000000000000FFF6F6F6FF000000000000"
	$"000000FF0000000000000000000000000000000000FFF6F6F6F6FF0000000000"
	$"000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000"
	$"000000FF0000000000000000000000000000000000000000000000FF00000000"
	$"000000FF0000FFFFFFFFFFFFFFFF000000000000FFFFFFFFFF0000FF00000000"
	$"000000FFFFFF2B2B2B2B2B2B2B2BFFFFFFFFFFFF2B2B2B2BFFFFFFFF00000000"
	$"000000FF00FF2BFCFCFCFCFCF82BFF00FF2B2B2B2B2B2B2BFF0000FF00000000"
	$"000000FF00FF2BFC2A2A2A2A002BFF00FF2B2B2B2B2B2B2BFF0000FF00000000"
	$"000000FFFFFF2BFC2A2A2A2A002BFFFFFF2B2B2B2B2B2B2BFFFFFFFF00000000"
	$"000000FF00FF2BFC2A2A2A2A002BFF00FF2B2B2B2B2B2B2B2BFF00FF00000000"
	$"000000FF00FF2BF800000000002BFF00FF2B2B2B2B2B2B2B2BFF00FF00000000"
	$"000000FFFFFF2B2B2B2B2B2B2B2BFFFFFF2B2B2B2B2B2B2B2BFFFFFF00000000"
	$"000000FF00FF2B2B2B2BFFFFFF2BFF00FF2B2B2B2B2B2B2B2BFF00FF00000000"
	$"000000FF00FF2B2B2B2B2B2B2B2BFF00FF2B2B2B2B2B2B2B2BFF00FF00000000"
	$"000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFF9F9F9F9F9F9FFFFFFFFFF00000000"
	$"000000FF0000FFF7F8F9FAFBFCFF0000FFF9F9F9F9F9F9FF000000FF00000000"
	$"000000FF0000FFFFFFFFFFFFFFFF0000FFFFFFFFFFFFFFFF000000FF00000000"
	$"000000FFF5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5FF00000000"
	$"000000FFF5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5FF00000000"
	$"000000FFF5FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5FF00000000"
	$"000000FFF5FF00000000000000000000000000000000000000FFF5FF00000000"
	$"000000FFF5FF00FF0000FF0000FF00FF0000FF000000FF0000FFF5FF00000000"
	$"000000FFF5FF00FF00FF00FF00FF00FF00FF00FF00FF00FF00FFF5FF00000000"
	$"000000FFF5FF00FF0000FF0000FF00FF0000FF000000FF0000FFF5FF00000000"
	$"000000FFF5FF00000000000000000000000000000000000000FFF5FF00000000"
	$"000000FFF5FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF5FF00000000"
	$"000000FFF5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5F5FF00000000"
	$"000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
};

/* Random seed icon */

resource 'ICN#' (FREF_Seed, purgeable) {
	{	/* array: 2 elements */
		/* [1] */
		$"1FFFFC00 18F36600 161EF500 1CC92C80"
		$"1CF2EC40 10662C20 108E07F0 151F0490"
		$"1E00C4F0 1803BBD0 1FC5BE10 108B5A90"
		$"1B3C4F50 1267AC90 14B60470 1BB791B0"
		$"17F4D2B0 1DC1F830 1B029450 1B753DD0"
		$"145A8170 11390DD0 1E15A8B0 1CC4CD90"
		$"154ECED0 15C9CF30 172CDB50 12617970"
		$"15E45C90 1D4B9890 15CE4430 1FFFFFF0",
		/* [2] */
		$"1FFFFC00 1FFFFE00 1FFFFF00 1FFFFF80"
		$"1FFFFFC0 1FFFFFE0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
		$"1FFFFFF0 1FFFFFF0 1FFFFFF0 1FFFFFF0"
	}
};
resource 'icl4' (FREF_Seed) {
	$"000FFFFFFFFFFFFFFFFFFF0000000000"
	$"000FFC0CFFFF0CFF1FFC0FF000000000"
	$"000F0FF0C0CFFFF1FFFFCFCF00000000"
	$"000FFF0CFF0CF11F0CFCFFCCF0000000"
	$"000FFFC0FFFF11F0FFF0FFCCCF000000"
	$"000F0C0C0FF11FFC0CFCFFCCCCF00000"
	$"000FC0C0F111FFF0C0C0CFFFFFFF0000"
	$"000F0F0F111FFFFF0C0C0F0CFC0F0000"
	$"000FFFF111111111FFC0CFC0FFFF0000"
	$"000FF111111111FFFCFFF0FFFF0F0000"
	$"000FFFFFFF111FCFF0FFFFF0C0CF0000"
	$"000F0C0CF111FCFF0F0FFCFCFC0F0000"
	$"000FF0FF11FFFFC0CFC0FFFFCFCF0000"
	$"000F0CF11FFC0FFFFCFCFF0CFC0F0000"
	$"000FCF11F0FFCFF0C0C0CFC0CFFF0000"
	$"000FF1FFFCFF0FFFFC0F0C0FFCFF0000"
	$"000F1FFFFFFFCFC0FFCFC0F0F0FF0000"
	$"000FFF0FFF0C0C0FFFFFFC0C0CFF0000"
	$"000FF0FFC0C0C0F0F0CF0FC0CFCF0000"
	$"000FFCFF0FFF0F0F0CFFFF0FFF0F0000"
	$"000FCFC0CF0FF0F0F0C0C0CFCFFF0000"
	$"000F0C0F0CFFFC0F0C0CFF0FFF0F0000"
	$"000FFFF0C0CFCFCFF0F0F0C0F0FF0000"
	$"000FFF0CFF0C0F0CFF0CFF0FFC0F0000"
	$"000FCFCF0FC0FFF0FFC0FFF0FFCF0000"
	$"000F0F0FFF0CFC0FFF0CFFFF0CFF0000"
	$"000FCFFFC0F0FFC0FFCFF0FFCFCF0000"
	$"000F0CFC0FFC0C0F0FFFFC0F0FFF0000"
	$"000FCFCFFFF0CFC0CFCFFFC0F0CF0000"
	$"000FFF0F0F0CF0FFFC0FFC0CFC0F0000"
	$"000FCFCFFFC0FFF0CFC0CFC0C0FF0000"
	$"000FFFFFFFFFFFFFFFFFFFFFFFFF"
};
resource 'icl8' (FREF_Seed) {
	$"000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000000000000000"
	$"000000FFFFF5F5F5FFFFFFFFF5F5FFFF05FFFFF5F5FFFF000000000000000000"
	$"000000FFF5FFFFF5F5F5F5FFFFFFFF05FFFFFFFFF5FF2BFF0000000000000000"
	$"000000FFFFFFF5F5FFFFF5F5FF0505FF0000FFF5FFFF2B2BFF00000000000000"
	$"000000FFFFFFF5F5FFFFFFFF0505FFF5FFFFFFF5FFFF2B2B2BFF000000000000"
	$"000000FFF5F5F5F5F5FFFF0505FFFFF5F5F5FFF5FFFF2B2B2B2BFF0000000000"
	$"000000FFF5F5F5F5FF050505FFFFFFF5F5F5F5F5F5FFFFFFFFFFFFFF00000000"
	$"000000FFF5FFF5FF050505FFFFFFFFFFF5F5F5F5F5FFF5F5FFF5F5FF00000000"
	$"000000FFFFFFFF050505050505050505FFFFF5F5F5FFF5F5FFFFFFFF00000000"
	$"000000FFFF050505050505050505FFFFFFF5FFFFFFF5FFFFFFFFF5FF00000000"
	$"000000FFFFFFFFFFFFFF050505FFF5FFFFF5FFFFFFFFFFF5F5F5F5FF00000000"
	$"000000FFF5F5F5F5FF050505FFF5FFFFF5FFF5FFFFF5FFF5FFF5F5FF00000000"
	$"000000FFFFF5FFFF0505FFFFFFFFF5F5F5FFF5F5FFFFFFFFF5FFF5FF00000000"
	$"000000FFF5F5FF0505FFFFF5F5FFFFFFFFF5FFF5FFFFF5F5FFF5F5FF00000000"
	$"000000FFF5FF0505FFF5FFFFF5FFFFF5F5F5F5F5F5FFF5F5F5FFFFFF00000000"
	$"000000FFFF05FFFFFFF5FFFFF5FFFFFFFFF5F5FFF5F5F5FFFFF5FFFF00000000"
	$"000000FF05FFFFFFFFFFFFFFF5FFF5F5FFFFF5FFF5F5FFF5FFF5FFFF00000000"
	$"000000FFFFFFF5FFFFFFF5F5F5F5F5FFFFFFFFFFFFF5F5F5F5F5FFFF00000000"
	$"000000FFFFF5FFFFF5F5F5F5F5F5FF00FFF5F5FFF5FFF5F5F5FFF5FF00000000"
	$"000000FFFFF5FFFFF5FFFFFFF5FF00FFF5F5FFFFFFFFF5FFFFFFF5FF00000000"
	$"000000FFF5FFF5F5F5FFF5FFFF00FF00FFF5F5F5F5F5F5FFF5FFFFFF00000000"
	$"000000FFF5F5F5FFF5F5FFFFFF0000FFF5F5F5F5FFFFF5FFFFFF00FF00000000"
	$"000000FFFFFFFFF5F5F5F5FFF5FF00FFFFF5FFF5FFF5F5F5FF00FFFF00000000"
	$"000000FFFFFFF5F5FFFFF5F5F5FF0000FFFFF5F5FFFFF5FFFF0000FF00000000"
	$"000000FFF5FFF5FFF5FFF5F5FFFFFF00FFFFF5F5FFFFFFF5FFFF00FF00000000"
	$"000000FFF5FFF5FFFFFFF5F5FFF5F5FFFFFFF5F5FFFFFFFFF5F5FFFF00000000"
	$"000000FFF5FFFFFFF5F5FFF5FFFFF5F5FFFFF5FFFFF5FFFFF5FFF5FF00000000"
	$"000000FFF5F5FFF5F5FFFFF5F5F5F5FFF5FFFFFFFFF5F5FFF5FFFFFF00000000"
	$"000000FFF5FFF5FFFFFFFFF5F5FFF5F5F5FFF5FFFFFFF5F5FFF5F5FF00000000"
	$"000000FFFFFFF5FFF5FFF5F5FFF5FFFFFFF5F5FFFFF5F5F5FFF5F5FF00000000"
	$"000000FFF5FFF5FFFFFFF5F5FFFFFFF5F5FFF5F5F5FFF5F5F5F5FFFF00000000"
	$"000000FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
};
resource 'ics#' (FREF_Seed) {
	{	/* array: 2 elements */
		/* [1] */
		$"7FE0 56B0 59A8 637C 51DC 6794 59AC 76EC"
		$"7224 7C6C 743C 71AC 505C 459C 4424 7FFC",
		/* [2] */
		$"7FE0 7FF0 7FF8 7FFC 7FFC 7FFC 7FFC 7FFC"
		$"7FFC 7FFC 7FFC 7FFC 7FFC 7FFC 7FFC 7FFC"
	}
};
resource 'ics4' (FREF_Seed) {
	$"0FFFFFFFFFF00000"
	$"0F0F0FF1FCFF0000"
	$"0FCFF11FF0FCF000"
	$"0FF111FF0FFFFF00"
	$"0FCF111FFFCFFF00"
	$"0FF11FFFFC0F0F00"
	$"0F1FF0CFF0F0FF00"
	$"0FFF0FFCFFFCFF00"
	$"0FFFC0F0C0F0CF00"
	$"0FFFFF0C0FFCFF00"
	$"0FFFCFC0C0FFFF00"
	$"0FFF0C0FFCFCFF00"
	$"0FCFC0C0CFCFFF00"
	$"0F0C0F0FFC0FFF00"
	$"0FC0CFC0C0F0CF00"
	$"0FFFFFFFFFFFFF"
};
resource 'ics8' (FREF_Seed) {
	$"00FFFFFFFFFFFFFFFFFFFF0000000000"
	$"00FFF5FFF5FFFF05FFF5FFFF00000000"
	$"00FFF5FFFF0505FFFFF5FF2BFF000000"
	$"00FFFF050505FFFFF5FFFFFFFFFF0000"
	$"00FFF5FF050505FFFFFFF5FFFFFF0000"
	$"00FFFF0505FFFFFFFFF5F5FFF5FF0000"
	$"00FF05FFFFF5F5FFFFF5FFF5FFFF0000"
	$"00FFFFFFF5FFFFF5FFFFFFF5FFFF0000"
	$"00FFFFFFF5F5FFF5F5F5FFF5F5FF0000"
	$"00FFFFFFFFFFF5F5F5FFFFF5FFFF0000"
	$"00FFFFFFF5FFF5F5F5F5FFFFFFFF0000"
	$"00FFFFFFF5F5F5FFFFF5FFF5FFFF0000"
	$"00FFF5FFF5F5F5F5F5FFF5FFFFFF0000"
	$"00FFF5F5F5FFF5FFFFF5F5FFFFFF0000"
	$"00FFF5F5F5FFF5F5F5F5FFF5F5FF0000"
	$"00FFFFFFFFFFFFFFFFFFFFFFFFFF"
};

/*
 * Resources to copy into created files
 */

/*
 * Application-missing message string, for random seed and host key database
 * files.
 */
resource 'STR ' (-16397, purgeable) {
    "This file is used internally by PuTTY.  It cannot be opened."
};

/* Missing-application name string, for saved sessions. */
resource 'STR ' (-16396, purgeable) {
    "PuTTY"
};

/* ResEdit template resource for saved sessions. */

type 'TMPL' {
    array {
	pstring;
	literal longint;
    };
};

resource 'TMPL' (TMPL_Int, "Int ", purgeable) {
    { "Value", 'DLNG' }
};

/*
 * Internal resources
 */

/* Menu bar */

resource 'MBAR' (MBAR_Main, preload) {
    { mApple, mFile, mEdit, mWindow }
};

resource 'MENU' (mApple, preload) {
    mApple,
    textMenuProc,
    0b11111111111111111111111111111101,
    enabled,
    apple,
    {
	"About PuTTY\0xc9",	noicon, nokey, nomark, plain,
	"-",			noicon, nokey, nomark, plain,
    }
};

resource 'MENU' (mFile, preload) {
    mFile,
    textMenuProc,
    0b11111111111111111111111101111011,
    enabled,
    "Session",
    {
	"New",			noicon, "N",   nomark, plain,
	"Open\0xc9",		noicon, "O",   nomark, plain,
	"-",			noicon, nokey, nomark, plain,
	"Close",		noicon, "W",   nomark, plain,
	"Save",			noicon, "S",   nomark, plain,
	"Save As\0xc9",		noicon, nokey, nomark, plain,
	"Duplicate",		noicon, "D",   nomark, plain,
	"-",			noicon, nokey, nomark, plain,
	"Quit",			noicon, "Q",   nomark, plain,
    }
};

resource 'MENU' (mEdit, preload) {
    mEdit,
    textMenuProc,
    0b11111111111111111111111111111101,
    enabled,
    "Edit",
    {
	"Undo",			noicon, "Z",   nomark, plain,
	"-",			noicon, nokey, nomark, plain,
	"Cut",			noicon, "X",   nomark, plain,
	"Copy",			noicon, "C",   nomark, plain,
	"Paste",		noicon, "V",   nomark, plain,
	"Clear",		noicon, nokey, nomark, plain,
	"Select All",		noicon, "A",   nomark, plain,
    }
};

resource 'MENU' (mWindow, preload) {
    mWindow,
    textMenuProc,
    0b11111111111111111111111111111111,
    enabled,
    "Window",
    {
	"Show Event Log",	noicon, nokey, nomark, plain,
    }
};

/* Fatal error box.  Stolen from the Finder. */

resource 'ALRT' (wFatal, "fatalbox", purgeable) {
	{54, 67, 152, 435},
	wFatal,
	beepStages,
	alertPositionMainScreen
};

resource 'DITL' (wFatal, "fatalbox", purgeable) {
	{	/* array DITLarray: 3 elements */
		/* [1] */
		{68, 299, 88, 358},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{68, 227, 88, 286},
		StaticText {
			disabled,
			""
		},
		/* [3] */
		{7, 74, 55, 358},
		StaticText {
			disabled,
			"^0"
		}
	}
};

/* Caution box.  Stolen from the Finder. */

resource 'ALRT' (wQuestion, "questionbox", purgeable) {
	{54, 67, 152, 435},
	wQuestion,
	beepStages,
	alertPositionMainScreen
};

resource 'DITL' (wQuestion, "fatalbox", purgeable) {
	{	/* array DITLarray: 3 elements */
		/* [1] */
		{68, 299, 88, 358},
		Button {
			enabled,
			"OK"
		},
		/* [2] */
		{68, 227, 88, 286},
		Button {
			enabled,
			"Cancel"
		},
		/* [3] */
		{7, 74, 55, 358},
		StaticText {
			disabled,
			"^0"
		}
	}
};

/* Terminal window */

resource 'WIND' (wTerminal, "terminal", purgeable) {
    { 0, 0, 200, 200 },
    zoomDocProc,
    invisible,
    goAway,
    0x0,
    "untitled",
    staggerParentWindowScreen
};

resource 'CNTL' (cVScroll, "vscroll", purgeable) {
    { 0, 0, 48, 16 },
    0, invisible, 0, 0,
    scrollBarProc, 0, ""
};

/* Settings dialogue */

resource 'WIND' (wSettings, "settings", purgeable) {
    { 0, 0, 432, 626 },
    noGrowDocProc,
    invisible,
    goAway,
    0x0,
    "untitled",
    staggerParentWindowScreen
};

/* Event log */
resource 'WIND' (wEventLog, "event log", purgeable) {
    { 0, 0, 200, 200 },
    zoomDocProc,
    invisible,
    goAway,
    0x0,
    "PuTTY Event Log",
    staggerParentWindowScreen
};

/* "About" box */

resource 'DLOG' (wAbout, "about", purgeable) {
    { 0, 0, 120, 240 },
    noGrowDocProc,
    invisible,
    goAway,
    wAbout,		/* RefCon -- identifies the window to PuTTY */
    wAbout,		/* DITL ID */
    "About PuTTY",
    alertPositionMainScreen
};

resource 'dlgx' (wAbout, "about", purgeable) {
    versionZero {
	kDialogFlagsUseThemeBackground | kDialogFlagsUseThemeControls
    }
};

resource 'DITL' (wAbout, "about", purgeable) {
    {
	{ 87, 13, 107, 227 },
	Button { enabled, "View Licence" },
	{ 13, 13, 29, 227 },
	StaticText { disabled, "PuTTY"},
	{ 42, 13, 74, 227 },
	StaticText { disabled, "Some version or other\n"
			       "Copyright � 1997-9 Simon Tatham"},
    }
};

/* Licence box */

resource 'WIND' (wLicence, "licence", purgeable) {
    { 0, 0, 250, 400 },
    noGrowDocProc,
    visible,
    goAway,
    wLicence,
    "PuTTY Licence",
    alertPositionParentWindowScreen
};

type 'TEXT' {
    string;
};

resource 'TEXT' (wLicence, "licence", purgeable) {
    "PuTTY is copyright 1997-2004 Simon Tatham.\n"
    "\n"
    "Portions copyright Robert de Bath, Joris van Rantwijk, Delian"
    "Delchev, Andreas Schultz, Jeroen Massar, Wez Furlong, Nicolas Barry,"
    "Justin Bradford, Ben Harris, and CORE SDI S.A.\n"
    "\n"    
    "Permission is hereby granted, free of charge, to any person "
    "obtaining a copy of this software and associated documentation "
    "files (the \"Software\"), to deal in the Software without "
    "restriction, including without limitation the rights to use, "
    "copy, modify, merge, publish, distribute, sublicense, and/or "
    "sell copies of the Software, and to permit persons to whom the "
    "Software is furnished to do so, subject to the following "
    "conditions:\n\n"
    
    "The above copyright notice and this permission notice shall be "
    "included in all copies or substantial portions of the Software.\n\n"
    
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, "
    "EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF "
    "MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND "
    "NONINFRINGEMENT.  IN NO EVENT SHALL SIMON TATHAM BE LIABLE FOR "
    "ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF "
    "CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN "
    "CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
    "SOFTWARE."
};

/* Custom xDEFs */

data 'CDEF' (CDEF_EditBox) {
    $"4EF9 00000000"
};
data 'CDEF' (CDEF_Default) {
    $"4EF9 00000000"
};
data 'CDEF' (CDEF_ListBox) {
    $"4EF9 00000000"
};

/* List box template */

resource 'ldes' (ldes_Default) {
    versionZero {
	0, /* rows */
	1, /* cols */
	0, 0, /* default cell size */
	hasVertScroll, noHorizScroll,
	0, /* LDEF number */
	noGrowSpace
    }
};