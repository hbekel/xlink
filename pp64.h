#ifndef PP64_H
#define PP64_H

#ifdef __cplusplus
extern "C" {
#endif

int pp64_configure(char* spec);
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
int pp64_drive_status(unsigned char* status);
int pp64_reset(void);

#ifdef __cplusplus
}
#endif

#endif // PP64_H
