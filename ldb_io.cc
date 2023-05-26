#include "storage/ldb/ldb_io.h"
#include <fcntl.h>
#include "my_dbug.h"
#include "my_sys.h"

IO::IO() {
  DBUG_TRACE;
  file = -1;
  record_header_size = sizeof(uchar) + sizeof(ull);
  header_size = sizeof(ull);
  totalRecord = 0;
}

IO::~IO() {
  if (file != -1) {
    my_close(file, MYF(0));
  }
  // fprintf(stderr, "%llu\n", totalRecord);
  file = 0;
}

int IO::openTable(const char *name, bool flag) {
  DBUG_TRACE;
  file = my_open(name, O_RDWR | O_CREAT, MYF(0));
  if (flag) {
    readHeader();
  }
  if (file == -1) {
    return 1;
  }
  return 0;
}

int IO::createTable(const char *name) {
  DBUG_TRACE;
  openTable(name, false);
  writeHeader();
  return 0;
}

int IO::closeTable() {
  DBUG_TRACE;
  if (file != -1) {
    my_close(file, MYF(0));
    file = -1;
  }
  return 0;
}

int IO::writeRow(const uchar *buf, unsigned long long length) {
  DBUG_TRACE;

  if (file == -1) {
    return -1;
  }

  int i = 0;
  uchar deleted = 0;
  // locate to the end of the file
  my_seek(file, 0, MY_SEEK_END, MYF(0));
  i = my_write(file, &deleted, sizeof(uchar), MYF(0));
  i = my_write(file, (uchar *)&length, sizeof(ull), MYF(0));
  i = my_write(file, buf, length, MYF(0));

  if (i == -1) {
    return -1;
  }
  ++totalRecord;
  // update Records;
  writeHeader();
  return 0;
}

int IO::readRow(uchar *buf, unsigned long long length, unsigned long long offset) {
  DBUG_TRACE;
  readHeader();
  // 空表格
  if (totalRecord == 0) {
    return -1;
  }

  my_off_t ii = 0;
  int i = 0;
  uchar deleted = 2;
  ull len = 0;
  // ull offset = 0;

  ii = my_seek(file, offset, MY_SEEK_SET, MYF(0));

  if (ii == MY_FILEPOS_ERROR) {
    return -1;
  }

  i = my_read(file, &deleted, sizeof(uchar), MYF(0));
  // my_seek(file, pos + sizeof(uchar), MY_SEEK_SET, MYF(0));
  // to skip those deleted rows
  // fprintf(stderr, "now read: %d\n", deleted);
  while (deleted == 1) {
    i = my_read(file, (uchar *)&len, sizeof(ull), MYF(0));
    if (i == 0 || i == -1) {
      return -1;
    }
    // i = my_read(file, buf, (length < len ? length : len), MYF(0));
    //  just jump to next record
    // fprintf(stderr, "deleted record found, len: %llu\n", len);
    my_seek(file, len, MY_SEEK_CUR, MYF(0));
    my_read(file, &deleted, sizeof(uchar), MYF(0));
  }
  i = my_read(file, (uchar *)&len, sizeof(ull), MYF(0));
  i = my_read(file, buf, (length < len ? length : len), MYF(0));

  // EOF or error
  if (i == -1 || i == 0) {
    return -1;
  }
  return 0;
}

// write totalRecord to the header of the file
int IO::writeHeader() {
  DBUG_TRACE;
  if (file == -1) {
    return -1;
  }

  my_seek(file, 0, MY_SEEK_SET, MYF(0));
  int flag = my_write(file, (uchar *)&totalRecord, sizeof(ull), MYF(0));
  if (flag == -1) {
    return -1;
  }
  return 0;
}

int IO::readHeader() {
  DBUG_TRACE;
  if (file == -1) {
    return -1;
  }

  my_seek(file, 0, MY_SEEK_SET, MYF(0));
  int flag = my_read(file, (uchar *)&totalRecord, sizeof(ull), MYF(0));
  if (flag == -1) {
    return -1;
  }
  return 0;
}

int IO::deleteAllRows() {
  if (file == -1) {
    return 1;
  } else {
    my_seek(file, 0, MY_SEEK_SET, MYF(0));
    my_chsize(file, 0, 0, MYF(0));
    totalRecord = 0;
    writeHeader();
    return 0;
  }
}

int IO::deleteRow(const uchar *buf, ull buflen) {
  DBUG_TRACE;
  auto rbuf = new uchar[buflen];
  ull len = 0;
  uchar deleted = 1;
  readHeader();
  if (totalRecord == 0) {
    return -1;
  }
  // jump the header
  my_seek(file, sizeof(ull), MY_SEEK_SET, MYF(0));
  ull offset = sizeof(ull);
  while (true) {
    my_seek(file, sizeof(uchar), MY_SEEK_CUR, MYF(0));
    my_read(file, (uchar *)&len, sizeof(ull), MYF(0));
    if (len != buflen) {
      my_seek(file, len, MY_SEEK_CUR, MYF(0));
      offset += (len + sizeof(ull) + sizeof(uchar));
      continue;
    }
    my_read(file, rbuf, len, MYF(0));
    if (memcmp(rbuf, buf, len) == 0) {
      break;
    } else {
      offset += (len + sizeof(ull) + sizeof(uchar));
    }
  }
  // fprintf(stderr, "offset: %llu\n", offset);
  //  my_seek(file, -len - sizeof(ull) - sizeof(uchar), MY_SEEK_CUR, MYF(0));
  my_seek(file, offset, MY_SEEK_SET, MYF(0));
  my_write(file, &deleted, sizeof(uchar), MYF(0));
  --totalRecord;
  writeHeader();
  delete rbuf;
  return 0;
}

my_off_t IO::curPos() {
  if (file == -1) {
    return MY_FILEPOS_ERROR;
  } else {
    return my_seek(file, 0, MY_SEEK_CUR, MYF(0));
  }
}

int IO::updateRow(const uchar *oldbuf, uchar *newbuf, ull len) {
  DBUG_TRACE;

  if (file == -1) {
    return -1;
  }

  readHeader();

  if (totalRecord == 0) {
    return -1;
  }

  uchar deleted = 2;
  ull buflen = 0;
  auto rbuf = new uchar[len];
  size_t read_data = 0;

  my_seek(file, sizeof(ull), MY_SEEK_SET, MYF(0));
  while (true) {
    read_data = my_read(file, &deleted, sizeof(uchar), MYF(0));
    if (read_data == 0) {
      return -1;
    }
    my_read(file, (uchar *)&buflen, sizeof(ull), MYF(0));
    if (deleted == 1) {
      my_seek(file, buflen, MY_SEEK_CUR, MYF(0));
      continue;
    }
    if (buflen != len) {
      my_seek(file, buflen, MY_SEEK_CUR, MYF(0));
      continue;
    }
    read_data = my_read(file, rbuf, len, MYF(0));
    if (read_data == 0) {
      return -1;
    }
    if (memcmp(rbuf, oldbuf, len) == 0) {
      break;
    }
  }
  my_seek(file, -len, MY_SEEK_CUR, MYF(0));
  my_write(file, newbuf, len, MYF(0));
  delete rbuf;

  return 0;
}