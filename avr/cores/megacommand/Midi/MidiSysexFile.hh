#ifndef MIDISYSEXFILE_H__
#define MIDISYSEXFILE_H__

class MidiSysexFile {
  
  File file;
  
  bool send(char *file_name) {
    
    bool ret;
    ret = file.open(file_name, O_RD);
    
    if (!ret) { 
        DEBUG_PRINTLN("Could not open SYSEX file:");
        DEBUG_PRINTLN(file_name);
        return false; 
    }

    uint8_t buf[512];

    uint32_t file_size = file.fileSize();

    uint32_t bytes_read = 0;

    while (bytes_read < file_size) {

       uint16_t len;

       if (file_size - bytes_read < 512) {
          len = file_size - bytes_read;
       }
    
       else {
          len = 512;
       }

       if (mcl_sd.read_data(&buf,len,&file)) {
          bytes_read += len;
       }
       
       if (bytes_read == 0) {
         if (buf[0] != 0xF0) {
           DEBUG_PRINTLN("incorrect file type");
           return false;
         } 
       }
       for (uint8_t c = 0; c < len; c++) {
          m_putc(buf[c]);
       }

       else {
          DEBUG_PRINTLN("failed to send");
          return false;
       }

    }

  }
  bool recv();

};

#endif /* MIDISYSEXFILE_H__ */
