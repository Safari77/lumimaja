/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/
#include "PWSfileV3.h"
#include "PWSrand.h"
#include "Util.h"
#include "SysInfo.h"
#include "PWScore.h"
#include "PWSFilters.h"
#include "PWSdirs.h"
#include "PWSprefs.h"
#include "PWStime.h"
#include "core.h"

#include "os/debug.h"
#include "os/file.h"
#include "os/logit.h"

#include "XML/XMLDefs.h"  // Required if testing "USE_XML_LIBRARY"

#ifdef _WIN32
#include <io.h>
#endif

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <iomanip>
#include <unistd.h>
#include <argon2.h>

using namespace std;
using pws_os::CUUID;

#define V3TAG "LuM3"

PWSfileV3::PWSfileV3(const StringX &filename, RWmode mode, VERSION version)
: PWSfile(filename, mode), m_HashPasses(1), m_HashMemKiB(1<<20)
{
  m_curversion = version;
  m_rawpos = 0;
}

PWSfileV3::~PWSfileV3()
{
  fprintf(stderr, "bye PWSfileV3 %p\n", this);
}

typedef int (*argon2fun)(Argon2_Context*);

typedef struct {
  argon2fun f;
  uint8_t name[3];
} argon2funmap;

// not overengineering at all
static argon2funmap argon2funmaps[256] = {
  [PWSfileV3::V3_ARGON2_DS] = { &Argon2ds, "ds" },
  [PWSfileV3::V3_ARGON2_ID] = { &Argon2id, "id" }
};

bool PWSfileV3::Argon2HashPass(const StringX &passkey, const struct TAGHDR *taghdr, unsigned char *out, size_t outlen,
                               unsigned char *salt, size_t saltlen, uint32 t_cost, uint32 m_cost, uint32 nLanes)
{
  size_t passLen = 0;
  unsigned char *pstr = NULL;
  ConvertString(passkey, pstr, passLen);
  struct TAGHDR copytag = *taghdr;
  argon2funmap muchfun = argon2funmaps[copytag.Argon2Type];
  int aret;

  if (muchfun.f == NULL) {
    fprintf(stderr, "Argon2 error: unsupported type %u\n", copytag.Argon2Type);
    return false;
  }
  /* password and ad are cleared by Argon2 */
  fprintf(stderr, "Argon2%s outlen=%zu passlen=%zu saltlen=%zu t_cost=%u m_cost=%u nLanes=%u starting...",
          muchfun.name, outlen, passLen, saltlen, t_cost, m_cost, nLanes);
  Argon2_Context ctx(out, outlen, pstr, passLen, salt, saltlen,
                 /* ad */ reinterpret_cast<unsigned char*>(&copytag), sizeof(copytag),
                 /* secret */ NULL, 0,
                 t_cost, m_cost, nLanes, nLanes,
                 NULL, NULL, true, false, true, false);
  aret = (*muchfun.f)(&ctx);
  delete[] pstr;
  fprintf(stderr, " done\n");
  if (aret != ARGON2_OK) {
    fprintf(stderr, "Argon2 error: %s\n", ErrorMessage(aret));
    return false;
  }
  return true;
}

int PWSfileV3::Open(const StringX &passkey)
{
  PWS_LOGIT;

  int status = SUCCESS;

  ASSERT(m_curversion == V30);
  if (passkey.empty()) { // Can happen if db 'locked'
    pws_os::Trace(_T("PWSfileV3::Open(empty_passkey)\n"));
    return PWScore::WRONG_PASSWORD;
  }
  m_passkey = passkey;

  fprintf(stderr, "PWSfileV3::Open\n");
  FOpen();
  if (m_fd == NULL)
    return PWScore::CANT_OPEN_FILE;

  if (m_rw == Write) {
    fprintf(stderr, "PWSfileV3::Open WRITE start pos=%ld\n", ftell(m_fd));
    status = WriteHeader();
    fprintf(stderr, "PWSfileV3::Open WRITE status=%d pos=%ld\n",
            status, ftell(m_fd));
  } else { // open for read
    fprintf(stderr, "PWSfileV3::Open READ start\n");
    status = ReadHeader();
    if (m_fd) fprintf(stderr, "PWSfileV3::Open fpos=%ld status=%d\n",
            ftell(m_fd), status);
    if (status != SUCCESS) {
      Close();
      return status;
    }
  }
  return status;
}

int PWSfileV3::Close()
{
  PWS_LOGIT;

  if (m_fd == NULL)
    return SUCCESS; // idempotent

  fprintf(stderr, "m_rawdata.size()=%zu to encrypt, fpos=%lu m_filename=%ls\n",
          m_rawdata.size(), ftell(m_fd), m_filename.c_str());

  // Write or verify HMAC, depending on RWmode.
  if (m_rw == Write) {
    unsigned long long ctlen;
    ENCSIZEHDR encsz;
    uint8_t *ct;

    putInt64(reinterpret_cast<unsigned char *>(&encsz.sz), m_rawdata.size());
    if ((crypto_aead_chacha20poly1305_encrypt(reinterpret_cast<unsigned char *>(&encsz),
             &ctlen, reinterpret_cast<unsigned char *>(&encsz.sz), sizeof(encsz.sz),
             NULL, 0, NULL, m_nonce, m_key) == -1) ||
        (fwrite(&encsz, ctlen, 1, m_fd) != 1)) {
      PWSfile::Close();
      return FAILURE;
    }
    m_nonce[0]++; // 😇

    ct = new uint8_t[m_rawdata.size() + crypto_aead_chacha20poly1305_ABYTES];
    if ((crypto_aead_chacha20poly1305_encrypt(ct, &ctlen, &m_rawdata[0],
             m_rawdata.size(), NULL, 0, NULL, m_nonce, m_key) == -1) ||
        (fwrite(ct, ctlen, 1, m_fd) != 1)) {
      delete[] ct;
      PWSfile::Close();
      return FAILURE;
    }
    delete[] ct;
    fprintf(stderr, "...fwrite OK for %llu bytes (incl. %zu for encsz)\n",
            sizeof(encsz) + ctlen, sizeof(encsz));
    return PWSfile::CloseSync();
  } else { // Read
    if (m_rawdata.size() == 0) {
      fprintf(stderr, "Closing corrupted database\n");
      PWSfile::Close();
      return FAILURE;
    }
    fprintf(stderr, "PWSfileV3::Close READ\n");
    return PWSfile::Close();
  }
}

int PWSfileV3::CheckPasskey(const StringX &filename,
                            const StringX &passkey, FILE *a_fd,
                            unsigned char *aPtag, uint32 *tCOST, uint32 *mCOST)
{
  PWS_LOGIT;

  FILE *fd = a_fd;
  int retval = SUCCESS;
  uint32 nT, nM, nL;
  unsigned char Ptag[ARGON2_TAGLEN];
  unsigned char checkHPtag[HPTAGLEN];
  PWSfileV3::PTHDR hdr;

  if (passkey.empty()) {
    fprintf(stderr, "PWSfileV3::CheckPasskey passkey.empty\n");
    return PWScore::WRONG_PASSWORD;
  }

  if (fd == NULL) {
    fd = pws_os::FOpen(filename.c_str(), _T("rb"));
  }
  if (fd == NULL)
    return PWScore::CANT_OPEN_FILE;

  if (fread(&hdr, sizeof(hdr), 1, fd) != 1) {
    retval = PWScore::TRUNCATED_FILE;
    goto err;
  }

  if (memcmp(&hdr.taghdr.tag, V3TAG, sizeof(hdr.taghdr.tag)) != 0) {
    retval = PWScore::NOT_LUMI3_FILE;
    goto err;
  }
  // hdr.taghdr.Argon2Type check done later
  if (hdr.taghdr.AEAD != V3_AEAD_CHACHA20POLY1305) {
    retval = PWScore::CRYPTO_ERROR;
    goto err;
  }
  if (hdr.taghdr.Hash != V3_HASH_BLAKE2B) {
    retval = PWScore::CRYPTO_ERROR;
    goto err;
  }

  nT = getInt32(&hdr.nPasses[0]);
  nM = getInt32(&hdr.nMemKiB[0]);
  nL = getInt32(&hdr.nLanes[0]);

  if (tCOST != NULL)
    *tCOST = nT;
  if (mCOST != NULL)
    *mCOST = nM;
  if (aPtag == NULL)
    aPtag = Ptag;

  if (Argon2HashPass(passkey, &hdr.taghdr, aPtag, sizeof(Ptag), hdr.salt,
                     sizeof(hdr.salt), nT, nM, nL) != true) {
    retval = PWScore::ARGON2_FAIL;
  } else {
    if (crypto_generichash_blake2b(checkHPtag, sizeof(checkHPtag), aPtag,
                                   sizeof(Ptag), NULL, 0) != 0) {
      fprintf(stderr, "blake2b fail\n");
      retval = PWScore::WRONG_PASSWORD;
    }
    if (memcmp(hdr.HPtag, checkHPtag, sizeof(checkHPtag)) != 0) {
      fprintf(stderr, "PWSfileV3::CheckPasskey WRONG_PASSWORD\n");
      retval = PWScore::WRONG_PASSWORD;
    }
  }

err:
  if (a_fd == NULL) // if we opened the file, we close it...
    fclose(fd);

  return retval;
}

int PWSfileV3::WriteRecord(const CItemData &item)
{
  ASSERT(m_fd != NULL);
  ASSERT(m_curversion == V30);
  return item.Write(this);
}

int PWSfileV3::ReadRecord(CItemData &item)
{
  ASSERT(m_fd != NULL);
  ASSERT(m_curversion == V30);
  fprintf(stderr, "PWSfileV3::ReadRecord\n");
  return item.Read(this);
}

const uint16 VersionNum = 0x030E;

int PWSfileV3::WriteHeader()
{
  PWS_LOGIT;
  int status = PWScore::SUCCESS;
  PWSfileV3::PTHDR hdr;
  unsigned char Ptag[PWSfileV3::ARGON2_TAGLEN];

  // See formatV3.txt for explanation of what's written here and why
  uint32 NumHashPasses = std::max(m_HashPasses, MIN_HASH_PASSES);
  uint32 NumHashMemKiB = std::max(m_HashMemKiB, MIN_HASH_MEM_KIB);
#ifndef _SC_NPROCESSORS_CONF
  long nProcs = 1;
#else
  long nProcs = sysconf(_SC_NPROCESSORS_CONF);
#endif
  if (nProcs < ARGON2_MIN_LANES) nProcs = ARGON2_MIN_LANES;
  if (nProcs > ARGON2_MAX_LANES) nProcs = ARGON2_MAX_LANES;
  uint32 nLanes = nProcs;

  memcpy(hdr.taghdr.tag, V3TAG, TAGHDR::V3TAGLEN);
  hdr.taghdr.Argon2Type = V3_ARGON2_DS; // XXX make configurable
  hdr.taghdr.AEAD = V3_AEAD_CHACHA20POLY1305;
  hdr.taghdr.Hash = V3_HASH_BLAKE2B;
  PWSrand::GetInstance()->GetRandomData(hdr.salt, sizeof(hdr.salt));
  putInt32(&hdr.nPasses[0], NumHashPasses);
  putInt32(&hdr.nMemKiB[0], NumHashMemKiB);
  putInt32(&hdr.nLanes[0], nLanes);

  if (Argon2HashPass(m_passkey, &hdr.taghdr, Ptag, sizeof(Ptag),
                     hdr.salt, sizeof(hdr.salt),
                     NumHashPasses, NumHashMemKiB, nLanes) != true) {
    status = PWScore::ARGON2_FAIL;
    goto end;
  }
  if (crypto_generichash_blake2b(hdr.HPtag, sizeof(hdr.HPtag), Ptag,
                                 sizeof(Ptag), NULL, 0) != 0) {
    fprintf(stderr, "blake2b fail\n");
    status = PWScore::ARGON2_FAIL;
    goto end;
  }

  memcpy(m_nonce, &Ptag[0], sizeof(m_nonce));
  memcpy(m_key, &Ptag[sizeof(m_nonce)], sizeof(m_key));
  trashMemory(Ptag, sizeof(Ptag));

  if (fwrite(&hdr, sizeof(hdr), 1, m_fd) != 1) {
      status = FAILURE;
      goto end;
  }
  fprintf(stderr, "PWSfileV3::WriteHeader fpos=%ld\n", ftell(m_fd));
  m_rawdata.clear();

  // write some actual data (at last!)
  // Write version number
  unsigned char vnb[sizeof(VersionNum)];
  vnb[0] = static_cast<unsigned char>(VersionNum & 0xff);
  vnb[1] = static_cast<unsigned char>((VersionNum & 0xff00) >> 8);
  m_hdr.m_nCurrentMajorVersion = static_cast<unsigned short>((VersionNum & 0xff00) >> 8);
  m_hdr.m_nCurrentMinorVersion = static_cast<unsigned short>(VersionNum & 0xff);

  // OOM is fatal when writing
  PWSfile::WriteRaw(HDR_VERSION, vnb, sizeof(VersionNum));

  // Write UUID
  if (m_hdr.m_file_uuid == pws_os::CUUID::NullUUID()) {
    // If not there or zeroed, create new
    CUUID uuid;
    m_hdr.m_file_uuid = uuid;
  }

  PWSfile::WriteRaw(HDR_UUID, *m_hdr.m_file_uuid.GetARep(),
                        sizeof(uuid_array_t));

  // Write (non default) user preferences
  PWSfile::WriteRaw(HDR_NDPREFS, m_hdr.m_prefString.c_str());

  // Write out display status
  if (!m_hdr.m_displaystatus.empty()) {
    StringX ds(_T(""));
    vector<bool>::const_iterator iter;
    for (iter = m_hdr.m_displaystatus.begin();
         iter != m_hdr.m_displaystatus.end(); iter++)
      ds += (*iter) ? _T("1") : _T("0");
    PWSfile::WriteRaw(HDR_DISPSTAT, ds);
  }

  // Write out time of this update
  {
    PWStime pwt; // c'tor set current time
    PWSfile::WriteRaw(HDR_LASTUPDATETIME, pwt, PWStime::TIME_LEN);
    m_hdr.m_whenlastsaved = pwt;
  }

  // Write out who saved it!
  {
    const SysInfo *si = SysInfo::GetInstance();
    stringT user = si->GetRealUser();
    stringT sysname = si->GetRealHost();
    PWSfile::WriteRaw(HDR_LASTUPDATEUSER, user.c_str());
     PWSfile::WriteRaw(HDR_LASTUPDATEHOST, sysname.c_str());
    m_hdr.m_lastsavedby = user.c_str();
    m_hdr.m_lastsavedon = sysname.c_str();
  }

  // Write out what saved it!
  PWSfile::WriteRaw(HDR_LASTUPDATEAPPLICATION, m_hdr.m_whatlastsaved);

  if (!m_hdr.m_dbname.empty()) {
    PWSfile::WriteRaw(HDR_DBNAME, m_hdr.m_dbname);
  }
  if (!m_hdr.m_dbdesc.empty()) {
    PWSfile::WriteRaw(HDR_DBDESC, m_hdr.m_dbdesc);
  }
  if (!m_MapFilters.empty()) {
    coStringXStream oss;  // XML is always char not wchar_t
    m_MapFilters.WriteFilterXMLFile(oss, m_hdr, _T(""));
    PWSfile::WriteRaw(HDR_FILTERS,
                      reinterpret_cast<const unsigned char *>(oss.str().c_str()),
                      oss.str().length());
  }

  if (!m_hdr.m_RUEList.empty()) {
    size_t num = m_hdr.m_RUEList.size();
    if (num > 255)
      num = 255; // Only save up to max as defined by FormatV3.

    size_t buflen = (num * sizeof(uuid_array_t)) + 1;
    unsigned char *buf = new unsigned char[buflen];
    buf[0] = (unsigned char)num;
    unsigned char *buf_ptr = buf + 1;

    UUIDListIter iter = m_hdr.m_RUEList.begin();

    for (size_t n = 0; n < num; n++, iter++) {
      const uuid_array_t *rep = iter->GetARep();
      memcpy(buf_ptr, rep, sizeof(uuid_array_t));
      buf_ptr += sizeof(uuid_array_t);
    }

    PWSfile::WriteRaw(HDR_RUE, buf, buflen);
    delete[] buf;
  }

  // Named Policies
  if (!m_MapPSWDPLC.empty()) {
    oStringXStream oss;
    oss.fill(charT('0'));

    size_t num = m_MapPSWDPLC.size();
    if (num > 255)
      num = 255;  // Do not exceed 2 hex character length field

    oss << setw(2) << hex << num;
    PSWDPolicyMapIter iter = m_MapPSWDPLC.begin();
    for (size_t n = 0; n < num; n++, iter++) {
      // The Policy name is limited to 255 characters.
      // This should have been prevented by the GUI.
      // If not, don't write it out as it may cause issues
      if (iter->first.length() > 255)
        continue;

      oss << setw(2) << hex << iter->first.length();
      oss << iter->first.c_str();
      StringX strpwp(iter->second);
      oss << strpwp.c_str();
      if (iter->second.symbols.empty()) {
        oss << _T("00");
      } else {
        oss << setw(2) << hex << iter->second.symbols.length();
        oss << iter->second.symbols.c_str();
      }
    }

    PWSfile::WriteRaw(HDR_PSWDPOLICIES, StringX(oss.str().c_str()));
  }

  // Empty Groups
  for (size_t n = 0; n < m_vEmptyGroups.size(); n++) {
    PWSfile::WriteRaw(HDR_EMPTYGROUP, m_vEmptyGroups[n]);
  }

  for (UnknownFieldList::iterator vi_IterUHFE = m_UHFL.begin();
       vi_IterUHFE != m_UHFL.end(); vi_IterUHFE++) {
    UnknownFieldEntry &unkhfe = *vi_IterUHFE;
    PWSfile::WriteRaw(unkhfe.uc_Type, unkhfe.uc_pUField,
                      static_cast<unsigned int>(unkhfe.st_length));
  }

  if (m_hdr.m_yubi_sk != NULL) {
    PWSfile::WriteRaw(HDR_YUBI_SK, m_hdr.m_yubi_sk, HeaderRecord::YUBI_SK_LEN);
  }

  // Write zero-length end-of-record type item
  PWSfile::WriteRaw(HDR_END, NULL, 0);

end:
  if (status != SUCCESS) {
    Close();
  }
  fprintf(stderr, "end of WriteHeader HDR_END %zu\n", m_rawdata.size());
  return status;
}

int PWSfileV3::ReadHeader()
{
  PWS_LOGIT;

  fprintf(stderr, "PWSfileV3::ReadHeader m_rawpos=%zu\n", m_rawpos);
  unsigned char Ptag[ARGON2_TAGLEN];
  int status = CheckPasskey(m_filename, m_passkey, m_fd,
                            Ptag, &m_HashPasses, &m_HashMemKiB);
  if (status != SUCCESS) {
    fprintf(stderr, "ReadHeader ret %d\n", status);
    return status;
  }

  m_rawdata.clear();
  memcpy(m_nonce, &Ptag[0], sizeof(m_nonce));
  memcpy(m_key, &Ptag[sizeof(m_nonce)], sizeof(m_key));
  trashMemory(Ptag, sizeof(Ptag));

  fprintf(stderr, "fpos=%lu\n", ftell(m_fd));
  m_rawdata.reserve(sizeof(ENCSIZEHDR));
  if (fread(&m_rawdata[0], sizeof(ENCSIZEHDR), 1, m_fd) != 1) {
    fprintf(stderr, "PWSfileV3::ReadHeader failed to read %lu bytes\n",
            sizeof(ENCSIZEHDR));
    Close();
    m_rawdata.clear();
    return PWScore::TRUNCATED_FILE;
  }
  uint64_t sz64;
  unsigned long long ptlen;

  if (crypto_aead_chacha20poly1305_decrypt(reinterpret_cast<unsigned char*>(&sz64),
            &ptlen, NULL, &m_rawdata[0], sizeof(ENCSIZEHDR), NULL, 0, m_nonce, m_key) != 0) {
    fprintf(stderr, "PWSfileV3::ReadHeader encrypted size decrypt failed\n");
    Close();
    m_rawdata.clear();
    return PWScore::CRYPTO_ERROR;
  }

  sz64 = getInt64(reinterpret_cast<unsigned char*>(&sz64));
  fprintf(stderr, "read encsz %lu\n", sz64);

  sz64 += crypto_aead_chacha20poly1305_ABYTES;
  m_rawdata.resize(sz64);
  m_nonce[0]++; // 😎 
  size_t nread = fread(&m_rawdata[0], 1, sz64, m_fd);
  if (nread != sz64) {
    fprintf(stderr, "PWSfileV3::ReadHeader failed to read %lu bytes of "
            "encrypted data, %zu bytes missing\n", sz64, sz64 - nread);
    m_rawdata.clear();
    Close();
    return PWScore::TRUNCATED_FILE;
  }
  if (crypto_aead_chacha20poly1305_decrypt(&m_rawdata[0], &ptlen, NULL, &m_rawdata[0],
            sz64, NULL, 0, m_nonce, m_key) != 0) {
    fprintf(stderr, "PWSfileV3::ReadHeader data decrypt failed\n");
    m_rawdata.clear();
    Close();
    return PWScore::CRYPTO_ERROR;
  }
  m_rawpos = 0;
  m_rawdata.resize(sz64 - crypto_aead_chacha20poly1305_ABYTES);

  unsigned char fieldType;
  StringX text;
  bool utf8status;
  unsigned char *utf8 = NULL;
  size_t utf8Len = 0;
  size_t maxFails = 42;

  do {
    if (PWSfile::ReadRaw(fieldType, utf8, utf8Len) == 0) {
      if (--maxFails == 0) {
        delete[] utf8;
        m_rawdata.clear();
        return FAILURE;
      }
      continue;
    }

    switch (fieldType) {
    case HDR_VERSION: /* version */
      if (utf8Len != sizeof(VersionNum)) {
        delete[] utf8;
        Close();
        return FAILURE;
      }
      if (utf8[1] !=
          static_cast<unsigned char>((VersionNum & 0xff00) >> 8)) {
        //major version mismatch
        delete[] utf8;
        Close();
        return PWScore::UNSUPPORTED_VERSION;
      }
      // for now we assume that minor version changes will
      // be backward-compatible
      m_hdr.m_nCurrentMajorVersion = static_cast<unsigned short>(utf8[1]);
      m_hdr.m_nCurrentMinorVersion = static_cast<unsigned short>(utf8[0]);
      break;

    case HDR_UUID: /* UUID */
      if (utf8Len != sizeof(uuid_array_t)) {
        delete[] utf8;
        Close();
        return FAILURE;
      }
      uuid_array_t ua;
      memcpy(ua, utf8, sizeof(ua));
      m_hdr.m_file_uuid = pws_os::CUUID(ua);
      break;

    case HDR_NDPREFS: /* Non-default user preferences */
      if (utf8Len != 0) {
        if (utf8 != NULL) utf8[utf8Len] = '\0';
        StringX pref;
        utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, pref);
        if (utf8status) {
          m_hdr.m_prefString = pref;
        } else {
          pws_os::Trace0(_T("FromUTF8(m_prefString) failed\n"));
        }
      } else
        m_hdr.m_prefString = _T("");
      break;

    case HDR_DISPSTAT: /* Tree Display Status */
      if (utf8 != NULL) utf8[utf8Len] = '\0';
      utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
      if (utf8status) {
        for (StringX::iterator iter = text.begin(); iter != text.end(); iter++) {
          const TCHAR v = *iter;
          m_hdr.m_displaystatus.push_back(v == TCHAR('1'));
        }
      } else {
        pws_os::Trace0(_T("FromUTF8(m_displaystatus) failed\n"));
      }
      break;

    case HDR_LASTUPDATETIME: /* When last saved */
    {
      ASSERT(utf8Len == PWStime::TIME_LEN);
      PWStime pwt(utf8);
      m_hdr.m_whenlastsaved = pwt;
      break;
    }

    case HDR_LASTUPDATEAPPLICATION: /* and by what */
      if (utf8 != NULL) utf8[utf8Len] = '\0';
      utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, m_hdr.m_whatlastsaved);
      if (!utf8status)
        pws_os::Trace0(_T("FromUTF8(m_whatlastsaved) failed\n"));
      break;

    case HDR_LASTUPDATEUSER:
      if (utf8 != NULL) utf8[utf8Len] = '\0';
      m_utf8conv.FromUTF8(utf8, utf8Len, m_hdr.m_lastsavedby);
      break;

    case HDR_LASTUPDATEHOST:
      if (utf8 != NULL) utf8[utf8Len] = '\0';
      m_utf8conv.FromUTF8(utf8, utf8Len, m_hdr.m_lastsavedon);
      break;

    case HDR_DBNAME:
      if (utf8 != NULL) utf8[utf8Len] = '\0';
      m_utf8conv.FromUTF8(utf8, utf8Len, m_hdr.m_dbname);
      break;

    case HDR_DBDESC:
      if (utf8 != NULL) utf8[utf8Len] = '\0';
      m_utf8conv.FromUTF8(utf8, utf8Len, m_hdr.m_dbdesc);
      break;

#if !defined(USE_XML_LIBRARY) || (!defined(_WIN32) && USE_XML_LIBRARY == MSXML)
      // Don't support importing XML from non-Windows platforms
      // using Microsoft XML libraries
      // Will be treated as an 'unknown header field' by the 'default' clause below
#else
    case HDR_FILTERS:
      if (utf8 != NULL) utf8[utf8Len] = '\0';
      utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
      if (utf8status && (utf8Len > 0)) {
        stringT strErrors;
        stringT XSDFilename = PWSdirs::GetXMLDir() + _T("lumimaja_filter.xsd");
        if (!pws_os::FileExists(XSDFilename)) {
          fprintf(stderr, "No filter schema\n");
          // No filter schema => user won't be able to access stored filters
          // Inform her of the fact (probably an installation problem).
          stringT message, message2;
          Format(message, IDSC_MISSINGXSD, L"lumimaja_filter.xsd");
          LoadAString(message2, IDSC_FILTERSKEPT);
          message += stringT(_T("\n\n")) + message2;
          if (m_pReporter != NULL)
            (*m_pReporter)(message);

          // Treat it as an Unknown field!
          // Maybe user used a later version of PWS
          // and we don't want to lose anything
          UnknownFieldEntry unkhfe(fieldType, utf8Len, utf8);
          m_UHFL.push_back(unkhfe);
          break;
        }
        int rc = m_MapFilters.ImportFilterXMLFile(FPOOL_DATABASE, text.c_str(), _T(""),
                                                  XSDFilename.c_str(),
                                                  strErrors, m_pAsker);
        if (rc != PWScore::SUCCESS) {
          // Can't parse it - treat as an unknown field,
          // Notify user that filter won't be available
          fprintf(stderr, "ImportFilterXMLFile failed\n");
          stringT message;
          LoadAString(message, IDSC_CANTPROCESSDBFILTERS);
          if (m_pReporter != NULL)
            (*m_pReporter)(message);
          pws_os::Trace(L"Error while parsing header filters.\n\tData: %ls\n\tErrors: %ls\n",
                        text.c_str(), strErrors.c_str());
          UnknownFieldEntry unkhfe(fieldType, utf8Len, utf8);
          m_UHFL.push_back(unkhfe);
        }
      }
      break;
#endif

    case HDR_RUE:
      {
        // All data is binary
        // Get number of entries
        int num = utf8[0];

        // verify we have enough data
        if (utf8Len != num * sizeof(uuid_array_t) + 1)
          break;

        // Get the entries and save them
        unsigned char *buf = utf8 + 1;
        for (int n = 0; n < num; n++, buf += sizeof(uuid_array_t)) {
          uuid_array_t uax = {0};
          memcpy(uax, buf, sizeof(uuid_array_t));
          const CUUID uuid(uax);
          if (uuid != CUUID::NullUUID())
            m_hdr.m_RUEList.push_back(uuid);
        }
        break;
      }

    case HDR_YUBI_SK:
      if (utf8Len != HeaderRecord::YUBI_SK_LEN) {
        delete[] utf8;
        Close();
        return FAILURE;
      }
      m_hdr.m_yubi_sk = new unsigned char[HeaderRecord::YUBI_SK_LEN];
      memcpy(m_hdr.m_yubi_sk, utf8, HeaderRecord::YUBI_SK_LEN);
      break;

    case HDR_PSWDPOLICIES:
      {
        const size_t minPolLen = 1 + 1 + 1 + 2 + 2*5 + 1; // see formatV4.txt
        if (utf8Len < minPolLen)
          break; // Error

        unsigned char *buf_ptr = utf8;
        const unsigned char *max_ptr = utf8 + utf8Len; // for sanity checks
        int num = *buf_ptr++;

        // Get the policies and save them
        for (int n = 0; n < num; n++) {
          StringX sxPolicyName;
          PWPolicy pwp;

          int nameLen = *buf_ptr++;
          // need to tack on null byte to name before conversion
          unsigned char *nmbuf = new unsigned char[nameLen + 1];
          memcpy(nmbuf, buf_ptr, nameLen); nmbuf[nameLen] = 0;
          utf8status = m_utf8conv.FromUTF8(nmbuf, nameLen, sxPolicyName);
          trashMemory(nmbuf, nameLen); delete[] nmbuf;
          if (!utf8status)
            continue;
          buf_ptr += nameLen;
          if (buf_ptr > max_ptr)
            break; // Error
          pwp.flags = getInt16(buf_ptr);            buf_ptr += 2;
          pwp.length = getInt16(buf_ptr);           buf_ptr += 2;
          pwp.lowerminlength = getInt16(buf_ptr);   buf_ptr += 2;
          pwp.upperminlength = getInt16(buf_ptr);   buf_ptr += 2;
          pwp.digitminlength = getInt16(buf_ptr);   buf_ptr += 2;
          pwp.symbolminlength = getInt16(buf_ptr);  buf_ptr += 2;
          if (buf_ptr > max_ptr)
            break; // Error
          int symLen = *buf_ptr++;
          if (symLen > 0) {
            // need to tack on null byte to symbols before conversion
            unsigned char *symbuf = new unsigned char[symLen + 1];
            memcpy(symbuf, buf_ptr, symLen); symbuf[symLen] = 0;
            utf8status = m_utf8conv.FromUTF8(symbuf, symLen, pwp.symbols);
            trashMemory(symbuf, symLen); delete[] symbuf;
            if (!utf8status)
              continue;
            buf_ptr += symLen;
          }
          if (buf_ptr > max_ptr)
            break; // Error
          pair< map<StringX, PWPolicy>::iterator, bool > pr;
          pr = m_MapPSWDPLC.insert(PSWDPolicyMapPair(sxPolicyName, pwp));
          if (pr.second == false) break; // Error
        } // iterate over named policies
      }
      break;

    case HDR_EMPTYGROUP:
      {
        if (utf8 != NULL) utf8[utf8Len] = '\0';
        utf8status = m_utf8conv.FromUTF8(utf8, utf8Len, text);
        if (utf8status) {
          m_vEmptyGroups.push_back(text);
        }
        break;
      }

    case HDR_END: /* process END so not to treat it as 'unknown' */
      break;

    default:
      // Save unknown fields that may be addded by future versions
      UnknownFieldEntry unkhfe(fieldType, utf8Len, utf8);

      m_UHFL.push_back(unkhfe);
#if 0
#ifdef _DEBUG
      stringT stimestamp;
      PWSUtil::GetTimeStamp(stimestamp);
      pws_os::Trace(_T("Header has unknown field: %02x, length %d/0x%04x, value:\n"),
                    fieldType, utf8Len, utf8Len);
      pws_os::HexDump(utf8, utf8Len, stimestamp);
#endif
#endif
      break;
    }
    // unknown header
    //pws_os::Trace(_T("Header field: %02x length %d/0x%04x\n"),
    //                 fieldType, utf8Len, utf8Len);
  } while (fieldType != HDR_END);
  delete[] utf8;

  return SUCCESS;
}

