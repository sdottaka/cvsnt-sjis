#include "cvsdelta.h"

int cvsdelta_diff(const void *buf1, size_t buf1_len, const void *buf2, size_t buf2_len, void **buf3, size_t *buf3_len)
{
  cvsdelta c;
  const ByteArray b1((Byte*)buf1, buf1_len);
  const ByteArray b2((Byte*)buf2, buf2_len);
  ByteArray b3((Byte*)NULL, 0);

  int ret = (c.diff(b1,b2,b3)==true)?0:1;
  *buf3=&b3[0];
  *buf3_len=b3.size();
  return ret;
}

int cvsdelta_patch(const void *buf1, size_t buf1_len, const void *buf2, size_t buf2_len, void **buf3, size_t *buf3_len, size_t *buf3_reserved)
{
  cvsdelta c;
  const ByteArray b1((Byte*)buf1, buf1_len);
  const ByteArray b2((Byte*)buf2, buf2_len);
  ByteArray b3((Byte*)*buf3, *buf3_reserved);

  int ret = (c.patch(b1,b2,b3)==true)?0:1;
  *buf3=&b3[0];
  *buf3_len=b3.size();
  *buf3_reserved=b3.reserved_size();
  return ret;
}
