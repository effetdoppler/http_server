#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <err.h>
#include <gmodule.h>
#include <glib.h>
#include <glib/gprintf.h>

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
void get_www_resource(int cfd, gchar* resource)
{
    gchar* file_content;
    gsize response_size;
    GError* error = NULL;
    
    gchar* resource_path = malloc((strlen(resource) + 4) * sizeof(char));
    sprintf(resource_path, "www/%s", resource);

    if(g_file_get_contents(resource_path, &file_content, &response_size, &error) == FALSE)
    {
        char message[] = "HTTP/1.1 404 Not Found\r\n\r\n404 Not Found";
        write(cfd, message, strlen(message));
    }
    else
    {
        //Send message status
        char message[] = "HTTP/1.1 200 OK\r\n\r\n";
        send(cfd, message, strlen(message), MSG_MORE);
        
        //Send message content
        rewrite(cfd, file_content, response_size);
    }
    
    if(error != NULL) 
        g_error_free(error);
    g_free(file_content);
    g_free(resource_path);
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
    
    if (rp == NULL)              /* No address succeeded */
        errx(EXIT_FAILURE, "Could not connect\n");
    if (listen(sfd, 5) == -1)
        err(EXIT_FAILURE, "main: listen()");

    //Print a message saying that your server is waiting for connections.
    printf("Static Server\nListening to port 2048...\n");
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
        if (g_str_has_prefix(request->str, "GET ") == TRUE)
        {
            gchar* resource = g_strndup(request->str+5, g_strstr_len(request->str, -1, " HTTP/")-request->str-5);
            if(strcmp(resource, "slow.html") == 0)
                sleep(10);

            if(strcmp(resource, "") == 0)
            {
                resource = realloc(resource, 10 * sizeof(gchar));
                g_stpcpy(resource, "index.html");
            }

            printf("%d: %s\n", cfd, resource);
            
            get_www_resource(cfd, resource);
            g_free(resource);
            close(cfd);
        }
        
        
    }
    //Close sfd
    close(sfd);
    return 0;
   
}
