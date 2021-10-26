#ifndef UTILS_H
#define UTILS_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#include <iostream>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;

#define MESSAGE_LEN 1600
#define ID_LEN 11
#define SUBSCRIBE 0
#define UNSUBSCRIBE 1
#define INT 0
#define SHORT_REAL 1
#define FLOAT 2
#define STRING 3

// structure of a subcription packet of a tcp client
struct subscription {
    uint8_t type_subscription;  // type of subscription, that may consist in a
                                // subscribe of unsubscribe flag
    char topic[50];             // topic to which the client subscribes
    uint8_t sf;                 // store-and-forward option
} __attribute__((packed));

// structure of package sent to a tcp client
struct tcp_package {
    char topic[50];           // topic of the message
    uint8_t type;             // data type
    char content[1501];       // payload of the package
    unsigned short udp_port;  // port of the udp client datagram sender
    char udp_ip[16];          // ip of the udp client datagram sender
} __attribute__((packed));

// structure of a datagram received from a udp client
struct udp_datagram {
    char topic[50];      // topic of the message
    uint8_t type;        // data type
    char content[1501];  // payload of the datagram
} __attribute__((packed));

#endif  // UTILS_H