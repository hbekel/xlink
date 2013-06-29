#define PP64_PARPORT_CONTROL_STROBE 0x01
#define PP64_PARPORT_CONTROL_AUTOFD 0x02
#define PP64_PARPORT_CONTROL_INIT   0x04
#define PP64_PARPORT_CONTROL_SELECT 0x08
#define PP64_PARPORT_CONTROL_IRQ    0x10
#define PP64_PARPORT_CONTROL_INPUT  0x20

#define PP64_COMMAND_LOAD         0x01
#define PP64_COMMAND_SAVE         0x02
#define PP64_COMMAND_POKE         0x03
#define PP64_COMMAND_PEEK         0x04
#define PP64_COMMAND_JUMP         0x05
#define PP64_COMMAND_RUN          0x06
#define PP64_COMMAND_DOS          0x07
#define PP64_COMMAND_SECTOR_READ  0x08
#define PP64_COMMAND_SECTOR_WRITE 0x09

int pp64_configure(char* spec);
int pp64_open(void);
int pp64_ping(int timeout);
int pp64_load(unsigned char memory, unsigned char bank, int start, int end, char* data, int size);
int pp64_save(unsigned char memory, unsigned char bank, int start, int end, char* data, int size);
int pp64_peek(unsigned char memory, unsigned char bank, int address, unsigned char* value);
int pp64_poke(unsigned char memory, unsigned char bank, int address, unsigned char value);
int pp64_jump(unsigned char memory, unsigned char bank, int address);
int pp64_run(void);
int pp64_dos(char* cmd);
int pp64_sector_read(unsigned char track, unsigned char sector, unsigned char* data);
int pp64_sector_write(unsigned char track, unsigned char sector, unsigned char* data);
int pp64_reset(void);
void pp64_close(void);

void pp64_init(void);
unsigned char pp64_read(void);
void pp64_write(unsigned char byte);
void pp64_control(unsigned char ctrl);
unsigned char pp64_status(void);
unsigned char pp64_receive(void);
void pp64_send(unsigned char byte);
int pp64_send_with_timeout(unsigned char byte);
void pp64_send_signal_input(void);
void pp64_send_signal_output(void);
int pp64_receive_signal(int timeout); 

