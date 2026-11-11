/* platinum3ds (from SoulSilver's pspshim, keep identical in layout): libntrwifi's CPS (cps/src/network.c, SDK_BUILD_LINUX
   branch, pspoke-patch 0012) finds the local IP with getifaddrs(). The 3DS has no such call; platinum3ds/platform/p3ds_net.c
   provides getifaddrs/freeifaddrs. Keep the struct in sync with the copy in p3ds_net.c. */
#ifndef SS3DS_IFADDRS_H
#define SS3DS_IFADDRS_H
#include <sys/socket.h>
/* network.c (Linux branch) creates sockets with SOCK_DGRAM|SOCK_NONBLOCK; newlib/libctru have no such flag. 0 = blocking
   sockets: connect() then simply blocks until done instead of returning EINPROGRESS, and the code already retries on a
   failed getpeername, so the logic still holds. */
#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK 0
#endif
struct ifaddrs {
    struct ifaddrs *ifa_next;
    char *ifa_name;
    unsigned int ifa_flags;
    struct sockaddr *ifa_addr;
    struct sockaddr *ifa_netmask;
};
int getifaddrs(struct ifaddrs **ifap);
void freeifaddrs(struct ifaddrs *ifa);
#endif
