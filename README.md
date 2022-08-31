# OS-PCC-Server

Implementation of a toy client/server architecture: a printable characters counting (PCC) server.
Written in c as an assignment in operating-systems course (Tel Aviv University).

# Assignments goal
The goal of this assignment is to gain experience with sockets and network programming.
Clients connect to the server and send it a stream of bytes.
The server counts how many of the bytes are printable and returns that number to the client.
The server also maintains overall statistics on the number of printable characters it has received
from all clients. When the server terminates, it prints these statistics to standard output.

# Server side
1. The server accepts TCP connections from clients.
2. A client that connects sends the server a stream of N bytes (N is determined by the client and is not a global constant).
3. The server counts the number of printable characters in the stream (a printable character is a
  byte b whose value is 32 ≤ b ≤ 126).
4/ Once the stream ends, the server sends the count back to the client over the same connection.
5. In addition, the server maintains a data structure in which it counts the number of times each printable character was observed in all the connections.
6. When the server receives a SIGINT, it prints these counts and exits.

# Client side
1. The client creates a TCP connection to the server and sends it the contents of a user-supplied file.
2. The client then reads back the count of printable characters from the server, prints it, and exits.
