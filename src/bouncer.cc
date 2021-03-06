// Project assignment 3 solution; compile with C++11.
// Source: Beej's Guide to Network Programming examples: listener.c and talker.c
// DESIGN:
// uses 2 sockets, one for receiving, used by one thread; one for sending, used by the other thread
// The sender sends a UDP packet every second. Waiting is performed on a condition variable, so the thread can wake up if the receiver receives something. The sender sends a single byte as payload, taken from the global value field.
// The receiver changes this global value upon receipt of a packet; it then notifies the sender who will send a new packet using the updated value.

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <zlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "SimpleHeader.h"

#define MYPORT "5010"	// the port on which we receive
#define DESTPORT "5000" // the port where we send packets
#define SZ 512 // Set the size for temporary buffer payload for receiver

// we want to print the character AND the space as an atomic operation, i.e.
// once a thread prints a character, another thread should not print something else.
std::mutex print_mutex;



// How to compile bouncer.cc
// Step 1) Run in terminal g++ -std=c++11 -o bouncer bouncer.cc SimpleHeader.cc -lpthread -lz
// Step 2) Run the generated executable bouncer by running ./bouncer 127.0.0.1
// Step 3) Run in a different terminal the appropriate netcat command if needed

/* Cmd line arguments: bouncer destination_addr */

#define MAXBUFLEN 512 // Read at most 512 bytes?

/* Since we use threads, we declare some global variables first.
   It works just fine to declare the globals outside of a struct, but gathered
   in a struct just feels you are more in control :)
*/

struct globals_t {
  unsigned char value='a';  // the byte value being sent
  std::condition_variable cv_timer;  // for the timed wait
  // protect concurrent access by the other thread;
  std::mutex exclude_other_mtx;
};

struct globals_t gl;
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// creates the socket for receiving, using family to identify the protocol and port for the port we will be receiving on
// family = AF_INET or AF_INET6 or AF_UNSPEC
// port = string representing the port number we bind to as receiver
// return
//   -1 on error
//   the socked descriptor if OK
// create_sock_recv is the receiver function
int create_sock_recv(int family, const char * port) {
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int the_sock;

    // start with the hints (local address, UDP, etc
  memset(&hints, 0, sizeof hints);
  hints.ai_family = family; // set to AF_INET to use IPv4
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE; // use own IP

  if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
    std::cerr << "getaddrinfo for recv: " << gai_strerror(rv) << std::endl;
    return -1;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((the_sock = socket(p->ai_family, p->ai_socktype,
			 p->ai_protocol)) == -1) {
      std::cerr << "recv socket: " << std::strerror(errno) << std::endl;
      continue;
    }

    if (bind(the_sock, p->ai_addr, p->ai_addrlen) == -1) {
      close(the_sock);
      std::cerr << "recv socket bind: " << std::strerror(errno) << std::endl;
      continue;
    }

    break;
  }

  if (p == NULL) {
    std::cerr << "recv socket: failed to find a suitable network" << std::endl;
    return -1;
  }

  freeaddrinfo(servinfo);
  return the_sock;
}

// creates the socket for sending, using family to identify the protocol and port to send to
// family = AF_INET or AF_INET6 or AF_UNSPEC
// port = string representing the port number we send to
// dest_host = string representing the destination address or name
// return
//   -1 on error
//   the socked descriptor if OK
// this is the sender function
int create_sock_send(int family, const char * port, const char * dest_host) {
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = family; // set to AF_INET to use IPv4
  hints.ai_socktype = SOCK_DGRAM;

  if ((rv = getaddrinfo(dest_host, port, &hints, &servinfo)) != 0) {
    std::cerr << "getaddrinfo: " << gai_strerror(rv) << std::endl;
    return -1;
  }

  // loop through all the results and make a socket
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
			 p->ai_protocol)) == -1) {
      std::cerr << "send socket: " << std::strerror(errno) << std::endl;
      continue;
    }

    break;
  }

  if (p == NULL) {
    std::cerr << "send socket: failed to create" << std::endl;
    return -1;
  }

  // connect, save the destination address with the socket so we can use send instead of sendto
  if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
    std::cerr << "Send socket connect: " << std::strerror(errno) << std::endl;
    return -1;
  }

  return sockfd;
}

// ********** the workers ***********
// the sender thread;
// sockfd: socket opened for sending the datagrams
// From the sender thread, read at most 512 bytes from the
// start of the file given as argument. Calculate the CRC32 value for the data read and output
// it.
/*
Calculate the CRC32 value for the data read and output it.
??Remove the CRC32 output from the sender thread and, using the code you developed for
Project Assignment 1, assemble ONE valid DATA packet, and send it from the sender thread,
to the destination and port given as command line arguments. Maintain the timeout pro-
tection available from PA2.
*/
void wipeBuffer(char*& buf, int n);
void loadBuffer(char* buf, char* c, unsigned int n);

void wipeBuffer(char* &buf, int n){
   for(int i=0; i<n; i++){
     buf[i] = 0;
   }
}
void loadBuffer(char* buf, char* c, unsigned int n){
  for(int i=0; i<n; i++){
    buf = c;
  }
}
void send_thread(int sockfd) {
  std::ofstream of;
  int numbytes;
  char buf[MAXBUFLEN]; // read at most 512 bytes
  buf[MAXBUFLEN] = '\0';
  char* bufPtr = buf;
  unsigned long crc;
  struct addrinfo hints, *res;
  crc=crc32(0L, NULL, 0);
  // create a mutex and a lock for the condition variable
  std::mutex mtx;
  std::unique_lock<std::mutex> lock(mtx);

  /*
    Therefore, you create a header object (use the header class you tested in Project Assignment 1), then pass
    the entire buffer to the sendto or send socket function. First, just implement the send. You can receive
    with netcat, dump the packet to a file, and examine the file with hexdump to make sure the packet looks all right.
  */

  // create a packet object
  SimpleHeader* h_ = new SimpleHeader;
  // Set packet object values
  // set Type
  h_->setType(2);
  // set TR
  h_->setTR(1);
  // set Window
  h_->setWindow(5);
  // set Seq num
  h_->setSeqNum(5);
  // set Length
  h_->setPayloadLength(30);
  // set Payload
  h_->setEntirePayload("Hello", strlen("Hello"));
  // do packet function call

  // load Buffer
  char* c = "hello"; // h_->getEntirePayload();
  // void loadBuffer(char* buf, const char* c, int n){
  unsigned int messageLength = strlen("hello") +1;
  loadBuffer(bufPtr, "hello", messageLength);
  std::cout<<"The buffer at index 0 is" << buf[0] << std::endl;
  std::cout<<"The buffer at index 1 is" << buf[1] << std::endl;
  std::cout<<"The buffer at index 2 is" << buf[2] << std::endl;
  std::cout<<"The buffer at index 3 is" << buf[3] << std::endl;
  std::cout<<"The buffer at index 4 is" << buf[4] << std::endl;



  //std::cout << "The value of packetSize is " << h_->totalPacketSize() << std::endl;
  unsigned int packetSize = reinterpret_cast<unsigned int>(h_->totalPacketSize()); // char* to unsigned int
  //std::cout << "The new value of packetSize is " << packetSize << std::endl;

  // unsigned int to const char*
  sendto(sockfd, (const char*) h_->thePacket(), packetSize, 0, res->ai_addr, res->ai_addrlen);
  // Send packet buffer into the sendto or send socket function

  // open file in binary mode
  of.open("one.bin", std::ios::binary | std::ios::out);
  if (of.fail()) {
    std::cout << "Cannot create file" << std::endl;
    exit(EXIT_FAILURE);
  }

  while (1) {
    //wipeBuffer(buf, 128);
    // send then wait on cv
    buf[0] = gl.value;
    buf[1] = 5;
    buf[2] = 6;
    buf[3] = 7;
    buf[4] = 8;
    //std::cout<<"The value of buf[0] is " << buf[0] << std::endl;
    //std::cout<<"The value of h_->thePacket() is " << h_->thePacket() << std::endl;
    // calculate the crc32 val
    crc = crc32(crc, reinterpret_cast<const Bytef*>(bufPtr), MAXBUFLEN);
    //std::cout<<"\nThis is the value of crc " << crc << std::endl;
    if (send(sockfd, buf, 1, 0) != 1) {
      std::cerr << "Sender thread: " << std::strerror(errno) << std::endl;
    }
    if (gl.cv_timer.wait_for(lock, std::chrono::milliseconds(1000)) == std::cv_status::timeout) {
      // we can do something here in case our sleep was interrupted; for this problem, we don't need to do anything
    }
    else {
      // we can do something in case the timer expired. Nothing for us now.
    }
  }
}

// the receiver thread;
// sockfd: socket opened for receiving the datagrams
/*
Work on the receiver: start with the code you developed for Project Assignent 2. Modify
the main function to handle the command line arguments for the receiver. In the receiver
thread, open the file for writing, as binary.
??Receive the data packet in the receiver thread and save the payload to the file. Test that the
file received is intact by running diff between the file used as source (at sender)
and the file saved by the receiver.
*/

void recv_thread(int sockfd) {
  char s[INET6_ADDRSTRLEN];
  socklen_t addr_len;
  int numbytes, n;
  unsigned int len;
  struct sockaddr_in cliaddr;
  struct sockaddr_storage their_addr;
  char buf[MAXBUFLEN];
  std::ifstream of;
  char* bufPtr = buf;

  // Captures the first message
  n = recvfrom(sockfd, (char *) buf, SZ, 0, (struct sockaddr *) &cliaddr, &len);
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  printf("Start : %d\n", n);
  buf[n] = '\0';
  printf("Client : %s\n", buf);
/*
  // In the receiver thread, open the file for writing, as binary
  if (of.fail()) {
    std::cout << "Cannot create file" << std::endl;
    exit(EXIT_FAILURE);
  }
  // write at most 1024 bytes in it
  char payload[SZ];
  for (int i = 0; i <= 5; i++) {
    buf[i] = i;
  }
  // Write to file
  /*
  Receive the data packet in the receiver thread and save the payload to the file.
  Test that the file received is intact by running diff between the file used
  as source (at sender) and the file saved by the receiver.
  of.write(buf, SZ);
*/
  addr_len = sizeof(their_addr);
  while (1) {
    n = 0;
    wipeBuffer(bufPtr, n);
    std::cout << "The buffer[0] is " << buf[0] << "\n";
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
			     (struct sockaddr *)&their_addr, &addr_len)) == -1) {
      std::cout << "In the if statement in receiver thread" << std::endl;
      std::cerr << "receiver: " << std::strerror(errno) << std::endl;
    }
    //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    n = recvfrom(sockfd, (char *) buf, SZ, 0, (struct sockaddr *) &cliaddr, &len);
    printf("Start : %d\n", n);
    buf[n] = '\0';
    printf("Client : %s\n", buf);

    // modify the global value
    // should protect next assignment with a lock, but the other thread only
    // reads, so no problem so far
    gl.value = buf[0];

    // wake up the sender
    gl.cv_timer.notify_one();
  }
}
/*
First, modify the main function to handle the optional file parameter
from the project documentation. Open the file for reading as a binary file.
*/
// cmd line: bouncer destination
int main(int argc, const char* argv[])
{
  std::ofstream of;
  int sock_send, sock_recv;  // one socket for sending, another for receiving
  char buf[SZ];

  // Modify the main function to handle the command line arguments for
  // the receiver.

  // are args OK?
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " destination_addr" << std::endl;
    return 2;
  }

  // args: family, port as string
  if ((sock_recv = create_sock_recv(AF_INET, MYPORT)) < 0) {
    std::cerr << "I failed" << std::endl;
    return 1;
  }

  // args: family, port as string, destination name or addr as string
  if ((sock_send = create_sock_send(AF_INET, DESTPORT, argv[1])) < 0) {
    std::cerr << "I failed" << std::endl;
    return 1;
  }

  // start the threads
  std::thread tsender(send_thread, sock_send);
  std::thread treceiver(recv_thread, sock_recv);

  tsender.join();
  treceiver.join();

  // write 20 bytes in it

  for (int i = 0; i <= SZ; i++) {
    buf[i] = i;
  }
  of.write(buf, SZ);

  close(sock_send);
  close(sock_recv);
  of.close();

  return 0;
}
