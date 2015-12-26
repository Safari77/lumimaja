/*
 * Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
 * All rights reserved. Use of the code is allowed under the
 * Artistic License 2.0 terms, as specified in the LICENSE file
 * distributed with this code, or available from
 * http://www.opensource.org/licenses/artistic-license-2.0.php
 */
#include "PWSfile.h"
#include "PWSfileV3.h"
#include "SysInfo.h"
#include "core.h"
#include "os/file.h"
#include "PWScore.h"
#include "PWSrand.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits>
#include <unistd.h>
#include <sodium.h>

PWSfile *PWSfile::MakePWSfile(const StringX &a_filename, VERSION &version,
                              RWmode mode, int &status,
                              Asker *pAsker, Reporter *pReporter)
{
  if (mode == Read && !pws_os::FileExists(a_filename.c_str())) {
    status = PWScore::CANT_OPEN_FILE;
    return NULL;
  }

  fprintf(stderr, "PWSfile::MakePWSfile v=%u mode=%u\n", version, mode);
  PWSfile *retval = NULL;
  switch (version) {
    case UNKNOWN_VERSION:
      ASSERT(mode == Read);
      version = V30;
    case V30:
      status = SUCCESS;
      retval = new PWSfileV3(a_filename, mode, version);
      break;
    default:
      ASSERT(0);
      status = FAILURE; return NULL;
  }
  if (!retval) {
    status = PWScore::CANT_OPEN_FILE;
    return NULL;
  }
  retval->m_pAsker = pAsker;
  retval->m_pReporter = pReporter;
  return retval;
}

PWSfile::PWSfile(const StringX &filename, RWmode mode)
  : m_filename(filename), m_filename_tmp(_T("")), m_passkey(_T("")), m_fd(NULL),
  m_curversion(UNKNOWN_VERSION), m_rw(mode), m_defusername(_T("")),
  m_nRecordsWithUnknownFields(0)
{
}

PWSfile::~PWSfile()
{
  Close(); // idempotent
}

PWSfile::HeaderRecord::HeaderRecord()
  : m_nCurrentMajorVersion(0), m_nCurrentMinorVersion(0),
    m_file_uuid(pws_os::CUUID::NullUUID()),
    m_prefString(_T("")), m_whenlastsaved(0),
    m_lastsavedby(_T("")), m_lastsavedon(_T("")),
    m_whatlastsaved(_T("")),
    m_dbname(_T("")), m_dbdesc(_T("")), m_yubi_sk(NULL)
{
  m_RUEList.clear();
}

PWSfile::HeaderRecord::HeaderRecord(const PWSfile::HeaderRecord &h) 
  : m_nCurrentMajorVersion(h.m_nCurrentMajorVersion),
    m_nCurrentMinorVersion(h.m_nCurrentMinorVersion),
    m_file_uuid(h.m_file_uuid),
    m_displaystatus(h.m_displaystatus),
    m_prefString(h.m_prefString), m_whenlastsaved(h.m_whenlastsaved),
    m_lastsavedby(h.m_lastsavedby), m_lastsavedon(h.m_lastsavedon),
    m_whatlastsaved(h.m_whatlastsaved),
    m_dbname(h.m_dbname), m_dbdesc(h.m_dbdesc), m_RUEList(h.m_RUEList)
{
  if (h.m_yubi_sk != NULL) {
    m_yubi_sk = new unsigned char[YUBI_SK_LEN];
    memcpy(m_yubi_sk, h.m_yubi_sk, YUBI_SK_LEN);
  } else {
    m_yubi_sk = NULL;
  }
}

PWSfile::HeaderRecord::~HeaderRecord()
{
  if (m_yubi_sk)
    trashMemory(m_yubi_sk, YUBI_SK_LEN);
  delete[] m_yubi_sk;
}

PWSfile::HeaderRecord &PWSfile::HeaderRecord::operator=(const PWSfile::HeaderRecord &h)
{
  if (this != &h) {
    m_nCurrentMajorVersion = h.m_nCurrentMajorVersion;
    m_nCurrentMinorVersion = h.m_nCurrentMinorVersion;
    m_file_uuid = h.m_file_uuid;
    m_displaystatus = h.m_displaystatus;
    m_prefString = h.m_prefString;
    m_whenlastsaved = h.m_whenlastsaved;
    m_lastsavedby = h.m_lastsavedby;
    m_lastsavedon = h.m_lastsavedon;
    m_whatlastsaved = h.m_whatlastsaved;
    m_dbname = h.m_dbname;
    m_dbdesc = h.m_dbdesc;
    m_RUEList = h.m_RUEList;
    if (h.m_yubi_sk != NULL) {
      if (m_yubi_sk)
        trashMemory(m_yubi_sk, YUBI_SK_LEN);
      delete[] m_yubi_sk;
      m_yubi_sk = new unsigned char[YUBI_SK_LEN];
      memcpy(m_yubi_sk, h.m_yubi_sk, YUBI_SK_LEN);
    } else {
      m_yubi_sk = NULL;
    }
  }
  return *this;
}

void PWSfile::FOpen()
{
  ASSERT(!m_filename.empty());
  const TCHAR* m = (m_rw == Read) ? _T("rb") : _T("wbex");
  if (m_fd != NULL) {
    fclose(m_fd);
    m_fd = NULL;
  }
  if (m_rw == Read) {
    m_fd = pws_os::FOpen(m_filename.c_str(), m);
  } else {
    m_filename_tmp = stringx2std(m_filename) + _T(".") + PWSrand::GetInstance()->RandAZ(8) + _T(".tmp");
    m_fd = pws_os::FOpen(m_filename_tmp.c_str(), m);
  }
  m_fileLength = pws_os::fileLength(m_fd);
}

int PWSfile::Close()
{
  fprintf(stderr, "PWSfile::Close m_filename=[%ls] m_filename_tmp=[%ls]\n",
         stringx2std(m_filename).c_str(), m_filename_tmp.c_str());
  if (m_fd != NULL) {
    fflush(m_fd);
    fclose(m_fd);
    if (m_filename_tmp.size()) {
      if (pws_os::RenameFile(m_filename_tmp.c_str(),
                             m_filename.c_str()) == false) {
        fprintf(stderr, "PWSfile::Close rename error: %s\n", strerror(errno));
      }
      m_filename_tmp.clear();
    }
    m_fd = NULL;
  }
  return SUCCESS;
}

int PWSfile::CloseSync()
{
  int ret = SUCCESS;

  // XXX SetRawdatasize(m_rawdata.size());

  fprintf(stderr, "PWSfile::CloseSync m_filename=[%ls] m_filename_tmp=[%ls]\n",
         stringx2std(m_filename).c_str(), m_filename_tmp.c_str());
  if (m_fd != NULL) {
    if (fflush(m_fd)) ret = FAILURE;
    if (fsync(fileno(m_fd)) == -1) ret = FAILURE;
    if (fclose(m_fd)) ret = FAILURE;
    if (m_filename_tmp.size()) {
      if (pws_os::RenameFile(m_filename_tmp.c_str(),
                             m_filename.c_str()) == false) {
        fprintf(stderr, "PWSfile::CloseSync rename error: %s\n", strerror(errno));
        ret = FAILURE;
      }
      m_filename_tmp.clear();
    }
    m_fd = NULL;
  }
  return ret;
}

size_t PWSfile::WriteRaw(unsigned char type, const StringX &data)
{
  bool status;
  const unsigned char *utf8;
  size_t utf8Len;
  status = m_utf8conv.ToUTF8(data, utf8, utf8Len);
  if (!status)
    pws_os::Trace(_T("ToUTF8(%ls) failed\n"), data.c_str());
  return WriteRaw(type, utf8, utf8Len);
}

size_t PWSfile::WriteRaw(unsigned char type, const unsigned char *data,
                         size_t length)
{
  if (length > UINT32_MAX) length = UINT32_MAX;
  size_t written = m_rawdata.size();
  m_rawdata.insert(m_rawdata.end(), type);
  vec_push32(m_rawdata, length);
  if (length)
    m_rawdata.insert(m_rawdata.end(), data, data+length);
  written = m_rawdata.size() - written;
#ifdef DEBUG
  fprintf(stderr, "PWSfileV3::WriteRaw type=%u len=%zu written=%zu totsize=%zu\n",
          type, length, written, m_rawdata.size());
#endif
  return written;
}

size_t PWSfile::ReadRaw(unsigned char &type, unsigned char* &data,
                        size_t &length)
{
  uint32 u32;

#ifdef DEBUG
  fprintf(stderr, "PWSfileV3::ReadRaw sz=%zu m_rawpos=%zu\n",
          m_rawdata.size(), m_rawpos);
#endif
  if (m_rawpos >= m_rawdata.size()) {
    return 0;
  }
  type = m_rawdata.at(m_rawpos++);
  if (m_rawpos > m_rawdata.size() - 4) {
    return 0;
  }
  u32 = vec_pull32(m_rawdata, m_rawpos);
  if ((u32 + m_rawpos > m_rawdata.size())) {
      return 0;
  }
  length = u32;
  if (data) delete[] data;
  data = new unsigned char[u32+1]; // for extra char after utf8 string
  memcpy(data, &m_rawdata[m_rawpos], u32);
  m_rawpos += u32;
#ifdef DEBUG
  fprintf(stderr, "... type=%u len=%zu m_rawpos=%zu\n",
          type, length, m_rawpos);
#endif

  return 1 + 4 + u32;
}

int PWSfile::CheckPasskey(const StringX &filename,
                          const StringX &passkey, VERSION &version)
{
  if (passkey.empty())
    return PWScore::WRONG_PASSWORD;

  int status;
  version = UNKNOWN_VERSION;
  status = PWSfileV3::CheckPasskey(filename, passkey);
  fprintf(stderr, "PWSfile::CheckPasskey %d %u\n", status, version);
  if (status == SUCCESS)
    version = V30;
  return status;
}

void PWSfile::GetUnknownHeaderFields(UnknownFieldList &UHFL)
{
  if (!m_UHFL.empty())
    UHFL = m_UHFL;
  else
    UHFL.clear();
}

void PWSfile::SetUnknownHeaderFields(UnknownFieldList &UHFL)
{
  if (!UHFL.empty())
    m_UHFL = UHFL;
  else
    m_UHFL.clear();
}

static stringT ErrorMessages()
{
  stringT cs_text;

  switch (errno) {
  case EACCES:
    LoadAString(cs_text, IDSC_FILEREADONLY);
    break;
  case EEXIST:
    LoadAString(cs_text, IDSC_FILEEXISTS);
    break;
  case EINVAL:
    LoadAString(cs_text, IDSC_INVALIDFLAG);
    break;
  case EMFILE:
    LoadAString(cs_text, IDSC_NOMOREHANDLES);
    break;
  case ENOENT:
    LoadAString(cs_text, IDSC_FILEPATHNOTFOUND);
    break;
  case EIO: // synthesized upon fwrite failure
    LoadAString(cs_text, IDSC_FILEWRITEERROR);
    break;
  case EFBIG:
    LoadAString(cs_text, IDSC_FILE_TOO_BIG);
    break;
  default:
    break;
  }
  return cs_text;
}

// std::numeric_limits<>::max() && m'soft's silly macros don't work together
#ifdef max
#undef max
#endif

//-----------------------------------------------------------------
// A quick way to determine if two files are equal, or if a given
// file has been modified.

// For large files (>2K), we only hash the head & tail of the file.
// This is due to a performance trade-off.

// For unencrypted file, only checking the head & tail of large files may
// cause changes made to the middle of the file to be missed.
// However, since we write encrypted in CBC mode, this means that if the file
// was modified at offset X, then everything from X to the end of the file will
// be modified and the digests would be different.

PWSFileSig::PWSFileSig(const stringT &fname)
{
  const long THRESHOLD = 2048;
  unsigned char buf[THRESHOLD];

  m_length = 0;
  m_iErrorCode = PWSfile::SUCCESS;
  memset(m_digest, 0, sizeof(m_digest));
  FILE *fp = pws_os::FOpen(fname, _T("rb"));
  if (fp != NULL) {
    crypto_generichash_blake2b_state hash;

    crypto_generichash_blake2b_init(&hash, NULL, 0, sizeof(m_digest));
    m_length = pws_os::fileLength(fp);
    crypto_generichash_blake2b_update(&hash, reinterpret_cast<unsigned char*>(&m_length),
																	    sizeof(m_length));
    fprintf(stderr, "PWSFileSig::PWSFileSig len=%zu\n", m_length);
    // Not the right place to be worried about min size, as this is format
    // version specific (and we're in PWSFile).
    // An empty file, though, should be failed.
    if (m_length > 0) {
      if (m_length <= THRESHOLD) {
        if (fread(buf, size_t(m_length), 1, fp) == 1) {
          crypto_generichash_blake2b_update(&hash, buf, m_length);
          crypto_generichash_blake2b_final(&hash, m_digest, sizeof(m_digest));
        }
      } else { // m_length > THRESHOLD
        if (fread(buf, THRESHOLD / 2, 1, fp) == 1 &&
            fseek(fp, -THRESHOLD / 2, SEEK_END) == 0 &&
            fread(buf + THRESHOLD / 2, THRESHOLD / 2, 1, fp) == 1) {
          crypto_generichash_blake2b_update(&hash, buf, THRESHOLD);
          crypto_generichash_blake2b_final(&hash, m_digest, sizeof(m_digest));
        }
      }
    } else { // Empty file
      m_iErrorCode = PWScore::TRUNCATED_FILE;
    }

    fclose(fp);
  } else {
    m_iErrorCode = PWScore::CANT_OPEN_FILE;
  }
}

PWSFileSig::PWSFileSig(const PWSFileSig &pfs)
{
  m_length = pfs.m_length;
  m_iErrorCode = pfs.m_iErrorCode;
  memcpy(m_digest, pfs.m_digest, sizeof(m_digest));
}

PWSFileSig &PWSFileSig::operator=(const PWSFileSig &that)
{
  if (this != &that) {
    m_length = that.m_length;
    m_iErrorCode = that.m_iErrorCode;
    memcpy(m_digest, that.m_digest, sizeof(m_digest));
  }
  return *this;
}

bool PWSFileSig::operator==(const PWSFileSig &that)
{
  // Check this first as digest may otherwise be invalid
  if (m_iErrorCode != 0 || that.m_iErrorCode != 0)
    return false;

  return ((m_length == that.m_length) &&
          (crypto_verify_32(m_digest, that.m_digest) == 0));
}
