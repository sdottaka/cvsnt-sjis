/*
** The URL encoder/decoder used by WinCvs
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
 * Original design by knj yamamoto at xware on Fri Apr  4 17:14:17 2003,
 * rewritten for WinCvs (on "C" linkage) by xware_hitoh@users.sf.net on
 * Wed Feb  4 01:23:57 JST 2004.
 *
 * See detailed in RFC1738.
 *
 * <pre>
 * {
 *   char* enc = URI_encode("#$%&hoge17/");
 *   ASSERT(std::string(enc) == "%23%24%25%26hoge17%2F");
 *   char* dec = URI_decode(enc);
 *   ASSERT(std::string(dec) == "#$%&hoge17/");
 *   free(enc);
 *   free(dec);
 * }
 * </pre>
 *
 */
#ifndef HID_1049444057_03764223_51FD_434B_92F1_C06F7BF6336B__INCLUDED__
#define HID_1049444057_03764223_51FD_434B_92F1_C06F7BF6336B__INCLUDED__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef __cplusplus
extern "C" {
#endif
#define URI_ENCODER_RFC1738FULL   0xFFFFFFFF
#define URI_ENCODER_8BITCODE      0x00000001
#define URI_ENCODER_DQUOTE        0x00000002
#define URI_ENCODER_COMPACTSUBSET URI_ENCODER_8BITCODE | URI_ENCODER_DQUOTE
extern char* URI_encode(const char* src, unsigned int encode_type);
extern char* URI_decode(const char* src);
#ifdef __cplusplus
};
#endif

#endif // HID_1049444057_03764223_51FD_434B_92F1_C06F7BF6336B__INCLUDED__
