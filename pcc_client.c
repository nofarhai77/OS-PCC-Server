#define _DEFAULT_SOURCE /* to ensure that most features are included */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>

/* ----------------------------------- Assignment description -----------------------------------
The goal of this assignment is to gain experience with sockets and network programming.
We will implement a toy client/server architecture: a printable characters counting (PCC) server.
# Clients connect to the server and send it a stream of bytes.
# The server counts how many of the bytes are printable and returns that number to the client.
  The server also maintains overall statistics on the number of printable characters it has received
  from all clients. When the server terminates, it prints these statistics to standard output.

----- This program is the Client (pcc_client) -----
# The client creates a TCP connection to the server and sends it the contents of a user-supplied file.
# The client then reads back the count of printable characters from the server, prints it, and exits.
----------------------------------------------------------------------------------------------- */

int main(int argc, char *argv[])
{
	char *sendBuffer;
	char *intBuffer;
	int bytes;
	int bytesCnt;
	int sockfd = -1; /* socket descriptor */
	FILE *fileToSend;
	uint32_t N, net_int, C;
	struct sockaddr_in serv_addr; /* where we want to get */

	if (argc != 4)
	{
		fprintf(stderr, "Invalid amount of arguments. Error: %s\n", strerror(EINVAL));
		exit(1);
	}

	fileToSend = fopen(argv[3], "rb");
	if (fileToSend == NULL)
	{
		fprintf(stderr, "An error occured when tried to open input file: %s\n Error: %s\n", argv[3], strerror(errno));
		exit(1);
	}

	/* file size in bytes */
	fseek(fileToSend, 0, SEEK_END);
	N = ftell(fileToSend);

	/* save file data as array of chars */
	fseek(fileToSend, 0, SEEK_SET);
	sendBuffer = malloc(N); /* allocating N bytes for buffer */
	if (sendBuffer == NULL)
	{
		fprintf(stderr, "An error occured when tried to allocate N=%u bytes for the buffer: %s\n", N, strerror(errno));
		exit(1);
	}
	if (fread(sendBuffer, 1, N, fileToSend) != N)
	{
		fprintf(stderr, "An error occured when tried to read from file: %s\n", strerror(errno));
		exit(1);
	}
	fclose(fileToSend);

	/* tries to create socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		fprintf(stderr, "An error occured when tried to create socket. Error: %s\n", strerror(errno));
		exit(1);
	}

	/* setting server properties */
	memset(&serv_addr, 0, sizeof(serv_addr));
	inet_pton(AF_INET, argv[1], &(serv_addr.sin_addr)); /* convert an IP string to binary representation */
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2])); /* sets port from args (htons for endiannes) */

	/* tries to connect socket to the target address*/
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		fprintf(stderr, "An error occured when tried to connect socket. Error: %s\n", strerror(errno));
		exit(1);
	}

	/* writing file size to the server */
	net_int = (htonl(N)); /* htonl takes 32-bit number in host byte order, returns 32-bit number in the network byte order */
	intBuffer = (char *)&net_int;
	bytesCnt = 0;
	while (bytesCnt < 4)
	{
		bytes = write(sockfd, intBuffer + bytesCnt, 4 - bytesCnt);
		if (bytes < 0)
		{
			fprintf(stderr, "An error occured when tried to write N to the server. Error: %s\n", strerror(errno));
			exit(1);
		}
		bytesCnt += bytes;
	}

	/* writing file data to the server - N bytes */
	bytesCnt = 0;
	while (bytesCnt < N)
	{
		bytes = write(sockfd, sendBuffer + bytesCnt, N - bytesCnt);
		if (bytes < 0)
		{
			fprintf(stderr, "An error occured when tried to write data to the server. Error: %s\n", strerror(errno));
			exit(1);
		}
		bytesCnt += bytes;
	}
	free(sendBuffer);

	/* reading C from the server - 4 bytes */
	bytesCnt = 0;
	while (bytesCnt < 4)
	{
		bytes = read(sockfd, intBuffer + bytesCnt, 4 - bytesCnt);
		if (bytes < 0)
		{
			fprintf(stderr, "An error occured when tried to read C from server. Error: %s\n", strerror(errno));
			exit(1);
		}
		bytesCnt += bytes;
	}
	close(sockfd);

	/* converting C and print it to stdout */
	C = ntohl(net_int); /* ntohl translates a long integer from network byte order to host byte order */
	printf("# of printable characters: %u\n", C);
	exit(0);
}
