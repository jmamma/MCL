#include "MegaComFileServer.h"

MCFileServer::MCFileServer() {
  file.close();
  SD.chdir(true);
}

uint16_t MCFileServer::readstr(char* pstr) {
  uint16_t len = 0;
  while(pending()) {
    auto data = get();
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
    default:
      return -1;
  }
}

int MCFileServer::run() {
  state = 0;
  cmd = (filecommand_t) get();
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
    toggleLed2();
    reply_error("cwd failed. path = ", path);
  } else {
    reply_ok();
  }
  return -1;
}

void MCFileServer::reply_ok() {
  if (!megacom_task.tx_begin(msg.channel, msg.type, 1)) {
    return;
  }
  megacom_task.tx_data(msg.channel, FC_RSP_OK);
  megacom_task.tx_end(msg.channel);
}

void MCFileServer::reply_error(char* errmsg, char* more) {
  int len1 = strlen(errmsg);
  int len2 = 0;
  if(more != nullptr) len2 = strlen(more);
  if (!megacom_task.tx_begin(msg.channel, msg.type, 1 + len1 + len2)) {
    return;
  }
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

MCFileServer megacom_fileserver;
