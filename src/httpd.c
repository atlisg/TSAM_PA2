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

#define LOGFILE "httpd.log"

/* Iterate through the hash table and print out the values */
void print_ht(GHashTable *ht) {
    printf("Going to iterate through the table of size %d\n--------------------------------------\n", g_hash_table_size(ht));
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, ht);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        printf("Current Key: %s\nCurrent Value: %s\n\n", key, value);
    }
    printf("--------------------------------------\nIteration finished.\n\n");
}

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

            /* put client address and port nr intp variables */
            char *client_addr = inet_ntoa(client.sin_addr);
            int client_port = ntohs(client.sin_port);

            /* Receive one byte less than declared,
               because it will be zero-termianted
               below. */
            ssize_t n = read(connfd, message, sizeof(message) - 1);
            
            /* Parse the Request Line */
            char line[50];
            int i, j;
            for (i = 0; message[i] != '\r'; i++) {
                line[i] = message[i];
            }
            char method[30], URL[30], version[30];
            sscanf(line, "%s %s %s", method, URL, version);

            /* Loop through the received header lines and parse them into a hash table */
            GHashTable *ht = g_hash_table_new(NULL, NULL);
            gboolean EOH = FALSE, keyRead = TRUE;
            char *key = NULL, *value = NULL;
            int start;
            for(i += 2, start = i; !EOH; i++) {
                /* end of the line */
                if (message[i] == '\r') {
                    /* end the header */
                    if (message[i+2] == '\r') {
                        EOH = TRUE;
                    }
                    value = malloc(i - start + 1);
                    for (j = 0; start < i; start++, j++) {
                        value[j] = message[start];
                    }
                    value[j] = '\0';
                    i++;
                    start = i + 1;
                    keyRead = TRUE;
                    g_hash_table_insert(ht, (gpointer) key, (gpointer) value);
                } else if (message[i] == ':' && keyRead) {
                    keyRead = FALSE;
                    key = malloc(i - start + 1);
                    for (j = 0; start < i; start++, j++) {
                        key[j] = message[start];
                    }
                    key[j] = '\0';
                    i++;
                    start = i + 1;
                }
            }

            //print_ht(ht);

            /* Send the message back. */
            if(write(connfd, message, (size_t) n) < 0){
                perror("Write error");
                exit(1);
            }

            /* We should close the connection. */
            shutdown(connfd, SHUT_RDWR);
            close(connfd);

            /* Open log file */
            FILE *flog = fopen(LOGFILE, "a");
            t = time(NULL);
            strftime(date, sizeof(date), "%FT%T\n", localtime(&t));
            fprintf(flog, "%s : %s:%d %s %s : <response code>\n",
                date, client_addr, client_port, method, URL);
            fclose(flog);

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
