#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/mysocket2062"
#define MAX_DATA_SIZE 1000





void c_start();

#endif