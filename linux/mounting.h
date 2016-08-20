#define TEMP_DEVICE "/dev/rufus_device"
#define TEMP_LOOP "/dev/rufus_loop"
#define TEMP_PART "/dev/rufus_device_partition"

#define TEMP_DIR "/mnt/rufus_rootfs/"
#define TEMP_DIR_ISO "/mnt/rufus_isofs"

int make_temp_device(uint8_t major, uint8_t minor, uint32_t *device_fd);
int make_temp_partition(uint8_t major, uint8_t minor, uint32_t *part_fd);
int make_temp_dir(const char *path);
int recursive_copy( char *src,  char *dest);
void clean_up(const uint32_t *dev_fd, const uint32_t *part_fd, const uint32_t *loop_fd,
              const uint32_t *iso_fd);
int make_loop_device(uint32_t *loop_fd);
int mount_device_to_temp(const int32_t *file_system);
int mount_iso_to_loop(const char *isopath, int isopath_len, const uint32_t *loop_fd,  uint32_t *iso_fd);
