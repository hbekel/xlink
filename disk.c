#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "disk.h"

Disk* disk_new(int size) {
  
  Disk* disk = (Disk*) calloc(1, sizeof(Disk)); 

  disk->name = (char *) calloc(17, sizeof(char));
  strncpy(disk->name, "UNKNOWN", 16);

  disk->id[0] = disk->id[1] = '0';
  disk->id[2] = '\0';

  disk->size = size;
  disk->tracks = (Track**) calloc(disk->size, sizeof(Track*));

  for (int i=0; i<disk->size; i++) {
    disk->tracks[i] = track_new(i+1);      
  }
  return disk;
}

Disk* disk_load(char *filename) {

  Disk* disk;
  FILE* file;
  struct stat st;
  long filesize;
  int size;

  if ((file = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "error loading d64 file: '%s': %s\n", filename, strerror(errno));
    return NULL;
  }

  stat(filename, &st);
  filesize = st.st_size;

  size = 0;

  if (filesize == THIRTY_FIVE_TRACKS || 
      filesize == THIRTY_FIVE_TRACKS_WITH_ERRORS) {
    size = 35;
  }
  else if (filesize == FORTY_TRACKS || 
      filesize == FORTY_TRACKS_WITH_ERRORS) {
    size = 40;
  }
  else {
    fprintf(stderr, "error loading d64 file: file size mismatch\n");
    return NULL;
  }

  disk = disk_new(size);

  for (int i=0; i<disk->size; i++) {
    track_load(disk->tracks[i], file);      
  }

  if (filesize == THIRTY_FIVE_TRACKS_WITH_ERRORS ||
      filesize == FORTY_TRACKS_WITH_ERRORS) {    
    
    bool sector_set_error(Sector *sector) {
      
      unsigned char byte;
      
      fread(&byte, sizeof(unsigned char), 1, file);      
      sector->error = byte;

      return true;
    }
    disk_each_sector(disk, &sector_set_error);
  }
  fclose(file);
  
  Sector *bam = disk->tracks[17]->sectors[0];

  for(int i=0; i<16; i++) {
    disk->name[i] = bam->bytes[0x90+i];
    disk->name[i] = (unsigned char) disk->name[i] == 0xa0 ? '\0' : disk->name[i];
  }

  for(int i=0; i<2; i++) {
    disk->id[i] = bam->bytes[0xa2+i];
  }

  return disk;
}

bool disk_each_sector(Disk *self, bool (*func) (Sector* sector)) {
  
  bool result = true;

  Track *track;
  Sector *sector;


  for(int t=0; t<self->size; t++) {
    track = self->tracks[t];
    
    for(int s=0; s<track->size; s++) {
      sector = track->sectors[s];
      if(!func(sector)) {
	result = false;
	goto abort;
      }
    }
  }

 abort:
  return result;
}

bool disk_save(Disk* self, char *filename) {

  FILE *file;
  
  bool save_sector(Sector* sector) {
    fwrite(sector->bytes, sizeof(unsigned char), sector->size, file);
    return true;
  }

  if((file = fopen(filename, "wb")) == NULL) {
    fprintf(stderr, "error opening %s\n", filename);
    return false;
  }

  if(!disk_each_sector(self, &save_sector)) {
    fprintf(stderr, "error saving sector\n");
    return false;
  }
  
  fclose(file);
  return true;
}

void disk_free(Disk* self) {

  free(self->name);

  for(int t=0; t<self->size; t++) {
    track_free(self->tracks[t]);
  }  
  free(self->tracks);
  free(self);
}

Track* track_new(int number) {  

  Track* track = (Track*) calloc(1, sizeof(Track));

  track->number = number;
  track->size = 0;

  if (track->number >= 1 && track->number <= 17) {
    track->size = 21;
  }

  if (track->number >= 18 && track->number <= 24) {
    track->size = 19;
  }  

  if (track->number >= 25 && track->number <= 30) {
    track->size = 18;
  }  

  if (track->number >= 31 && track->number <= 40) {
    track->size = 17;
  }  
  
  track->sectors = (Sector**) calloc(track->size, sizeof(Sector*));

  for(int i=0; i<track->size; i++) {
    track->sectors[i] = sector_new(track->number, i);
  }
  return track;
}

void track_load(Track* self, FILE *file) {

  for(int i=0; i<self->size; i++) {
    sector_load(self->sectors[i], file);
  }
}

bool track_each_sector(Track* self, bool (*func) (Sector* sector)) {

  Sector* sector;
  int result = true;

  for(int s=0; s<self->size; s++) {
    sector = self->sectors[s];
    if(!func(sector)) {
      result = false;
      goto abort;
    }
  }
 abort:
  return result;
}

void track_free(Track* self) {

  for(int s=0; s<self->size; s++) {
    sector_free(self->sectors[s]);
  }
  free(self->sectors);
  free(self);
}

Sector* sector_new(int track, int number) {
  Sector* sector = (Sector*) calloc(1, sizeof(Sector));

  sector->track = track;
  sector->number = number;
  sector->size = 256;
  sector->error = 0;  

  sector->bytes = (unsigned char*) calloc(sector->size, sizeof(unsigned char));
  return sector;
}

void sector_load(Sector* self, FILE *file) {
  fread(self->bytes, sizeof(char), self->size, file);
}

bool sector_equals(Sector* self, Sector* sector) {
  bool result = true;

  for(int i=0; i<self->size; i++) {
    if (self->bytes[i] != sector->bytes[i]) {
      result = false;
      break;
    }
  }
  return result;
}

bool sector_print(Sector *self) {
  
  printf("%d:%d (%02X)\n", self->track, self->number, self->error);

  for(int i=0; i<self->size; i++) {

    printf("%02X ", (unsigned char) self->bytes[i]);

    if ((i+1) % 24 == 0) {
      printf("\n");
    }
  }
  printf("\n\n");
  return true;
}

void sector_free(Sector* self) {
  free(self->bytes);
  free(self);
}

