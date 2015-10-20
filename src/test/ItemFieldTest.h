/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#include "test.h"
#include "core/ItemField.h"


class NullFish
{
public:
  NullFish() {}
  virtual ~NullFish() {}
  virtual unsigned int GetBlockSize() const {return 8;}
  // Following encrypt/decrypt a single block
  // (blocksize dependent on cipher)
  virtual void Encrypt(const unsigned char *pt, unsigned char *ct)
  {memcpy(ct, pt, GetBlockSize());}
  virtual void Decrypt(const unsigned char *ct, unsigned char *pt)
  {memcpy(pt, ct, GetBlockSize());}
};

class ItemFieldTest : public Test
{

public:
  ItemFieldTest()
    {
  }
  void run()
  {
    // The tests to run:
    testMe();
  }

  void testMe()
  {
    unsigned char v1[16] = {0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
                            0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf};
    unsigned char v2[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,};
    size_t lenV2 = sizeof(v2);
    
    CItemField i1(1);
    _test(i1.IsEmpty());
    _test(i1.GetType() == 1);
    i1.Set(v1, sizeof(v1));
    i1.Get(v2, lenV2);
    _test(lenV2 == sizeof(v1));
    _test(memcmp(v1, v2, sizeof(v1)) == 0);
  }
};
