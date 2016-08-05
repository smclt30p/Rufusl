#ifndef DOSFSTOOLS_FORMAT_FAT
#define DOSFSTOOLS_FORMAT_FAT

int format_device_fat(char *device, int fatsize, int sectorsize, char* label);

#endif // DOSFSTOOLS_FORMAT_FAT
