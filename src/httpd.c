/* A s is the time taken from your computerhttp server
 *
 */

#include <assert.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>
#include <glib.h>

#define LOGFILE "http.log"

int main(int argc, char **argv)
{
    int sockfd, port;
    struct sockaddr_in server, client;
    char message[512];
    char date[20];
    time_t t;
    
    /* Read in command line arguments */
    if(argc < 2){
        perror("1 argument needed");
        exit(1);
    }
    port = (int) atoi(argv[1]);
    
    /* Open log file */
    FILE *flog = fopen(LOGFILE, "w");
    t = time(NULL);
    strftime(date, sizeof(date), "%FT%T", localtime(&t));
    fclose(flog);

    /* Create and bind a UDP socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;

    /* Network functions need arguments in network byte order instead of
       host byte order. The macros htonl, htons convert the values, */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server));

    /* Before we can accept messages, we have to listen to the port. We allow one
     * 1 connection to queue for simplicity.
     */
    listen(sockfd, 1);

    for (;;) {
        fd_set rfds;
        struct timeval tv;
        int retval;

        /* Check whether there is data on the socket fd. */
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);

        /* Wait for five seconds. */
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        retval = select(sockfd + 1, &rfds, NULL, NULL, &tv);

        if (retval == -1) {
            perror("select()");
        } else if (retval > 0) {
            /* Data is available, receive it. */
            assert(FD_ISSET(sockfd, &rfds));

            /* Copy to len, since recvfrom may change it. */
            socklen_t len = (socklen_t) sizeof(client);

            /* For TCP connectios, we first have to accept. */
            int connfd;
            connfd = accept(sockfd, (struct sockaddr *) &client,
                    &len);
            char *client_addr = inet_ntoa(client.sin_addr);
            int client_port = ntohs(client.sin_port);
            printf("%s : %s:%d <request method> <requested URL> : <response code>\n", date, client_addr, client_port);
 
            /* Receive one byte less than declared,
               because it will be zero-termianted
               below. */
            ssize_t n = read(connfd, message, sizeof(message) - 1);
            
            char start_line[30], host[30], user_agent[30], accept[30], accept_language[30], connection[30];
            sscanf(message, "%[^\n]\n %[^\n]\n %[^\n]\n %[^\n]\n %[^\n]\n %[^\n]\n", 
                   start_line, host, user_agent, accept, accept_language, connection);
            printf("start_line: %s\nhost: %s\nuser_agent: %s\naccept: %s\naccept_language: %sconnection: %s", start_line, host, user_agent, accept, accept_language, connection);
            /* Send the message back. */
            if(write(connfd, message, (size_t) n) < 0){
                perror("Write error");
                exit(1);
            }

            /* We should close the connection. */
            shutdown(connfd, SHUT_RDWR);
            close(connfd);

            /* Zero terminate the message, otherwise
               printf may access memory outside of the
               string. */
            message[n] = '\0';
            /* Print the message to stdout and flush. */
            fprintf(stdout, "Received:\n%s\n", message);
            fflush(stdout);
        } else {
            fprintf(stdout, "No message in five seconds.\n");
            fflush(stdout);
        }
    }
}
