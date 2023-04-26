#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/farm.sck"
#define MAX_DATA_SIZE 1000





void c_start();

#endif