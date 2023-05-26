#pragma once

#include "my_sys.h"

/*
 * file format:
 * file header: 8 bytes (use unsigned long long to store it, which is the number
 * of total records) record header: 9 bytes (1 byte for deleted flag, 8 bytes
 * for record length record: variable length
 */

/*
 * record format:
 * 1 byte: deleted flag (0 for not deleted, 1 for deleted)
 * 8 bytes: record length (use unsigned long long to store it)
 *
 */

typedef unsigned long long ull;

class IO {
 private:
  File file;
  int header_size;  // sizeof (unsigned long long) + sizeof (unsigned long long)
  int record_header_size;  // sizeof (unsigned long long) + sizeof(uchar)
  ull totalRecord;

 public:
  IO();
  ~IO();

  // table control
  int openTable(const char *name, bool flag = true);
  int createTable(const char *name);
  int closeTable();

  // file structure control
  int writeHeader();
  int readHeader();

  // basic operations write, read, and delete
  int writeRow(const uchar *buf, unsigned long long length);
  int readRow(uchar *buf, unsigned long long length, unsigned long long offset);
  int updateRow(const uchar *buf, uchar *newbuf, ull buflen);
  int deleteRow(const uchar *buf, ull buflen);
  int deleteAllRows();

  my_off_t curPos();
};