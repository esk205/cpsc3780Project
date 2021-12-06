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
#include <vector>
#include <zlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "SimpleHeader.h"

#define DESTPORT "5000" // the port where we send packets
#define SZ 512 // Set the size for temporary buffer payload for receiver

// we want to print the character AND the space as an atomic operation, i.e.
// once a thread prints a character, another thread should not print something else.
std::mutex print_mutex;

#define MAXBUFLEN 512 // Read at most 512 bytes?

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

  freeaddrinfo(servinfo);
  return sockfd;
}

void wipeBuffer(char* &buf, int n){
   for(int i=0; i<n; i++){
     buf[i] = 0;
   }
}

void send_thread(int sockfd) {
  std::ifstream in_stream;
  unsigned long crc;
  crc = crc32(0L, NULL, 0);

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

  //unsigned int packetSize = reinterpret_cast<unsigned int>(h_->totalPacketSize()); // char* to unsigned int
  //send(sockfd, h_->thePacket(), packetSize, 0);

  // open file in binary mode
  in_stream.open("one.bin", std::ios::binary);
  if (in_stream.fail()) {
    std::cout << "Cannot create file" << std::endl;
    exit(EXIT_FAILURE);
  }
	
  std::vector<char> v_buf(MAXBUFLEN, 0);
  in_stream.read(v_buf.data(), MAXBUFLEN);
  auto bytes_read = in_stream.gcount();
  v_buf.resize(bytes_read);
  // set Payload
  h_->setEntirePayload(v_buf.data(), v_buf.size());
  // set Length
  h_->setPayloadLength(v_buf.size());

  // create a mutex and a lock for the condition variable
  // std::unique_lock<std::mutex> lock(gl.exclude_other_mtx);
  
  while (1) {
	// calc crc
    crc = crc32(crc, reinterpret_cast<const Bytef*>(v_buf.data()), v_buf.size());
	// std::cout << "This is the value of crc " << crc << std::endl;
    if (send(sockfd, h_->thePacket(), h_->totalPacketSize(), 0) < 0) {
      std::cerr << "Sender thread: " << std::strerror(errno) << std::endl;
    }
    /*
	if (gl.cv_timer.wait_for(lock, std::chrono::milliseconds(1000)) == std::cv_status::timeout) {
      // we can do something here in case our sleep was interrupted; for this problem, we don't need to do anything
    }
    else {
      // we can do something in case the timer expired. Nothing for us now.
    }*/

	// gl.cv_timer.wait(lock);
  }
}

int create_sock_recv(int family, const char * port, const char* host) {
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int the_sock;

  // start with the hints (local address, UDP, etc
  memset(&hints, 0, sizeof hints);
  hints.ai_family = family; // set to AF_INET to use IPv4
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE; // use own IP

  if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
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

	if(bind(the_sock, p->ai_addr, p->ai_addrlen) < 0) {
      std::cerr << "bind socket: " << std::strerror(errno) << std::endl;
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

void recv_thread(int sockfd) {
  int nRead;
  std::ofstream out_stream;
  std::vector<char> buf(1024, 0);
  SimpleHeader pHeader;
  simplepacket* pPacket = (simplepacket*)pHeader.thePacket();
  memset(pPacket, 0, sizeof(simplepacket));

  // In the receiver thread, open the file for writing, as binary
  out_stream.open("/home/sone/save.bin", std::ios::app | std::ios::binary);
  if (!out_stream.is_open()) {
    std::cout << "Cannot create file" << std::endl;
    exit(EXIT_FAILURE);
  }
  
  while (1) {
    //std::unique_lock<std::mutex> l(gl.exclude_other_mtx);

	if ((nRead = ::recv(sockfd, buf.data(), 1024, 0)) < 0) {
      std::cerr << "receiver: " << std::strerror(errno) << std::endl;
    }

	buf.resize(nRead);
	memcpy(pPacket, buf.data(), buf.size());
	out_stream << pPacket->data;
	// std::cout << pPacket->data << std::endl;
	out_stream.close();

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
	// wake up the sender
    //gl.cv_timer.notify_one();
  }

}

int main(int argc, const char* argv[])
{
  int sock_send;  // one socket for sending, another for receiving
  int sock_recv;  // one socket for sending, another for receiving

  // Modify the main function to handle the command line arguments for
  // the receiver.

  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " destination_addr" << std::endl;
    return 2;
  }

  // args: family, port as string, destination name or addr as string
  if ((sock_send = create_sock_send(AF_INET, DESTPORT, argv[1])) < 0) {
    std::cerr << "I failed" << std::endl;
    return 1;
  }

  if ((sock_recv = create_sock_recv(AF_INET, DESTPORT, argv[1])) < 0) {
    std::cerr << "I failed" << std::endl;
    return 1;
  }

  // start the threads
  std::thread tsender(send_thread, sock_send);
  std::thread treceiver(recv_thread, sock_recv);

  tsender.join();
  treceiver.join();
  
  close(sock_send);
  close(sock_recv);

  return 0;
}
