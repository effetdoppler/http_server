#define _GNU_SOURCE

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <unistd.h>
#include <glib.h>

#define BUFFER_SIZE 512
void rewrite(int fd, const void *buf, size_t count)
{
    ssize_t res = write(fd, buf, count);
    //If the return value of write() is smaller than its third argument, you must call write() a
    //gain in order to have the rest of the data written. Repeat this until all the data has been sent
    if (res != (ssize_t)count)
    {
        const char * buff  = buf;
        while(res != (ssize_t)count)
        {
            // If an error occurs, exit the program with an error message
            if (res == -1)
                err(EXIT_FAILURE, "write function has failed");
            res = write(fd, buf, count);
            buff = buff + res;
            count = count - res;
            res = write(fd, buff, count);
        }
    }
}

int main()
{
    char buffer[BUFFER_SIZE];
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *result;
    if (getaddrinfo(NULL, "2048", &hints, &result) != 0)
        err(EXIT_FAILURE, "server_connection: getaddrinfo()");
    struct addrinfo *rp;
    int sfd;
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        //If an error occurs, continue with the next address.
        if (sfd == -1)
            continue;
        int value = 1;
        //set SO_REUSEADDR to 1
        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int))==-1)
            err(EXIT_FAILURE, "server_connection: setsocketopt()");
        // Try to bind the socket to the address
        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;
        close(sfd);
    }
    //Free the linked list.
    freeaddrinfo(result);
    printf("Static Server\nListening to port 2048...\n");
    if (rp == NULL)              /* No address succeeded */
        errx(EXIT_FAILURE, "Could not connect\n");
    if (listen(sfd, 5) == -1)
        err(EXIT_FAILURE, "main: listen()");

    //Print a message saying that your server is waiting for connections.
    while(1)
    {
        int cfd;
        struct sockaddr client_address;
        socklen_t client_address_length = sizeof(struct sockaddr);
        //Wait for connections by using the accept(2) function
        cfd = accept(sfd, &client_address, &client_address_length);
        if (cfd == -1)
            err(EXIT_FAILURE, "main: accept()");
        GString *request = g_string_new("");
        ssize_t r = 1;
        while (r > 0)
        {
            r = read(cfd, buffer, BUFFER_SIZE);
            if (r == -1)
            {
                err(EXIT_FAILURE, "could not read the request");
            }
            request = g_string_append_len(request, buffer, r);
        } 
        //if empty request or invalid request
        if(!(g_str_has_suffix(request->str, "\r\n\r\n")))
        {
            close(cfd);
            continue;
        }
        //Print any message showing that a connection is successful.
        //Print a message to the client
        printf("%s", request->str);
        char response[] = "HTTP/1.1 200 OK\r\n\r\nHello World!";
        g_string_free(request, TRUE);
        rewrite(cfd, response, strlen(response));
        close(cfd);
        
    }
    //Close sfd
    close(sfd);
    return 0;
   
}
