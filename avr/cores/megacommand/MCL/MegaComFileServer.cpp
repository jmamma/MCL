#include "MegaComFileServer.h"

MCFileServer::MCFileServer(): MegaComServer(false) {
  file.close();
  SD.chdir(true);
}

uint16_t MCFileServer::readstr(char* pstr) {
  uint16_t len = 0;
  while(pending()) {
    auto data = msg_getch();
    *pstr++ = data;
    if (data == 0) break;
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
  char path[128];
  uint16_t len = readstr(path);
  if (!SD.chdir(path, true)){
    reply_error("cwd failed. path = ", path);
  } else {
    reply_ok();
  }
  return -1;
}

void MCFileServer::reply_ok() {
  megacom_task.tx_begin(msg.channel, msg.type, 1);
  megacom_task.tx_data(msg.channel, FC_RSP_OK);
  megacom_task.tx_end(msg.channel);
}

void MCFileServer::reply_error(char* errmsg, char* more) {
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
    reply_ok();
    return -1;
  }
}

static uint32_t _fsize;

int MCFileServer::get() {
  char buf[513];
  if (state == 0) {
    file.close();
    readstr(buf);
    file.open(buf, O_READ);
    _fsize = file.size();
  }

  if (file.isOpen() && file.isFile() && file.position() != _fsize) {
    uint16_t len = file.read(buf, 512);
    // payload size: cmd_type(1) + data(read_size)
    megacom_task.tx_begin(msg.channel, msg.type, 1 + len);
    megacom_task.tx_data(msg.channel, FC_RSP_DATA);
    megacom_task.tx_vec(msg.channel, buf, len);
    megacom_task.tx_end(msg.channel);
    return 1;
  } else {
    file.close();
    reply_ok();
    return -1;
  }
}

MCFileServer megacom_fileserver;
