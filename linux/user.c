#include <unistd.h>
#include <sys/types.h>

#include "user.h"

int check_root() {
     if (getuid() == 0) return 1;
     return 0;
}
