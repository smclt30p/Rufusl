#define TEMP_DEVICE "/dev/rufus_device"
#define TEMP_PART "/dev/rufus_device_partition"
#define TEMP_DIR "/tmp/rufus_rootfs"

int make_temp_device(uint8_t minor, uint8_t major);
int make_temp_partition(uint8_t minor, uint8_t major);
int make_temp_dir();
