int cable_open(void);
void cable_close(void);
int cable_load(unsigned char memory, unsigned char cartridge, int start, int end, char* data, int size);
int cable_save(unsigned char memory, unsigned char cartridge, int start, int end, char* data, int size);
int cable_jump(int address);
void run(void);
void cable_reset(void);
unsigned char cable_read(void);
void cable_write(unsigned char byte);
int cable_write_with_timeout(unsigned char byte);
void _cable_send_signal_input(void);
void _cable_send_signal_output(void);
int _cable_receive_signal(struct timeval* timeout); 
