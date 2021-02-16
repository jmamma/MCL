#pragma once

#include "MegaComTask.h"
#include "MCLSd.h"

enum filecommand_t {
  FC_CWD,
  FC_LS,
  FC_GET,
  FC_PUT_BEGIN,
  FC_PUT_DATA,
  FC_PUT_END,
  FC_DELETE,
  FC_RENAME,

  FC_RSP_OK,
  FC_RSP_DATA, // one data item in the stream. reply OK to indicate termination.
  FC_RSP_ERROR,
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
  void reply_ok();
  void reply_error(const char*, const char*);
  int dispatch();
  uint16_t readstr(char*);
  virtual int run();
  virtual int resume(int);
};

extern MCFileServer megacom_fileserver;
