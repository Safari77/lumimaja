/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#ifndef __PWSFILEV3_H
#define __PWSFILEV3_H

// PWSfileV3.h
// Abstract the gory details of reading and writing an encrypted database
//-----------------------------------------------------------------------------

#include <sodium/crypto_aead_chacha20poly1305.h>

#include "PWSfile.h"
#include "PWSFilters.h"
#include "UTF8Conv.h"

class PWSfileV3 : public PWSfile
{
public:
  enum {HDR_VERSION           = 0x00,
    HDR_UUID                  = 0x01,
    HDR_NDPREFS               = 0x02,
    HDR_DISPSTAT              = 0x03,
    HDR_LASTUPDATETIME        = 0x04,
    HDR_LASTUPDATEAPPLICATION = 0x06,
    HDR_LASTUPDATEUSER        = 0x07,     // added in format 0x0302
    HDR_LASTUPDATEHOST        = 0x08,     // added in format 0x0302
    HDR_DBNAME                = 0x09,     // added in format 0x0302
    HDR_DBDESC                = 0x0a,     // added in format 0x0302
    HDR_FILTERS               = 0x0b,     // added in format 0x0305
    HDR_RESERVED1             = 0x0c,     // added in format 0x030?
    HDR_RESERVED2             = 0x0d,     // added in format 0x030?
    HDR_RESERVED3             = 0x0e,     // added in format 0x030?
    HDR_RUE                   = 0x0f,     // added in format 0x0307 - Recently Used Entries
    HDR_PSWDPOLICIES          = 0x10,     // added in format 0x030A
    HDR_EMPTYGROUP            = 0x11,     // added in format 0x030B
    HDR_YUBI_SK               = 0x12,     // Yubi-specific: format 0x030c
    HDR_LAST,                             // Start of unknown fields!
    HDR_END                   = 0xff};    // header field types, per formatV{2,3}.txt

  enum V3_ARGON2 {
    V3_ARGON2_DS = 0,
    V3_ARGON2_ID,
    V3_ARGON2_D,
    V3_ARGON2_I
  };

  enum V3_AEAD {
    V3_AEAD_CHACHA20POLY1305 = 0,
    V3_AEAD_NORX6461
  };
  enum V3_HASH {
    V3_HASH_BLAKE2B = 0,
    V3_HASH_SHAKE256
  };
  enum {
    ARGON2_TAGLEN = (crypto_aead_chacha20poly1305_NPUBBYTES +
                          crypto_aead_chacha20poly1305_KEYBYTES)
  };
  enum {
    HPTAGLEN = 16
  };

  struct TAGHDR { // fed to Argon2 as Associated Data
    enum { V3TAGLEN = 4 };
    uint8_t tag[V3TAGLEN];
    uint8_t Argon2Type;
    uint8_t AEAD;
    uint8_t Hash;
  } __attribute__((packed));

  struct PTHDR { // plaintext header to be written to disk
    struct TAGHDR taghdr;
    uint8_t salt[SaltLengthV3];
    uint8_t nPasses[sizeof(uint32)];
    uint8_t nMemKiB[sizeof(uint32)];
    uint8_t nLanes[sizeof(uint32)];
    uint8_t HPtag[HPTAGLEN]; // Hash of Argon2 output for verifying supplied passphrase
  } __attribute__((packed));

  struct ENCSIZEHDR { // size of encrypted data, after PTHDR
    uint64_t sz;
    uint8_t tag[crypto_aead_chacha20poly1305_ABYTES];
  } __attribute__((packed));

  static int CheckPasskey(const StringX &filename,
                          const StringX &passkey,
                          FILE *a_fd = NULL,
                          unsigned char *aPtag = NULL, uint32 *nPasses = NULL,
                          uint32 *nMemKiB = NULL);

  PWSfileV3(const StringX &filename, RWmode mode, VERSION version);
  ~PWSfileV3();

  virtual int Open(const StringX &passkey);
  virtual int Close();

  virtual int WriteRecord(const CItemData &item);
  virtual int ReadRecord(CItemData &item);

  uint32 GetHashPasses() const { return m_HashPasses; }
  void SetHashPasses(uint32 N) { m_HashPasses = N; }

  uint32 GetHashMemKiB() const { return m_HashMemKiB; }
  void SetHashMemKiB(uint32 N) { m_HashMemKiB = N; }

  void SetFilters(const PWSFilters &MapFilters) {m_MapFilters = MapFilters;}
  const PWSFilters &GetFilters() const {return m_MapFilters;}

  void SetPasswordPolicies(const PSWDPolicyMap &MapPSWDPLC) {m_MapPSWDPLC = MapPSWDPLC;}
  const PSWDPolicyMap &GetPasswordPolicies() const {return m_MapPSWDPLC;}

  void SetEmptyGroups(const std::vector<StringX> &vEmptyGroups) {m_vEmptyGroups = vEmptyGroups;}
  const std::vector<StringX> &GetEmptyGroups() const {return m_vEmptyGroups;}

private:
  uint32 m_HashPasses; /* Argon2 t_cost */
  uint32 m_HashMemKiB; /* Argon2 m_cost */
  uint8_t m_nonce[crypto_aead_chacha20poly1305_NPUBBYTES];
  uint8_t m_key[crypto_aead_chacha20poly1305_KEYBYTES];

  static bool Argon2HashPass(const StringX &passkey, const struct TAGHDR *taghdr,
                             unsigned char *out,
                             size_t outlen, unsigned char *salt, size_t saltlen,
                             uint32 t_cost, uint32 m_cost, uint32 nLanes);

  int WriteHeader();
  int ReadHeader();

  PWSFilters m_MapFilters;
  PSWDPolicyMap m_MapPSWDPLC;

  // EmptyGroups
  std::vector<StringX> m_vEmptyGroups;
};
#endif /* __PWSFILEV3_H */
