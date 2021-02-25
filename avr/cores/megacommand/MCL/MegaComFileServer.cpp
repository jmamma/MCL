#include "MegaComFileServer.h"

#define FILEOP_BEGIN() \
  if (fileop_active) { \
    reply_error("device is busy", ""); \
    return -1; \
  } \
  fileop_active = true; \

#define FILEOP_END() \
  fileop_active = false; \
  

MCFileServer::MCFileServer(): MegaComServer(false) {
  file.close();
  SD.chdir(true);
  fileop_active = false;
}

uint16_t MCFileServer::readstr(char* pstr) {
  uint16_t len = 0;
  while(pending()) {
    auto data = msg_getch();
    *pstr++ = data;
    if (data == 0) break;
    ++len;
  }
  if (pstr[-1] != 0) {
    pstr[0] = 0; 
    ++len;
  }
  return len;
}

int MCFileServer::dispatch() {
  switch (cmd) {
    case FC_CWD:
      return cwd();
    case FC_LS:
      return ls();
    case FC_GET:
      return get();
    case FC_PUT_BEGIN:
      return put_begin();
    case FC_PUT_DATA:
      return put_data();
    case FC_PUT_END:
      return put_end();
    case FC_RENAME:
      return rename();
    default:
      return -1;
  }
}

int MCFileServer::run() {
  state = 0;
  cmd = (filecommand_t) msg_getch();
  return dispatch();
}

int MCFileServer::resume(int state) {
  this->state = state;
  return dispatch();
}

int MCFileServer::cwd() {
  char path[256];
  readstr(path);
  if (!SD.chdir(path, true)){
    reply_error("cwd failed. path = ", path);
  } else {
    reply_ok();
  }
  return -1;
}

int MCFileServer::rename() {
  char name1[128];
  char name2[128];
  int len1 = readstr(name1);
  int len2 = readstr(name2);

  if (len1 <= 0 || len2 <= 0) {
    reply_error("invalid file names.", "");
    return -1;
  }

  FILEOP_BEGIN();
  file.close();
  if (SD.rename(name1, name2)) {
    reply_ok();
  } else {
    reply_error("rename failed", "");
  }
  FILEOP_END();

  return -1;
}

void MCFileServer::reply_ok() {
  megacom_task.tx_begin(msg.channel, msg.type, 1);
  megacom_task.tx_data(msg.channel, FC_RSP_OK);
  megacom_task.tx_end(msg.channel);
}

void MCFileServer::reply_error(const char* errmsg, const char* more) {
  int len1 = strlen(errmsg);
  int len2 = 0;
  if(more != nullptr) len2 = strlen(more);
  megacom_task.tx_begin(msg.channel, msg.type, 1 + len1 + len2);
  megacom_task.tx_data(msg.channel, FC_RSP_ERROR);
  megacom_task.tx_vec(msg.channel, errmsg, len1);
  megacom_task.tx_vec(msg.channel, more, len2);
  megacom_task.tx_end(msg.channel);
}

int MCFileServer::ls() {

  if (state == 0) {
    FILEOP_BEGIN();
    SD.vwd()->rewind();
  }
  char path[128];
  bool isdir;
  int len;
  unsigned long fsize;
  if (file.openNext(SD.vwd(), O_READ)) {
    isdir = file.isDirectory();
    file.getName(path, sizeof(path));
    len = strlen(path);
    file.close();
    fsize = file.size();
    // payload size: cmd_type(1) + is_dir(1) + size(4) + filename(len)
    megacom_task.tx_begin(msg.channel, msg.type, 6 + len);
    megacom_task.tx_data(msg.channel, FC_RSP_DATA);
    megacom_task.tx_data(msg.channel, isdir);
    megacom_task.tx_dword(msg.channel, fsize);
    megacom_task.tx_vec(msg.channel, path, len);
    megacom_task.tx_end(msg.channel);

    return 1;
  } else {
    FILEOP_END();
    reply_ok();
    return -1;
  }
}

static uint32_t _fsize;

int MCFileServer::get() {
  char buf[513];
  if (state == 0) {
    FILEOP_BEGIN();

    file.close();
    readstr(buf);
    file.open(buf, O_READ);
    _fsize = file.size();
  }

  if (file.isOpen() && file.isFile() && file.position() != _fsize) {
    int16_t len = file.read(buf, 512);
    // payload size: cmd_type(1) + data(read_size)
    comstatus_t status = megacom_task.tx_checkstatus(COMCHANNEL_UART_USB, COMSERVER_FILESERVER);
    if (status == CS_TX_ACTIVE) {
      // previous tx still on-wire
      goto rollback;
    }
    if (status == CS_RESEND) {
      // the previosu tx failed. prepare to resend.
      file.seekCur(-512);
      goto rollback;
    }
    if (!megacom_task.tx_begin(msg.channel, msg.type, 1 + len)) {
      // buffer full
      goto rollback;
    }
    megacom_task.tx_data(msg.channel, FC_RSP_DATA);
    megacom_task.tx_vec(msg.channel, buf, len);
    megacom_task.tx_end(msg.channel);
    return 1;
rollback:
    file.seekCur(-len);
    return 1;
  } else {
    file.close();
    FILEOP_END();
    reply_ok();
    return -1;
  }
}

int MCFileServer::put_begin() {
  char path[256];

  FILEOP_BEGIN();
  file.close();
  readstr(path);
  if (!file.open(path, O_WRITE | O_CREAT)) {
    FILEOP_END();
    reply_error("put_begin: cannot open file", "");
    return -1;
  }
  reply_ok();
  return -1;
}

int MCFileServer::put_data() {
  char buf[513];
  uint16_t len = msg.len;

  if (!fileop_active) {
    reply_error("invalid request", "");
    return -1;
  }

  if (len > sizeof(buf)) {
    reply_error("put_data size too big","");
    file.close();
    FILEOP_END();
    return -1;
  }

  for(uint16_t i = 0; i < len; ++i) {
    buf[i] = msg_getch();
  }

  if (len == file.write(buf, len)) {
    reply_ok();
  } else {
    file.close();
    FILEOP_END();
    reply_error("file write error", "");
  }

  return -1;
}

int MCFileServer::put_end() {
  if (!fileop_active) {
    reply_error("invalid request", "");
    return -1;
  }

  file.close();
  FILEOP_END();

  reply_ok();
  return -1;
}

MCFileServer megacom_fileserver;
