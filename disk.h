#define THIRTY_FIVE_TRACKS 174848
#define THIRTY_FIVE_TRACKS_WITH_ERRORS 175531

#define FORTY_TRACKS 196608
#define FORTY_TRACKS_WITH_ERRORS 197376

typedef struct {
  int track;
  int number;
  int size;
  unsigned char *bytes;
  bool error;
} Sector;

typedef struct {
  int number;
  int size;
  Sector** sectors;
} Track;

typedef struct {
  int size;
  Track** tracks;
} Disk;

Disk* disk_new(char *filename);
void disk_free(Disk *self);

Track* track_new(int number, FILE* file);
void track_free(Track *self);

Sector* sector_new(int track, int number, FILE* file);
void sector_free(Sector *self);
