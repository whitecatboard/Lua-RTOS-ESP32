#ifndef SD_H
#define SD_H

#define NSD             1
#define NPARTITIONS     4
#define SECTSIZE        512

int sd_init(int unit);
int sd_size(int unit);
int sd_write(int unit, unsigned offset, char *data, unsigned bcount);
int sd_read(int unit, unsigned int offset, char *data, unsigned int bcount);
int sd_has_partition(int unit, int type);
int sd_has_partitions(int unit);

#endif
