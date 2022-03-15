#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
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

void read_file(char *filename, char* cwd, int client_socket) {
    FILE *fptr;
    
    char abs_path[512] = {0}; 
    strcpy(abs_path, cwd);
    strlcat(abs_path, filename, 4512);

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


int main(void)
{
    char *cwd;
    char tmp_cwd[4000];
    if (getcwd(tmp_cwd, sizeof(tmp_cwd)) != NULL)
    {
        printf("Det funker!\n");
        cwd = strcat(tmp_cwd, "/src/www");
    }
    else
        return 1;

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
    server_addr.sin_port = htons(6969);
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
            return -1;
        }
        printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Receive client's message:
        if (recv(client_sock, client_message, sizeof(client_message), 0) < 0)
        {
            printf("Couldn't receive\n");
            return -1;
        }

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

        read_file(request[1], cwd, client_sock);

        printf("While loop finished\n");

        printf("Token first possition: %s\n", request[0]);
        printf("Token path: %s \n", request[1]);

        // Respond to client:
        strcpy(server_message, "This is the server's message.");

        if (send(client_sock, server_message, strlen(server_message), 0) < 0)
        {
            printf("Can't send\n");
            return -1;
        }

        // Closing the socket:
        close(client_sock);
    };
    close(socket_desc);

    return 0;
}
