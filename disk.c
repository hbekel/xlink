#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "disk.h"

Disk* disk_new(char *filename) {
  
  Disk* disk = (Disk*) calloc(1, sizeof(Disk)); 

  FILE* file;
  struct stat st;
  long filesize;
  int i;

  if ((file = fopen(filename, "rb")) == NULL) {
    fprintf(stderr, "c64: error: load: '%s': %s\n", filename, strerror(errno));
    return NULL;
  }

  stat(filename, &st);
  filesize = st.st_size;

  disk->size = 0;

  if (filesize == THIRTY_FIVE_TRACKS || filesize == THIRTY_FIVE_TRACKS_WITH_ERRORS) {
    disk->size = 35;
  }

  if(filesize == FORTY_TRACKS || filesize == FORTY_TRACKS_WITH_ERRORS) {
    disk->size = 40;
  }

  disk->tracks = (Track**) calloc(disk->size, sizeof(Track*));
    
  for(i=0; i<disk->size; i++) {
    disk->tracks[i] = track_new(i+1, file);      
  }

  fclose(file);
  return disk;
}

void disk_free(Disk* self) {
  int t;

  for(t=0; t<self->size; t++) {
    track_free(self->tracks[t]);
  }
  free(self->tracks);
  free(self);
}

Track* track_new(int number, FILE* file) {  

  int i = 0;

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

  for(i=0; i<track->size; i++) {
    track->sectors[i] = sector_new(track->number, i, file);
  }
  return track;
}

void track_free(Track* self) {
  int s;

  for(s=0; s<self->size; s++) {
    sector_free(self->sectors[s]);
  }
  free(self->sectors);
  free(self);
}

Sector* sector_new(int track, int number, FILE *file) {

  Sector* sector = (Sector*) calloc(1, sizeof(Sector));

  sector->size = 256;
  sector->track = track;
  sector->number = number;

  sector->bytes = (unsigned char*) calloc(sector->size, sizeof(unsigned char));

  fread(sector->bytes, sizeof(unsigned char), sector->size, file);

  sector->error = false;
  return sector;
}

void sector_free(Sector* self) {
  free(self->bytes);
  free(self);
}

