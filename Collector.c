#undef M_DEBUG
#define M_DEBUG 0

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <MemoryManager.h>
#include <Collector.h>
#include <pthread.h>
#include <string.h>

struct element {
    char * path;
    long value;
    struct element* next;
};

// create a new elemento for the list
struct element* create_element(char* path, long value) {
    struct element* new_elem;
    MM_MALLOC(new_elem, struct element);

    new_elem->path = strdup(path);
    new_elem->value = value;
    new_elem->next = NULL;
    return new_elem;
}

//insert a new value to the struct
void insert_element(struct element** head, char* path, long value) {
    struct element* new_elem = create_element(path, value);
    if (*head == NULL || value < (*head)->value) {
        new_elem->next = *head;
        *head = new_elem;
    } else {
        struct element* curr = *head;
        while (curr->next != NULL && curr->next->value <= value) {
            curr = curr->next;
        }
        new_elem->next = curr->next;
        curr->next = new_elem;
    }
}

// funzione per stampare la lista
void print_list(struct element* head) {
    struct element* curr = head;
    while (curr != NULL) {
        // Check if the path starts with "./"
        if (strncmp(curr->path, "./", 2) == 0) {
            // Remove the first two characters
            memmove(curr->path, curr->path + 2, strlen(curr->path) - 1);
        }
        fprintf(stdout, "%ld \t %s\n", curr->value, curr->path);
        curr = curr->next;
    }
}


// funzione per liberare la memoria allocata per la lista
void free_list(struct element* head) {
    struct element* curr = head;
    while (curr != NULL) {
        struct element* temp = curr;
        curr = curr->next;
        MM_FREE(temp->path);
        MM_FREE(temp);
    }
}

void c_start() {

    DEBUGGER_PRINT_LOW("[Collector] started!");
    int    serverSocket = -1, clientConnectionSocket = -1;
    int    rc;
    char   buffer[MAX_DATA_SIZE];
    struct element * head = NULL;

    serverSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        perror("[Collector] socket() failed");
        return;
    }

    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strcpy(address.sun_path, SOCKET_PATH);

    rc = bind(serverSocket, (struct sockaddr *) &address, SUN_LEN(&address));
    if (rc < 0) {
        perror("[Collector] bind() failed");
        return;
    }


    rc = listen(serverSocket, 1);
    if (rc < 0) {
        perror("[Collector] listen() failed");
        return;
    }

    DEBUGGER_PRINT_LOW("[Collector] Ready for client connect()");

    clientConnectionSocket = accept(serverSocket, NULL, NULL);
    if (clientConnectionSocket < 0) {
        perror("[Collector] accept() failed");
        return;
    }


    int stop = 0;
    while (stop == 0) {
        DEBUGGER_PRINT_LOW("[Collector] waiting for reciving message ");
        memset(buffer, 0, sizeof(buffer));
        rc = recv(clientConnectionSocket, buffer, sizeof(buffer), 0);
        if (rc < 0) {
            perror("[Collector] recv() failed");
            return;
        }
        DEBUGGER_PRINT_LOW("[Collector] recived:  %s ", buffer);


        char * pathOrMaster = strtok(buffer, ":");
        if (strcmp(pathOrMaster, "MASTER") == 0) {
            //a message from master worker
            char * command = strtok(NULL, ":");
            DEBUGGER_PRINT_LOW("[Collector] command: %s  cmpSTAMP: %d  cmpSTOP: %d", command, strcmp(command, "STAMP"), strcmp(command, "CLOSE"));
            if (strcmp(command, "STAMP") == 0) {
                print_list(head);
            } else if (strcmp(command, "CLOSE") == 0) {
                print_list(head);
                stop = 1;
                break;
            }
        } else {
            //message from collector full path:fileName:value
            char * fileName = strtok(NULL, ":");
            char * valueString = strtok(NULL, ":");
            long value = atol(valueString);

            DEBUGGER_PRINT_LOW("[Collector] <FilePath : file name : value> | %s | %s | %ld ", pathOrMaster, fileName, value);
            insert_element(&head, pathOrMaster, value);
        }
    }

    free_list(head);
    unlink(SOCKET_PATH);
    close(clientConnectionSocket);
    close(serverSocket);

    return;

}