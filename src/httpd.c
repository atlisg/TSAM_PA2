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
#define HTMLTEMPLATE "<!DOCTYPE html>\n<html>\n<head>\n\t<meta charset=\"utf-8\">\n\t<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\n\t<title>PA2</title>\n</head>\n<body>\n\t<p>Nerders With Skapgerders</p>\n</body>\n</html>\n"
 

/**
 *  Prints the header string
 *
 *  @param Char message[512]    Header string
 *  @param int n                Size of header
**/
void print_header(char message[512], int n){
    /* Zero terminate the message, otherwise
       printf may access memory outside of the
       string. */
    message[n] = '\0';
    /* Print the message */
    printf("%s\n", message);
    printf("----------------\n");
}


/* Iterate through the hash table and print out the values */
void print_ht(GHashTable *ht) {
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, ht);
    gpointer key, value;

    printf("--------------------------------\n");
    printf("Iterate hash table of size: %d\n", g_hash_table_size(ht));
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        printf("%s: %s\n", (char*) key, (char*) value);
    }
    printf("--------------------------------------\n");
}

/**
 *  Iterates through the hash table and returns a value if the key is found
 *  
 *  @param GHashTable   A glib 2.0 hash table, contains header key/values
 *  @param char         Hash table key being requested
**/
char* get_value(GHashTable *ht, char *keyToFind) {
    GHashTableIter iter;
    g_hash_table_iter_init(&iter, ht);
    gpointer key, value;
    
    GString *returnValue = g_string_new(NULL);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        if (strcmp(key, keyToFind) == 0) {
            returnValue = g_string_new(value);
            return returnValue->str;
        }
    }

    return NULL;
}


/**
 *  Builds the response header for the http
 *
 *  @param GHashTable   A glib 2.0 hash table, contains header key/values
 *  @param GString      A glib 2.0 string, a reference to the header string
**/
void build_response_hdr(GHashTable *ht, GString *res_hdr)
{
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, ht);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        g_string_append(res_hdr, key);
        g_string_append(res_hdr, ": ");
        g_string_append(res_hdr, value);
        g_string_append(res_hdr, "\r\n");
    }
    g_string_append(res_hdr, "\r\n\0");
}

/**
 *  Reads the data from a file descriptor, proccesses it, and sends appropriate response
 *  
 *  @param int  A file descriptor
**/
void read_from_client(int fds){
    char message[512];
    char date[20];
    time_t t;


    /* Receive one byte less than declared,
       because it will be zero-termianted
       below. */
    ssize_t n = read(fds, message, sizeof(message) - 1);
    message[n] = '\0'; 
    printf("Received:\n");
    print_header(message, n);

    /* Parse the Request Line */
    char line[150];
    int i, j;
    for (i = 0; message[i] != '\r'; i++) {
        line[i] = message[i];
    }
    line[i] = '\0';
    GString *method = g_string_new(NULL),
            *URL = g_string_new(NULL),
            *version = g_string_new(NULL);
    for (i = 0; line[i] != ' '; i++) {
        g_string_append_c(method, line[i]);
    }
    for (i++; line[i] != ' '; i++) {
        g_string_append_c(URL, line[i]);
    }
    for (i++; line[i] != '\0'; i++) {
        g_string_append_c(version, line[i]);
    }

    /* Loop through the received header lines and parse them into a hash table */
    GHashTable *ht = g_hash_table_new(NULL, NULL);
    /* Response header hashtable */
    GHashTable *hashSponse = g_hash_table_new(NULL, NULL);
    gboolean EOH = FALSE, keyRead = TRUE;
    int start;
    GString *key = g_string_new(NULL), *value = g_string_new(NULL);
    for(i += 2, start = i; !EOH; i++) {
        /* end of the line */
        if (message[i] == '\r') {
            /* end the header */
            if (message[i+2] == '\r') {
                EOH = TRUE;
            }
            value = g_string_new(NULL);
            for (j = 0; start < i; start++, j++) {
                g_string_append_c(value, message[start]);
            }
            i++;
            start = i + 1;
            keyRead = TRUE;
            g_hash_table_insert(ht, key->str, value->str);
        } else if (message[i] == ':' && keyRead) {
            keyRead = FALSE;
            key = g_string_new(NULL);
            for (j = 0; start < i; start++, j++) {
                g_string_append_c(key, message[start]);
            }
            i++;
            start = i + 1;
        }
    }

    /* Fetch content from message */
    GString *content = g_string_new(NULL);
    for (i += 2; i < sizeof(message); i++) {
        g_string_append_c(content, message[i]);
    }
    g_string_append_c(content, '\0');

    /* Creating the html file in memory */
    GString *html = g_string_new(HTMLTEMPLATE);
    int insert_point = 176;
    GString *lineToAdd = g_string_new(NULL);
    if (g_strcmp0(method->str, "POST") == 0) {
        g_string_append(lineToAdd, "\n\t<p>");
        g_string_append(lineToAdd, content->str);
        g_string_append(lineToAdd, "</p>\n");
    } 

    /* split URL into args */
    GString *uri = g_string_new(NULL);
    GString *query = g_string_new(NULL);
    gboolean isQ = FALSE;
    int argcnt = 1;
    for (i = 1; i < URL->len; i++) {
        if (URL->str[i] == '?') {
            isQ = TRUE;
        } else if (isQ) {
            g_string_append_c(query, URL->str[i]);
            if (URL->str[i] == '&') argcnt++;
        } else {
            g_string_append_c(uri, URL->str[i]);
        }
    }
    //printf("uri: %s\n", uri->str);
    if (g_strcmp0(uri->str, "test") == 0) {
        gchar **arse = g_strsplit(query->str, "&", 0);
        printf("query: %s\n", query->str);
        g_string_append(lineToAdd, "\n\t<p>\n\t");
        for (i = 0; i < argcnt; i++) {
            printf("arse[%d]: %s\n", i, arse[i]);
            g_string_append(lineToAdd, arse[i]);
            g_string_append(lineToAdd, "<br>\n\t");
        }
        g_string_append(lineToAdd, "</p>\n");
    } else if (g_strcmp0(uri->str, "color") == 0) {
        insert_point = 142;
        gchar **bg;
        char* color;
        if (query->str[0] == 'b' && query->str[1] == 'g') {
            bg = g_strsplit(query->str, "=", 0);
            GString *cookie = g_string_new("color=");
            g_string_append(cookie, bg[1]);
            color = bg[1];
            g_hash_table_insert(hashSponse, "Set-cookie", cookie->str);
        } else {
            char *cookieValue = get_value(ht, "Cookie");
            if (cookieValue != NULL) {
                printf("Found %s!\n", cookieValue);
                bg = g_strsplit(g_string_new(cookieValue)->str, "=", 0);
                color = bg[1];
            } else {
                color = "white";
            }
        }
        g_string_append(lineToAdd, " style=\"background-color: ");
        g_string_append(lineToAdd, color);
        g_string_append(lineToAdd, "\"");

    }
    g_string_insert(html, insert_point, lineToAdd->str);

    t = time(NULL);
    strftime(date, sizeof(date), "%FT%T\n", localtime(&t));

    /* Create a new hash table with data to put into response header */
    g_hash_table_insert(hashSponse, "Date", date);
    g_hash_table_insert(hashSponse, "Content-Type", "text/html; charset=iso-8859-1");
    g_hash_table_insert(hashSponse, "Server", "Nilli/0.2000");
    char cl[10];
    sprintf(cl, "%d", (int)html->len);
    g_hash_table_insert(hashSponse, "Content-Length", cl);
    //print_ht(hashSponse);


    /* Make the response header */
    GString *res = g_string_new(version->str);
    g_string_append(res, " 200 OK\r\n");
    build_response_hdr(hashSponse, res);
    if (g_strcmp0(method->str, "HEAD") != 0) {
        printf("Sending:\n");
        print_header(res->str, res->len);
        g_string_append(res, html->str);
    }
    
    /* Send the header */
    if(write(fds, res->str, res->len) < 0) {
        perror("Write error");
        exit(1);
    }

    /* We should close the connection if requested. */
    char *connec = get_value(ht, "Connection");
    //printf("connec: %s\n", connec);
    if (connec != NULL && 
            (strcmp(connec, "close") == 0 || 
            strcmp(connec, "keep-alive") != 0)) { 
        shutdown(fds, SHUT_RDWR);
        //printf("You have been terminated by the terminator\n");
    }

    /* Open log file */
    FILE *flog = fopen(LOGFILE, "a");
    fprintf(flog, 
            "%s : %s:%d %s %s : 200 OK\n",
            date, "---.---.---.---", 1234, method->str, URL->str);
    fclose(flog);

}

int main(int argc, char **argv)
{
    int sockfd, port, i;
    struct sockaddr_in server, client;
    fd_set rfds, afds;
    gboolean open_socket = FALSE;
    
    /* Read in command line arguments */
    if(argc < 2){
        perror("1 argument needed");
        exit(1);
    }
    port = (int) atoi(argv[1]);
    
    /* Create and bind a socket */
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
    
    /* Check whether there is data on the socket fd. */
    FD_ZERO(&afds);
    FD_SET(sockfd, &afds);
    
    for (;;) {
        struct timeval tv;
        int retval;
        int connfd;
        
        tv.tv_sec = 30; // Wait for 30 seconds
        tv.tv_usec = 0;
        
        rfds = afds;
        retval = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);

        if (retval == -1) {
            perror("select()");
        } else if (retval > 0) {
            /* Loop through sockets with pending data, and service */
            for(i = 0; i < FD_SETSIZE; ++i){
                if(FD_ISSET(i, &rfds)){
                    /* Connecting on original socket */
                    if(i == sockfd){
                        /* Data is available, receive it. */
                        assert(FD_ISSET(sockfd, &rfds));

                        /* Copy to len, since recvfrom may change it. */
                        socklen_t len = (socklen_t) sizeof(client);

                        /* For TCP connectios, we first have to accept. */
                        connfd = accept(sockfd, (struct sockaddr *) &client, &len);
                        open_socket = TRUE;

                        /* put client address and port nr intp variables 
                           TODO: WE NEED TO FIX THIS, WE LOSE THE DATA, NEEDED IN LOG FILE */
                        char *client_addr = inet_ntoa(client.sin_addr);
                        int client_port = ntohs(client.sin_port);

                        /* Add the file descriptor to the active set */
                        FD_SET(connfd, &afds);
                    }
                    /* Socket is already connected */
                    else{
                        read_from_client(i);

                        /* Terminate the socket */
                        close(i);

                        /* Remove the file descriptor from the active set */
                        FD_CLR(i, &afds);
                    }
                }
            }
        } else {
            if (open_socket) {
                shutdown(connfd, SHUT_RDWR);
                close(connfd);
                open_socket = FALSE;
                //printf("Termination of the fraction erection\n");
            }
            printf("No message in 30 seconds.\n");
        }
    }
}
