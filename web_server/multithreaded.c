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
#include <pthread.h>
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
    
    if(error != NULL) g_error_free(error);
    g_free(file_content);
    g_free(resource_path);
}

void* worker(void* arg)
{
    int cfd = *((int*) arg);
    ssize_t r;
    GString *request = g_string_new("");
    char buffer[BUFFER_SIZE];
    
    //Get the request from the web client
    //Loop until full message is read
    while (r > 0 && !(g_str_has_suffix(request->str, "\r\n\r\n")))
        {
            r = read(cfd, buffer, BUFFER_SIZE);
            if (r == -1)
            {
                err(EXIT_FAILURE, "could not read the request");
            }
            request = g_string_append_len(request, buffer, r);
        } 

    //Get resource from the request
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
         
        //Print resource and free full_request and resource
        printf("%d: %s\n", cfd, resource);

        //Prepare response to the client depending on the requested resource
        get_www_resource(cfd, resource);

        g_free(resource);

        //Close client sockets
        close(cfd);
    }

    g_string_free(request, TRUE);
    
    return NULL;
}

int main()
{
    struct addrinfo hints;
    struct addrinfo *addr_list, *addr;
    int sfd;
    int res;

    // Init with a value of 1
    // first 0 is for unused option, keep it this way
    if ( sem_init(&lock, 0, 1) == -1)
        err(1, "Fail to initialized semaphore");
    
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
        sfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        //If error, try next adress
        if (sfd == -1)
            continue;

        //Set options on socket
        int enable = 1;
        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1)
            perror("setsockopt(SO_REUSEADDR) failed");

        //Bind a name to a socket, exit if no error
        if (bind(sfd, addr->ai_addr, addr->ai_addrlen) == 0)
            break;

        //Close current not connected socket
        close(sfd);
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
    if(listen(sfd, 5) == -1)
    {
        fprintf(stderr, "Cannot wait\n");
        exit(0);
    }

    //Socket waiting for connections on port 2048
    printf("Tic-Tac-Toe Server\nListening to port 2048...\n");

    //Allow multiple connections
    while(1)
    {
        //Accept connection from a client and exit the program in case of error
        int cfd;
        cfd = accept(sfd, addr->ai_addr, &(addr->ai_addrlen));
        if(cfd == -1)
        {
            err(EXIT_FAILURE, "main: accept()");
        }

        int thread;
        pthread_t thread_id;

        // - Create and execute the thread.
        pthread_t thr;
        int e = pthread_create(&thr, NULL, worker, (void*)&cfd);
        if (e!=0)
        {
            err(EXIT_FAILURE, "pthread_create()");
        }
    }

    //Close server sockets
    close(sfd);
   
}
