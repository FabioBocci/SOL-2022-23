#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/mysocket2052"
#define MAX_DATA_SIZE 500





void c_start();

#endif