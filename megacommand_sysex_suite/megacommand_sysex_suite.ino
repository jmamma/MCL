constexpr uint8_t INPUT_BUF_SIZE = 64;
uint8_t serial_read;
uint8_t current_col;
char input_buf[INPUT_BUF_SIZE];
uint8_t input_len = 0;
uint8_t cmd_buf[64];
uint8_t cmd_len = 0;

void setup() {
  Serial2.begin(31250);
  Serial.begin(250000);
  Serial.println("MegaCommand A4 SYSEX Reverse Engineering Suite");

  current_col = 0;
}

bool is_hex(char x)
{
  return (x >= '0' && x <= '9') || (x >= 'a' && x <= 'f') || (x >= 'A' && x <= 'F');
}

bool is_sep(char x)
{
  return isspace(x) || (x == ',') || (x == ';') || (x == '|') || (x == '/');
}

bool readline()
{
  if (Serial.available() > 0) {
    serial_read = Serial.read();
    if(serial_read == '\n' || input_len + 1 == INPUT_BUF_SIZE) {
      // parse input to cmd
      input_buf[input_len] = 0;
      int input_scanned = 0;
      cmd_len = 0;
      while(1 == sscanf(input_buf + input_scanned, "%x", cmd_buf + cmd_len))
      {
        while(input_scanned < input_len && is_hex(input_buf[input_scanned])) { ++input_scanned; }
        while(input_scanned < input_len && is_sep(input_buf[input_scanned])) { ++input_scanned; }
        ++cmd_len;
      }
      
      input_len = 0;
      return true;
    }else {
      input_buf[input_len] = serial_read;
      input_len++; 
    }
  }

  return false;
}

void print_hex(uint8_t data)
{
  char out_buf[8];
  sprintf(out_buf, "%02x", data);
  Serial.print(out_buf);

  ++current_col;

  if(current_col == 8 || current_col == 24) {
    Serial.print(" ");
  }

  if(current_col == 16) {
    Serial.print(" |");
  }
  
  if(current_col >= 32) {
    Serial.println();
    current_col = 0;
  }else {
    Serial.print(" ");
  }
}

void newline()
{
  Serial.println("");
  Serial.println("-----------------------------------");    
  current_col = 0;
}

void send_request()
{
  switch(cmd_buf[0]){
    case 0x7c: 
      Serial.println(">> not sending command!");
      return;
    default:
      break;
  }

  newline();
  Serial.println(">> sending sysex data:");

  // sysex begin
  Serial2.write(0xf0);

  // a4 header
  Serial2.write(0x00);
  Serial2.write(0x20);
  Serial2.write(0x3c);
  Serial2.write(0x06);
  Serial2.write(0x00);

  Serial2.write(cmd_buf[0]);
  print_hex(cmd_buf[0]);

  // a4 proto. ver.
  Serial2.write(0x01);
  Serial2.write(0x01);

  for(int i=1; i<cmd_len; ++i)
  {
    Serial2.write(cmd_buf[i]);
    print_hex(cmd_buf[i]);
  }

  // a4 proto. endframe
  Serial2.write(0x00);
  Serial2.write(0x00);
  Serial2.write(0x00);
  Serial2.write(0x05);

  // sysex endframe
  Serial2.write(0xf7);
  newline();

  cmd_len = 0;
}

uint8_t i = 0;
uint8_t j = 0;

void byte_1_bitbang()
{
  cmd_len = 1;
  cmd_buf[0] = i++;
  send_request();
}

void byte_2_bitbang()
{
  cmd_len = 2;
  cmd_buf[0] = i++;
  if(cmd_buf[0] == 0) ++j;
  cmd_buf[1] = j;
  send_request();
}

void byte_3_bitbang()
{
  cmd_len = 3;
  cmd_buf[0] = i++;

  if(cmd_buf[0] == 0) ++j;
  
  cmd_buf[1] = j;
  cmd_buf[2] = 0x64;
  send_request();
}

void loop() {
  if (Serial2.available() > 0) {
    serial_read = Serial2.read();
    print_hex(serial_read);
  }

  // bit banging A4
//  if(readline()){
//    if(cmd_len == 0){
//      byte_3_bitbang();
//    }else{
//      send_request();
//    }
//  }

  if (readline()){
    send_request();
  }
}
