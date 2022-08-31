#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>

/* ----------------------------------- Assignment description -----------------------------------
The goal of this assignment is to gain experience with sockets and network programming.
We will implement a toy client/server architecture: a printable characters counting (PCC) server.
# Clients connect to the server and send it a stream of bytes.
# The server counts how many of the bytes are printable and returns that number to the client.
  The server also maintains overall statistics on the number of printable characters it has received
  from all clients. When the server terminates, it prints these statistics to standard output.

----- This program is the Server (pcc_server) -----
# The server accepts TCP connections from clients.
# A client that connects sends the server a stream of N bytes (N is determined by the client and
  is not a global constant).
# The server counts the number of printable characters in the stream (a printable character is a
  byte b whose value is 32 ≤ b ≤ 126).
# Once the stream ends, the server sends the count back to the client over the same connection.
# In addition, the server maintains a data structure in which it counts the number of times each
  printable character was observed in all the connections.
# When the server receives a SIGINT, it prints these counts and exits.
----------------------------------------------------------------------------------------------- */

uint32_t pcc_total[95] = {0};
int sigintFlag = 0;
int connfd = -1;

void closeServer()
{
    for (int i = 0; i < 95; i++)
    { /* for every printable character, print the number of times it was observed */
        printf("char '%c' : %u times\n", (i + 32), pcc_total[i]);
    }
    exit(0);
}

void sigintHandler()
{
    if (connfd < 0)
    { /* in case its not connected */
        closeServer();
    }
    else
    {
        sigintFlag = 1;
    }
}

int main(int argc, char *argv[])
{
    int listenfd = -1;
    int option_value = 1; /* for setsockopt function */
    int bytes;
    int bytesCnt;
    char *intBuffer;
    char *rcv_buff;
    uint32_t N, net_int, C;
    uint32_t pcc_buff[95] = {0};
    struct sockaddr_in serv_addr;

    if (argc != 2)
    { /* validate amount of arguments is correct */
        fprintf(stderr, "Invalid amount of arguments. Error: %s\n", strerror(EINVAL));
        exit(1);
    }

    /* initialize SIGINT handler */
    struct sigaction sigint;
    sigint.sa_handler = &sigintHandler;
    sigemptyset(&sigint.sa_mask);
    sigint.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sigint, 0) != 0)
    {
        fprintf(stderr, "An error occured when tried to register Signal handle. Error: %s\n", strerror(errno));
        exit(1);
    }

    /* create socket & set address to be reuseable.
    SO_REUSEADDR socket option allows a socket to forcibly bind to a port in use by another socket. */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "An error occured when tried to create socket. Error: %s\n", strerror(errno));
        exit(1);
    }
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(int)) < 0)
    {
        fprintf(stderr, "setsockopt Error: %s\n", strerror(errno));
        exit(1);
    }

	/* setting server properties */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* INADDR_ANY - allows the server to receive packets that have been targeted by any of the interfaces. */
    serv_addr.sin_port = htons(atoi(argv[1])); /* set port */

    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) != 0)
    {
        fprintf(stderr, "An error occured when tried to bind. Error: %s \n", strerror(errno));
        exit(1);
    }

    if (listen(listenfd, 10) != 0)
    {
        fprintf(stderr, "Error; listen failed. %s \n", strerror(errno));
        exit(1);
    }

    /* while-loop to accept TCP connections */
    while (1)
    {
        if (sigintFlag)
        { /* in case of SIGINT mid processing (or slip of SIGINT before 'connfd = -1') */
            closeServer();
        }

        /* accept connection */
        connfd = accept(listenfd, NULL, NULL);
        if (connfd < 0)
        {
            fprintf(stderr, "Error; Acception Failed. %s\n", strerror(errno));
            exit(1);
        }

        /* read N from client */
        intBuffer = (char*)&net_int;
        bytes = 1;
        bytesCnt = 0;
        while (bytes > 0)
        {
            bytes = read(connfd, intBuffer + bytesCnt, 4 - bytesCnt);
            bytesCnt += bytes;
        }
        if (bytes < 0)
        {
            if (!(errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE))
            {
                fprintf(stderr, "An error occured when tried to read N. Error: %s\n", strerror(errno));
                exit(1);
            }
            else
            {
                fprintf(stderr, "TCP Error when tried to read N: %s\n", strerror(errno));
                close(connfd);
                connfd = -1;
                continue;
            }
        }
        else
        { /* in case bytes == 0 */
            if (bytesCnt != 4)
            { /* indication that client process was killed unexpectedly */
                fprintf(stderr, "Client process was killed unexpectedly while reading N. "
                                "Server will keep accepting new clients connections. Error: %s\n",
                                strerror(errno));
                close(connfd);
                connfd = -1;
                continue;
            }
        }

        /* converting C & allocating memory for data */
        N = ntohl(net_int); /* ntohl translates a long integer from network byte order to host byte order */
        rcv_buff = malloc(N);

        /* read file data from client - N bytes */
        bytesCnt = 0;
        bytes = 1;
        while (bytes > 0)
        {
            bytes = read(connfd, rcv_buff + bytesCnt, N - bytesCnt);
            bytesCnt += bytes;
        }
        if (bytes < 0)
        {
            if (!(errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE))
            {
                fprintf(stderr, "An error occured when tried to read data. Error: %s\n", strerror(errno));
                exit(1);
            }
            else
            {
                fprintf(stderr, "TCP Error when tried to read data: %s\n", strerror(errno));
                free(rcv_buff);
                close(connfd);
                connfd = -1;
                continue;
            }
        }
        else
        { /* in case bytes == 0 */
            if (bytesCnt != N)
            { /* indication that client process was killed unexpectedly */
                fprintf(stderr, "Client process was killed unexpectedly while reading data. "
                                "Server will keep accepting new clients connections. Error: %s\n",
                                strerror(errno));
                free(rcv_buff);
                close(connfd);
                connfd = -1;
                continue;
            }
        }

        /* calculating C & counting readable chars in pcc_buff */
        C = 0;
        for (int i = 0; i < 95; i++)
        {
            pcc_buff[i] = 0;
        }
        for (int i = 0; i < N; i++)
        {
            if (32 <= rcv_buff[i] && rcv_buff[i] <= 126)
            {
                C++;
                pcc_buff[(int)(rcv_buff[i] - 32)]++;
            }
        }
        free(rcv_buff);

        /* sending C to the client - 4 bytes */
        net_int = htonl(C); /* htonl takes 32-bit number in host byte order, returns 32-bit number in the network byte order */
        bytesCnt = 0;
        bytes = 1;
        while (bytes > 0)
        {
            bytes = write(connfd, intBuffer + bytesCnt, 4 - bytesCnt);
            bytesCnt += bytes;
        }
        if (bytes < 0)
        {
            if (!(errno == ETIMEDOUT || errno == ECONNRESET || errno == EPIPE))
            {
                fprintf(stderr, "An error occured when tried to write C. Error: %s\n", strerror(errno));
                exit(1);
            }
            else
            {
                fprintf(stderr, "TCP Error when tried to write C: %s\n", strerror(errno));
                close(connfd);
                connfd = -1;
                continue;
            }
        }
        else
        { /* in case bytes == 0 */
            if (bytesCnt != 4)
            { /* indication that client process was killed unexpectedly */
                fprintf(stderr, "Client process was killed unexpectedly while writing C. "
                                "Server will keep accepting new clients connections. Error: %s\n",
                                strerror(errno));
                close(connfd);
                connfd = -1;
                continue;
            }
        }

        /* updating pcc_total */
        for (int i = 0; i < 95; i++)
        {
            pcc_total[i] += pcc_buff[i];
        }
        close(connfd);
        connfd = -1;
    }
}
