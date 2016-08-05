#ifndef INFO_H
#define INFO_H

#include <stdint.h>

typedef struct DEVICE {
  char device[4];
  char model[255];
  char vendor[255];
  uint64_t capacity;
} Device;

int scan_devices(struct DEVICE *dev, int array_size, uint8_t *discovered);
void trimwhitespace(char *str);
int buffread(char *buf, int sizeofbuf, char *path);


#endif // INFO_H
