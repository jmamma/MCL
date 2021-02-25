#pragma once

#include "MegaComTask.h"
#include "MCLSd.h"

enum filecommand_t {
  FC_CWD = 0x00,
  FC_LS = 0x01,
  FC_GET = 0x02,
  FC_PUT_BEGIN = 0x03,
  FC_PUT_DATA = 0x04,
  FC_PUT_END = 0x05,
  FC_DELETE = 0x06,
  FC_RENAME = 0x07,

  FC_RSP_OK = 0x08,
  FC_RSP_DATA = 0x09, // one data item in the stream. reply OK to indicate termination.
  FC_RSP_ERROR = 0x0a,
};

class MCFileServer: public MegaComServer {
private:
  File file;
  filecommand_t cmd;
  int state;
  bool fileop_active;
public:
  MCFileServer();
  int cwd();
  int ls();
  int get();
  int put_begin();
  int put_data();
  int put_end();
  int rename();
  void reply_ok();
  void reply_error(const char*, const char*);
  int dispatch();
  uint16_t readstr(char*);
  virtual int run();
  virtual int resume(int);
};

extern MCFileServer megacom_fileserver;
