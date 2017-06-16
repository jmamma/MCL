#include "FS_FileSystem.hh"
#include "GUI.h"
#include "LCD.h"
//  FS_FileSystem::FS_FileSystem() {
//  init();
//  }
    
  void FS_FileSystem::init() {
    active = FS_DISABLED;
    sd_raw_init();

    //sd_raw_get_info(&sdcard_info);
    if (check_header()) {
      active = FS_ENABLED;
    }
  }
  bool FS_FileSystem::write_header() {
    if (active == FS_ENABLED) {
       if (sd_raw_write(0, (uint8_t*)&header, FS_HEADER_RESERVED) == 1) {
       return true;
       }
    } 
    return false;
  }
  bool FS_FileSystem::read_header() {
     if (active == FS_ENABLED) {
       if (sd_raw_read(0, (uint8_t*)&header, FS_HEADER_RESERVED) == 1) {
       return true;
       }
     } 
     return false;
  }
  bool FS_FileSystem::check_header() {
    read_header(); 
    if (header.check == FS_ID) {
      if (header.version >= FS_VERSION) {
      return true;
      }
    }
    return false;
  }

  void FS_FileSystem::format() {
  //  init();
    uint16_t offset = 0;
  //  clearLed();
  //  clearLed2();
    header.check = FS_ID;
    header.number_of_entries = 0;
    header.version = FS_VERSION;
    header.fs_size = sdcard_info.capacity - FS_TABLE_RESERVED - FS_HEADER_RESERVED;
    header.fs_allocated = 0;
    uint8_t ledstatus = 0;

    for (int i = 0; i < FS_MAX_ENTRIES; i++) {
  // LCD.goLine(0);
  //   LCD.putnumber(i);
        if (i % 25 == 0) {
         if (ledstatus == 0) {
             setLed2();
             ledstatus = 1;
         }
         else {
          clearLed2(); 
             ledstatus = 0;
         }
        }   

            reset_entry(i);
    }
      clearLed2(); 

    write_header();
  }
  void FS_FileSystem::reset_entry(int i) {
    FS_FileEntry temp;
    read_entry(i,&temp);
    temp.active = FS_ENTRY_DISABLED;
    write_entry(i,&temp);
  } 
  void FS_FileSystem::del_entry(int i) {
    FS_FileEntry temp;
    read_entry(i,&temp);
    temp.active = FS_ENTRY_DELETED;
    write_entry(i,&temp);
    header.fs_used -= temp.size;
    write_header();
  }
  
  int FS_FileSystem::list_entries(char** buf,int n) {
    if (active != FS_ENABLED) { return 0; }

     FS_FileEntry temp;
     int count = 0;

     for (int i = 0; i < FS_MAX_ENTRIES; i++) {
       if ((count < n) && (count < header.number_of_entries)) {
       read_entry(i,&temp);
       m_strncpy(buf[count * FS_FILE_NAME_LENGTH],temp.path,FS_FILE_NAME_LENGTH); 
       if (temp.active != FS_ENTRY_DISABLED) {
         count++;
       }
       else {
         return count;
       }
     return count;
     }
    }
  }
  bool FS_FileSystem::write_entry(int i, FS_FileEntry *temp) {
   if (sd_raw_write(FS_HEADER_RESERVED + i * FS_ENTRY_SIZE, (uint8_t*)temp, FS_ENTRY_SIZE) == 1) {
      return true;
    }
    else {
      return false;
    }
  }
  
  bool FS_FileSystem::read_entry(int i, FS_FileEntry *temp) {
    if (sd_raw_read(FS_HEADER_RESERVED + i * FS_ENTRY_SIZE, (uint8_t*)temp, FS_ENTRY_SIZE) == 1) {
      return true;
    }
    else {
      return false;
    }
  }


  int FS_FileSystem::open(const char *path, uint32_t size, FS_FileEntry *fileentry) {
    if (active != FS_ENABLED) { return false; }
    
    if (size < FS_MIN_FILE_SIZE) {
    size = FS_MIN_FILE_SIZE;
    }
    FS_FileEntry entry;

    if (int i = find(path) < 0) {
       if (create(path, size, fileentry)) {
          return i;
        }
       else { return -1; }
    }
    else { 
        if (read_entry(i,fileentry)) {
          return i;
        }
        else { return -1; }
    }
  }

  int FS_FileSystem::find(const char *path) {
    if (active != FS_ENABLED) { return -1; }

    FS_FileEntry temp;
    int count = 0;
    
    for (int i = 0; i < FS_MAX_ENTRIES; i++) {
      
      if (count < header.number_of_entries) {
        read_entry(i,&temp);
        if (temp.active != FS_ENTRY_DISABLED) {
          count++;
          if (strcmp(&(temp.path[0]),path) == 0) {
            return i;
          }

         }
      }
      else {
        return - 1;
      }
    }
    return -1;
   }
  
   bool FS_FileSystem::create(const char *path, uint32_t size, FS_FileEntry *entry) {
     if (active != FS_ENABLED) { return false; }
     
     FS_FileEntry temp;
     FS_FileEntry prev;

     int best_match = -1;
     uint32_t best_match_size;
     
     int count = 0;
     int i = 0;
     //Search for existing entry that we can reclaim
     //We search all entries and find the smallest existing that 
     //will accomodate our new file.
     for ( i = 0; i < FS_MAX_ENTRIES; i++) {
    
       if (count < header.number_of_entries) {
        
         read_entry(i,&temp);
      
         if (temp.active == FS_ENTRY_DISABLED) {
            //Assume no more entries after finding the first disabled one
            break;
         }
         if (temp.active == FS_ENTRY_DELETED) {
            count++;
         //If the deleted entry has a filesize >= what we need, then let's reclaim it.
            if (temp.size >= size) {
              if ((temp.size < best_match) || (best_match == -1)) {
              best_match_size = temp.size;
              best_match = i;
              }
            }
          }
       }
       else {
         break;
       }

     }
    if (best_match > -1) {
      //We found an existing entry we can use
      read_entry(i,&temp);
      m_strncpy(&temp.path,path,FS_FILE_NAME_LENGTH);
      temp.path[FS_FILE_NAME_LENGTH - 1] = '\n';
      temp.size = size;
      temp.active = FS_ENTRY_ACTIVE;
      write_entry(best_match,&temp);
      read_entry(best_match,entry);
      header.fs_used += temp.size;
      write_header();
      return true;
    }
   //We couldn't find an existing entry, so let's find the next blank one and use that.
     for (i = 0; i < FS_MAX_ENTRIES; i++) {
       read_entry(i,&temp);
       
       if (temp.active == FS_ENTRY_DISABLED) {
         
         temp.size = size;
         temp.active = FS_ENTRY_ACTIVE;
         if (i == 0) {
            temp.offset = ((FS_HEADER_RESERVED + FS_TABLE_RESERVED) / FS_BLOCK_SIZE) + 1;
            temp.offset *= FS_BLOCK_SIZE;
         }
         else {
            read_entry(i - 1, &prev);
            //Calculate new offset, align to blocksize
            temp.offset = (((prev.offset + prev.size) / FS_BLOCK_SIZE) + 1) * FS_BLOCK_SIZE;
         
        }
         //Check that the allocated size won't exceed card full capacity

        if (temp.offset + size < sdcard_info.capacity) {

         m_strncpy(&temp.path,path,FS_FILE_NAME_LENGTH);
         temp.path[FS_FILE_NAME_LENGTH - 1] = '\n';
         
         //Write the entry to file table then read it in to entry object
         write_entry(i,&temp);
         read_entry(i,entry);
         //Increase total size allocated.
         header.fs_allocated += size;
         header.number_of_entries++;
         header.fs_used += temp.size;
         write_header();
         return true;
        }
        else {
          //Out of space
         return false;
        }
       }
     } 
       return false;
     }

  
  
