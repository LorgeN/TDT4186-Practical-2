#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "worker.h"
#include <time.h>

typedef struct server_t
{
    int sock;
    struct sockaddr_in addr;
} server_t;

typedef struct conn_t
{
    struct sockaddr_in remote_addr; // todo: remove this later
    struct sockaddr_in local_addr;
} conn_t;

struct mtwww_options_t {
    char *path;
    int port;
    int worker_threads;
    int buffer_slots;   
};

// Keep here so that it is accessible to everything
struct mtwww_options_t opt;
    
int __file_isdir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0; // Not a directory
    }

    return S_ISDIR(st.st_mode);
}

int __read_options(struct mtwww_options_t *opt, int argc, char **argv) {
    if (argc != 5) {
        printf("Invalid number of arguments %d, expected 4\n", argc - 1);
        printf("Usage: ./mtwwwd <www-path> <port> <worker_threads> <buffer_slots>\n");
        return 1;
    }

    char *tmp, *path;
    int port, workers, buffer_slots;
    
    path = argv[1];
    // Check if the path exists, and is a directory
    if (!__file_isdir(path)) {
        printf("%s does not exist/is not a directory\n", path);
        return 1;
    }

    tmp = argv[2];
    if (!(port = atoi(tmp))) {
        printf("Invalid port number %s\n", tmp);
        return 1;
    }

    tmp = argv[3];
    if (!(workers = atoi(tmp))) {
        printf("Invalid worker thread count %s\n", tmp);
        return 1;    
    }

    tmp = argv[4];
    if (!(buffer_slots = atoi(tmp))) {
        printf("Invalid buffer slot count %s\n", tmp);
        return 1;
    }

    // Wait until everything is valid to update variables
    opt->path = path;
    opt->port = port;
    opt->worker_threads = workers;
    opt->buffer_slots = buffer_slots;
    return 0;
}

void read_file(char *filename, char* cwd, int client_socket) {
    FILE *fptr;
    
    char abs_path[512] = {0}; 
    strcpy(abs_path, cwd);
    strncat(abs_path, filename, 512 - strlen(abs_path));

    printf("%s\n", abs_path);
    if ((fptr = fopen(abs_path, "rb")) == NULL) {
        printf("Error trying to read file");
        exit(1);
    }
    fseek(fptr, 0, SEEK_END);
    long size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    printf("%lu", size);

    char buffer[1000] = {0};
    while (fgets(buffer, sizeof(buffer), fptr) != NULL)
    {
        if (send(client_socket, buffer, strlen(buffer), 0) < 0)
        {
            printf("Can't send\n");
            exit(1);   
        }
        
    }
}

void handle_connection(int file_descriptor) {
    printf("FD %d\n", file_descriptor);
    int socket_desc, client_size;
    struct sockaddr_in server_addr, client_addr;
    char server_message[2000], client_message[2000];

    // Receive client's message:
    if (recv(file_descriptor, client_message, sizeof(client_message), 0) < 0)
    {
        printf("Couldn't receive\n");
        return;
    }

    char cwd[2048];
    strncpy(cwd, opt.path, 2048);

    printf("Trying to access more memory\n");
    char request[3][4096];

    char delim[] = " ";
    char *token = strtok(client_message, delim);

    int tokenPossition = 0;

    while (token != NULL)
    {
        printf("Trying to input to memmory\n");
        if (tokenPossition < 4)
        {
            strcpy(request[tokenPossition], token);
        }
        printf("%s\n", token);
        token = strtok(NULL, delim);
        tokenPossition++;
    }

    read_file(request[1], cwd, file_descriptor);

    printf("While loop finished\n");

    printf("Token first possition: %s\n", request[0]);
    printf("Token path: %s \n", request[1]);

    if (send(file_descriptor, server_message, strlen(server_message), 0) < 0)
    {
        printf("Can't send\n");
        return;
    }

    // Closing the socket:
    close(file_descriptor);
}


int main(int argc, char **argv)
{   

    /*TODO: Legge inn worker_init og struct */


    // __read_options returns 1 if it fails, 0 otherwise
    if (__read_options(&opt, argc, argv)) {
        return EXIT_FAILURE;
    }

    worker_control_t *worker = worker_init(opt.worker_threads, opt.buffer_slots, handle_connection);

    char cwd[2048];
    strncpy(cwd, opt.path, 2048);

    printf("Hello world!");
    int socket_desc, client_sock, client_size;
    struct sockaddr_in server_addr, client_addr;
    char server_message[2000], client_message[2000];

    // Clean buffers:
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_desc < 0)
    {
        printf("Error while creating socket\n");
        return -1;
    }

    printf("Socket created successfully\n");

    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(opt.port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Bind to the set port and IP:
    if (bind(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Couldn't bind to the port\n");
        return -1;
    }
    printf("Done with binding\n");

    // Listen for clients:
    if (listen(socket_desc, 1) < 0)
    {
        printf("Error while listening\n");
        return -1;
    }
    printf("\nListening for incoming connections.....\n");

    while (1)
    {
        // Accept an incoming connection:
        client_size = sizeof(client_addr);
        client_sock = accept(socket_desc, (struct sockaddr *)&client_addr, &client_size);
        if (client_sock < 0)
        {
            printf("Can't accept\n");
            return EXIT_FAILURE;
        }
        printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        /*TODO: Legge inn worker_submit */
        worker_submit(worker, client_sock);
    };
    close(socket_desc);

    return EXIT_SUCCESS;
}
