#ifndef DISK_H
#define DISK_H

#define THIRTY_FIVE_TRACKS 174848
#define THIRTY_FIVE_TRACKS_WITH_ERRORS 175531

#define FORTY_TRACKS 196608
#define FORTY_TRACKS_WITH_ERRORS 197376

typedef struct {
  unsigned char track;
  unsigned char number;
  int size;
  unsigned char error;
  unsigned char *bytes;
} Sector;

typedef struct {
  int number;
  int size;
  Sector** sectors;
} Track;

typedef struct {
  char *name;
  char id[3];
  int size;
  Track** tracks;
} Disk;

Disk* disk_new(int size);
Disk* disk_load(char* filename);
bool disk_each_sector(Disk* self, bool (*func) (Sector* sector));
bool disk_save(Disk* self, char *filename);
void disk_free(Disk* self);

Track* track_new(int number);
void track_load(Track* self, FILE* file);
bool track_each_sector(Track* self, bool (*func) (Sector* sector));
void track_free(Track* self);

Sector* sector_new(int track, int number);
void sector_load(Sector* self, FILE* file);
bool sector_equals(Sector *self, Sector* sector);
bool sector_print(Sector *self);
void sector_free(Sector *self);

#endif // DISK_H
