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

#define BUFFER_SIZE 500

// Get resource
void get_www_resource(int client_socket_id, gchar* resource)
{
    gchar* file_content;
    gsize response_size;
    GError* error = NULL;
    
    gchar* resource_path = malloc((strlen(resource) + 4) * sizeof(char));
    sprintf(resource_path, "www/%s", resource);

    if(g_file_get_contents(resource_path, &file_content, &response_size, &error) == FALSE)
    {
        char message[] = "HTTP/1.1 404 Not Found\r\n\r\n404 Not Found";
        write(client_socket_id, message, strlen(message));
    }
    else
    {
        //Send message status
        char message[] = "HTTP/1.1 200 OK\r\n\r\n";
        send(client_socket_id, message, strlen(message), MSG_MORE);
        
        //Send message content
        if (write(client_socket_id, file_content, response_size) != ((int) response_size))
        {
            fprintf(stderr, "partial/failed write\n");
        }
    }
    
    if(error != NULL) g_error_free(error);
    g_free(file_content);
    g_free(resource_path);
}

// Define the thread function.
void* worker(void* arg)
{
    int client_socket_id = *((int*) arg);
    ssize_t request_size;
    char request[BUFFER_SIZE];
    
    //Get the request from the web client
    //Loop until full message is read
    GString *full_request = g_string_new("");
    do
    {
        request_size = read(client_socket_id, request, BUFFER_SIZE-1);
        if (request_size == -1)
        {
            perror("read");
            exit(0);
        }
        request[request_size] = '\0';
        full_request = g_string_append(full_request, request);
    } while (request_size > 0 &&
             g_str_has_suffix(full_request->str, "\r\n\r\n") == FALSE);

    //Get resource from the request
    if (g_str_has_prefix(full_request->str, "GET ") == TRUE)
    {
        gchar* resource = g_strndup(full_request->str+5, g_strstr_len(full_request->str, -1, " HTTP/")-full_request->str-5);

        if(strcmp(resource, "slow.html") == 0)
            sleep(10);
            
        if(strcmp(resource, "") == 0)
        {
            resource = realloc(resource, 10 * sizeof(gchar));
            g_stpcpy(resource, "index.html");
        }
         
        //Print resource and free full_request and resource
        printf("%d: %s\n", client_socket_id, resource);

        //Prepare response to the client depending on the requested resource
        get_www_resource(client_socket_id, resource);

        g_free(resource);

        //Close client sockets
        close(client_socket_id);
    }

    g_string_free(full_request, TRUE);
    
    return NULL;
}

int main()
{
    struct addrinfo hints;
    struct addrinfo *addr_list, *addr;
    int socket_id, client_socket_id;
    int res;

    //Get addresses list
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    res = getaddrinfo(NULL, "2048", &hints, &addr_list);

    //If error, exit the program
    if (res != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
        exit(0);
    }


    //Try to connect to each adress returned by getaddrinfo()
    for (addr = addr_list; addr != NULL; addr = addr->ai_next)
    {
        //Socket creation
        socket_id = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        //If error, try next adress
        if (socket_id == -1)
            continue;

        //Set options on socket
        int enable = 1;
        if (setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
            perror("setsockopt(SO_REUSEADDR) failed");

        //Bind a name to a socket, exit if no error
        if (bind(socket_id, addr->ai_addr, addr->ai_addrlen) == 0)
            break;

        //Close current not connected socket
        close(socket_id);
    }

    //addr_list freed
    freeaddrinfo(addr_list);

    //If no address works, exit the program
    if (addr == NULL)
    {
        fprintf(stderr, "Could not bind\n");
        exit(0);
    }

    //Specify that the socket can be used to accept incoming connections
    if(listen(socket_id, 5) == -1)
    {
        fprintf(stderr, "Cannot wait\n");
        exit(0);
    }

    //Socket waiting for connections on port 2048
    printf("Static Server\nListening to port 2048...\n");

    //Allow multiple connections
    while(1)
    {
        //Accept connection from a client and exit the program in case of error
        client_socket_id = accept(socket_id, addr->ai_addr, &(addr->ai_addrlen));
        if(client_socket_id == -1)
        {
            fprintf(stderr, "Cannot connect\n");
            exit(0);
        }

        int thread;
        pthread_t thread_id;

        // - Create and execute the thread.
        thread = pthread_create(&thread_id, NULL, &(worker), (void*)&client_socket_id);
        if(thread != 0)
        {
            fprintf(stderr, "hello: Can't create thread.\n");
            exit(0);
        }
    }

    //Close server sockets
    close(socket_id);
}
