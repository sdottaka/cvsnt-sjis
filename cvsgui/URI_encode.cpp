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
#include <string>
#include <cstdlib>
#include <cctype>
#include "URI_encode.h"

namespace /* anon */ {
    unsigned char htoi__(const unsigned char* const src)
    {
        unsigned char c = 0;
        for (int i = 0; i < 2; ++i)
            {
                if (isdigit(src[i]))
                    {
                        c = (c << 4) + (src[i] - '0');
                    }
                else if ('A' <= src[i] && src[i] <= 'F')
                    {
                        c = (c << 4) + (src[i] - 'A' + 10);
                    }
            }
        return c;
    }

    bool is_to_encoding(unsigned char c, unsigned int encode_type)
        {
            bool retVal = false;
            if (encode_type == URI_ENCODER_RFC1738FULL)
                {
                    return !isalnum(c) && !strchr("_-.", c);
                }
            if ((encode_type & URI_ENCODER_8BITCODE) != 0 && !retVal)
                {
                    retVal = ((c & 0x80) != 0);
                }
            if ((encode_type & URI_ENCODER_DQUOTE) != 0 && !retVal)
                {
                    retVal = (c == '"');
                }
            return retVal || c == '%';
        }

    /**
     * @brief uriencoder
     *
     * see RFC1738 for the details
     */
    std::string URI_encode__(const std::string& str, unsigned int encode_type)
    {
        int len = str.size();
        std::string buf;
        buf.resize(len * 3);
        const unsigned char* src = reinterpret_cast<const unsigned char*>(str.data());
        unsigned char* dst = reinterpret_cast<unsigned char*>(const_cast<char*>(buf.data()));
        for (int x = 0, y = 0; len--; ++x, ++y)
            {
                static const unsigned char hexchars[] = "0123456789ABCDEF";
                dst[y] = src[x];
                if (is_to_encoding(dst[y], encode_type))
                    {
                        dst[y++] = '%';
                        dst[y++] = hexchars[src[x] >> 4];
                        dst[y] = hexchars[src[x] & 0xf];
                    }
            }
        return buf/*.c_str()*/;
    }

    /**
     * @brief uridecoder
     *
     * see RFC1738 for the details
     */
    std::string URI_decode__(const std::string& str)
    {
        int len = str.size();
        std::string buf;
        buf.resize(len);
        const unsigned char* src = reinterpret_cast<const unsigned char*>(str.data());
        unsigned char* dst = reinterpret_cast<unsigned char*>(const_cast<char*>(buf.data()));
        for (register int x = 0, y = 0; len--; ++x, ++y)
            {
                if (src[x] == '%' && len >= 2 && isxdigit(src[x + 1]) && isxdigit(src[x + 2]))
                    {
                        dst[y] = htoi__(&src[x + 1]);
                        x += 2;
                        len -= 2;
                    }
                else
                    {
                        dst[y] = src[x];
                    }
            }
        return buf/*.c_str()*/;
    }
}; /* namespace anon */

char* URI_encode(const char* src, unsigned int encode_type)
{
    return strdup(URI_encode__(src, encode_type).c_str());
}
char* URI_decode(const char* src)
{
    return strdup(URI_decode__(src).c_str());
}
