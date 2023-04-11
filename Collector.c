#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <MemoryManager.h>
#include <Collector.h>
#include <pthread.h>

void c_start() {

    DEBUGGER_PRINT_LOW("[Collector] started!!!!");
    int    serverSocket = -1, clientConnectionSocket = -1;
    int    rc;
    char   buffer[MAX_DATA_SIZE];

    serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("[Collector] socket() failed");
        return;
    }

    DEBUGGER_PRINT_LOW("[Collector] socket created");
    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);

    rc = bind(serverSocket, (struct sockaddr *) &address, SUN_LEN(&address));
    if (rc < 0) {
        perror("[Collector] bind() failed");
        return;
    }

    DEBUGGER_PRINT_LOW("[Collector] bind done");

    rc = listen(serverSocket, 1);
    if (rc < 0) {
        perror("[Collector] listen() failed");
        return;
    }

    DEBUGGER_PRINT_LOW("[Collector] Ready for client connect()");

    clientConnectionSocket = accept(serverSocket, NULL, NULL);
    if (clientConnectionSocket < 0) {
        perror("accept() failed");
        return;
    }

    DEBUGGER_PRINT_LOW("[Collector] client connected %d", clientConnectionSocket);

    int i = 5;
    rc = recv(clientConnectionSocket, buffer, sizeof(buffer), 0);
    if (rc < 0) {
        perror("[Collector] recv() failed");
        return;
    }
    printf("[Collector] %s | %d bytes of data were received\n", buffer, rc);




    /********************************************************************/
    /* Program complete                                                 */
    /********************************************************************/

    sleep(1);
    unlink(SOCKET_PATH);
    close(clientConnectionSocket);
    close(serverSocket);

    return;

}