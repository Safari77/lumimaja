/*
* Copyright (c) 2003-2015 Rony Shapiro <ronys@users.sourceforge.net>.
* All rights reserved. Use of the code is allowed under the
* Artistic License 2.0 terms, as specified in the LICENSE file
* distributed with this code, or available from
* http://www.opensource.org/licenses/artistic-license-2.0.php
*/

#ifndef __PWSFILE_H
#define __PWSFILE_H

// PWSfile.h
// Abstract the gory details of reading and writing an encrypted database
//-----------------------------------------------------------------------------

#include <stdio.h> // for FILE *
#include <vector>
#include "argon2/argon2.h"

#include "ItemData.h"
#include "os/UUID.h"
#include "UnknownField.h"
#include "StringX.h"
#include "Proxy.h"
#include "coredefs.h"

// MIN_HASH_PASSES/MIN_HASH_MEM_KIB are used by the Argon2 KDF
#define MIN_HASH_PASSES  ((uint32)1)
#define MAX_HASH_PASSES  ((uint32)1000)
#define MIN_HASH_MEM_KIB ((uint32)32<<10) // really ARGON2_MIN_MEMORY
#define MAX_HASH_MEM_KIB ((uint32)32<<20)

#define DEFAULT_SUFFIX      _T("lumi3")

class Fish;
class Asker;

class PWSfile
{
public:
  enum VERSION {V30, VCURRENT = V30,
    NEWFILE = 98,
    UNKNOWN_VERSION = 99}; // supported file versions: V17 is last pre-2.0

  enum RWmode {Read, Write};

  enum {SUCCESS = 0, FAILURE = 1 };

  /**
  * The format defines a handful of fields in the file's header
  * Since the application needs these after the PWSfile object's
  * lifetime, it makes sense to define a nested header structure that
  * the app. can keep a copy of, rather than duplicating
  * data members, getters and setters willy-nilly.
  */
  struct HeaderRecord {
    HeaderRecord();
    HeaderRecord(const HeaderRecord &hdr);
    HeaderRecord &operator =(const HeaderRecord &hdr);
    ~HeaderRecord();
    unsigned short m_nCurrentMajorVersion, m_nCurrentMinorVersion;
    pws_os::CUUID m_file_uuid;         // Unique DB ID
    std::vector<bool> m_displaystatus; // Tree expansion state vector
    StringX m_prefString;              // Prefererences stored in the file
    uint64 m_whenlastsaved; // When last saved
    StringX m_lastsavedby; // and by whom
    StringX m_lastsavedon; // and by which machine
    StringX m_whatlastsaved; // and by what application
    StringX m_dbname, m_dbdesc;        // Descriptive name, Description
    UUIDList m_RUEList;
    unsigned char *m_yubi_sk;  // YubiKey HMAC key, added in 0x030a / 3.27Y
    enum {YUBI_SK_LEN = 20};
  };

  static PWSfile *MakePWSfile(const StringX &a_filename, VERSION &version,
                              RWmode mode, int &status, 
                              Asker *pAsker = NULL, Reporter *pReporter = NULL);

  static VERSION ReadVersion(const StringX &filename);
  static int CheckPasskey(const StringX &filename,
                          const StringX &passkey, VERSION &version);

  virtual ~PWSfile();

  virtual int Open(const StringX &passkey) = 0;
  virtual int Close();
  virtual int CloseSync(); /* also fsync */

  virtual int WriteRecord(const CItemData &item) = 0;
  virtual int ReadRecord(CItemData &item) = 0;

  const HeaderRecord &GetHeader() const {return m_hdr;}
  void SetHeader(const HeaderRecord &h) {m_hdr = h;}

  void SetDefUsername(const StringX &du) {m_defusername = du;} // for V17 conversion (read) only
  void SetCurVersion(VERSION v) {m_curversion = v;}
  void GetUnknownHeaderFields(UnknownFieldList &UHFL);
  void SetUnknownHeaderFields(UnknownFieldList &UHFL);
  int GetNumRecordsWithUnknownFields() const
    {return m_nRecordsWithUnknownFields;}
  virtual size_t WriteRaw(unsigned char type, const StringX &data);
  virtual size_t WriteRaw(unsigned char type, const unsigned char *data,
                          size_t length);

  virtual size_t ReadRaw(unsigned char &type, unsigned char* &data,
                         size_t &length);

protected:
  std::vector<uint8_t> m_rawdata;
  size_t m_rawpos;
  CUTF8Conv m_utf8conv;

  PWSfile(const StringX &filename, RWmode mode);
  void FOpen(); // calls right variant of m_fd = fopen(m_filename);
  const StringX m_filename;
  stringT m_filename_tmp;
  StringX m_passkey;
  FILE *m_fd;
  VERSION m_curversion;
  const RWmode m_rw;
  StringX m_defusername; // for V17 conversion (read) only
  HeaderRecord m_hdr;
  // Save unknown header fields on read to put back on write unchanged
  UnknownFieldList m_UHFL;
  int m_nRecordsWithUnknownFields;
  ulong64 m_fileLength;
  Asker *m_pAsker;
  Reporter *m_pReporter;

private:
  PWSfile& operator=(const PWSfile&); // Do not implement
};

// A quick way to determine if two files are equal,
// or if a given file has been modified. For large files,
// this may miss changes made to the middle. This is due
// to a performance trade-off.
class PWSFileSig
{
public:
  PWSFileSig(const stringT &fname);
  PWSFileSig(const PWSFileSig &pfs);
  PWSFileSig &operator=(const PWSFileSig &that);

  bool IsValid() {return m_iErrorCode == PWSfile::SUCCESS;}
  int GetErrorCode() {return m_iErrorCode;}

  bool operator==(const PWSFileSig &that);
  bool operator!=(const PWSFileSig &that) {return !(*this == that);}

private:
  ulong64 m_length; // -1 if file doesn't exist or zero length
  unsigned char m_digest[32];
  int m_iErrorCode;
};
#endif /* __PWSFILE_H */
