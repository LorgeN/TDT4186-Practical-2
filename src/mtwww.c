#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "worker.h"

#define DELIMITER " "
#define FILE_NOT_FOUND_DEFAULT "<p>File not found</p>\n"

struct mtwww_options_t {
    char *path;
    int port;
    int worker_threads;
    int buffer_slots;
};

// Keep here so that it is accessible to everything
struct mtwww_options_t opt;
volatile sig_atomic_t active;

void __shutdown_sig_handler(int sig) {
    active = 0;
}

int __file_isdir(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;  // Not a directory
    }

    return S_ISDIR(st.st_mode);
}

int __file_isreg(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;  // Not a directory
    }

    return S_ISREG(st.st_mode);
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

int __read_and_send_file(char *filename, char *cwd, int client_socket) {
    FILE *fptr;

    char abs_path[512] = {0};
    strcpy(abs_path, cwd);
    strncat(abs_path, filename, 512 - strlen(abs_path));

    // Handle case where file doesn't exist
    if (!__file_isreg(abs_path)) {
        // Allow user to specify a 404 page
        memset(abs_path, 0, 512);

        strcpy(abs_path, cwd);
        strncat(abs_path, "/404.html", 512 - strlen(abs_path));

        if (!__file_isreg(abs_path)) {
            if (send(client_socket, FILE_NOT_FOUND_DEFAULT, strlen(FILE_NOT_FOUND_DEFAULT), 0) < 0) {
                return 1;
            }

            return 0;
        }
    }

    if ((fptr = fopen(abs_path, "rb")) == NULL) {
        return 1;
    }

    fseek(fptr, 0, SEEK_END);
    long size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);

    char buffer[1000] = {0};
    while (fgets(buffer, sizeof(buffer), fptr) != NULL) {
        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            return 1;
        }
    }

    return 0;
}

void handle_connection(int file_descriptor) {
    struct sockaddr_in server_addr, client_addr;

    char client_message[2000] = {0};

    // Receive client's message:
    if (recv(file_descriptor, client_message, sizeof(client_message), 0) < 0) {
        printf("Couldn't receive request\n");
        return;
    }

    char cwd[2048];
    strncpy(cwd, opt.path, 2048);

    char request[3][256];
    char *token = strtok(client_message, DELIMITER);

    int tokenPosition = 0;
    while (token != NULL) {
        if (tokenPosition < 4) {
            // Avoid overflowing if the request is massive
            strncpy(request[tokenPosition], token, 256);
        }

        token = strtok(NULL, DELIMITER);
        tokenPosition++;
    }

    __read_and_send_file(request[1], cwd, file_descriptor);

    // Closing the socket:
    close(file_descriptor);
}

int main(int argc, char **argv) {
    // __read_options returns 1 if it fails, 0 otherwise
    if (__read_options(&opt, argc, argv)) {
        return EXIT_FAILURE;
    }

    printf("Loading webserver...\n");

    worker_control_t *worker;
    int socket_desc;

    // Make sure we clean up after ourselves. Without this,
    // the socket may be left bound to the given port even after
    // the process exits
    struct sigaction shutdown_signal_handler;
    shutdown_signal_handler.sa_handler = __shutdown_sig_handler;
    // Omitting this flag (which by default is on in some cases)
    // makes it so that #accept() is interrupted by SIGINT, which
    // is desirably for us
    shutdown_signal_handler.sa_flags &= ~SA_RESTART;
    sigaction(SIGINT, &shutdown_signal_handler, NULL);

    int client_sock, client_size;
    struct sockaddr_in6 server_addr, client_addr;

    client_size = sizeof(client_addr);
    char client_addr_str[INET6_ADDRSTRLEN];

    // Create socket
    socket_desc = socket(AF_INET6, SOCK_STREAM, 0);

    if (socket_desc < 0) {
        printf("Error while creating socket\n");
        return EXIT_FAILURE;
    }

    int ipv6_only_val = 0;
    // Make sure IPV6_V6ONLY is disabled
    if (setsockopt(socket_desc, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6_only_val, sizeof(ipv6_only_val)) < 0) {
        printf("Failed to disable IPV6_ONLY option\n");
        close(socket_desc);
        return EXIT_FAILURE;
    }

    printf("Socket created successfully\n");

    // Set server port and IP
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(opt.port);
    server_addr.sin6_addr = in6addr_any;

    // Bind to the set port and IP:
    if (bind(socket_desc, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Couldn't bind to the port\n");
        close(socket_desc);
        return EXIT_FAILURE;
    }

    printf("Done with binding\n");

    // Listen for clients:
    if (listen(socket_desc, 1) < 0) {
        printf("Error while listening\n");
        close(socket_desc);
        return EXIT_FAILURE;
    }

    printf("\nCreating worker thread pool...\n");
    worker = worker_init(opt.worker_threads, opt.buffer_slots, handle_connection);
    if (worker == NULL) {
        printf("Failed to create worker thread pool!\n");
        close(socket_desc);
        return EXIT_FAILURE;
    }

    printf("Done! Listening for incoming connections on port %d...\n", opt.port);

    active = 1;
    while (active) {
        // Accept an incoming connection
        client_sock = accept(socket_desc, (struct sockaddr *)&client_addr, &client_size);
        if (client_sock < 0) {
            continue;
        }

        if (inet_ntop(AF_INET6, &client_addr.sin6_addr, client_addr_str, sizeof(client_addr_str))) {
            printf("Client connected at IP: %s and port: %i\n", client_addr_str, ntohs(client_addr.sin6_port));
        }

        worker_submit(worker, client_sock);
    }

    printf("Shutting down workers...\n");
    worker_destroy(worker);
    worker = NULL;

    printf("Closing server socket...\n");
    close(socket_desc);
    socket_desc = 0;

    return EXIT_SUCCESS;
}
