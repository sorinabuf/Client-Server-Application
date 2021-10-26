#include "utils.h"

using namespace std;

/**
 * @brief Converts the payload data type of a tcp package to string message.
 *
 * @param recv_msg tcp package
 */
const char *convert_payload(struct tcp_package *recv_msg, int *return_flag) {
    const char *type;

    if (recv_msg->type == INT) {
        type = "INT";
    } else if (recv_msg->type == SHORT_REAL) {
        type = "SHORT_REAL";
    } else if (recv_msg->type == FLOAT) {
        type = "FLOAT";
    } else if (recv_msg->type == STRING) {
        type = "STRING";
    } else {
        fprintf(stderr, "Invalid message type: 0, 1, 2 or 3!");
        *return_flag = 1;
    }

    return type;
}

int main(int argc, char *argv[]) {
    // disenables output buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    char message[MESSAGE_LEN];
    int tcp_client_sock, addr_convert, connection, id_send, selection,
        nagle_flag = 1, ok = 1, subscription_send, server_msg, return_flag;
    struct sockaddr_in server_addr;
    const char delim[3] = " \n";
    const char *type;
    struct subscription subscribe, unsubscribe;

    // verifies the number of parameters
    if (argc != 4) {
        fprintf(stderr,
                "Incorrect number of arguments: ./subscriber <ID_CLIENT> "
                "<IP_SERVER> <PORT_SERVER>");
        return -1;
    }

    // file descriptors set through which data may be received
    fd_set server_fds;
    fd_set server_fds_aux;

    // maximum active file descriptor
    int fd_max;

    // resets the file descriptors set
    FD_ZERO(&server_fds);
    FD_ZERO(&server_fds_aux);

    // creates the tcp client socket used for communication
    tcp_client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_client_sock < 0) {
        fprintf(stderr, "Could not create socket!");
        return -1;
    }

    // completes the family, port and address of the server to which the
    // connection is about to be made
    server_addr.sin_family = AF_INET;

    server_addr.sin_port =
        htons(atoi(argv[3]));  // port extracted from command line parameters

    // converts the string address in IPv4 format extracted from command line
    // parameters to a network address
    addr_convert = inet_aton(argv[2], &server_addr.sin_addr);
    if (addr_convert < 0) {
        fprintf(stderr, "Invalid conversion from string to network address!");
        return -1;
    }

    // realizes the connection to the server
    connection = connect(tcp_client_sock, (struct sockaddr *)&server_addr,
                         sizeof(server_addr));
    if (connection < 0) {
        fprintf(stderr, "Connection to server failed!");
        return -1;
    }

    // sends client id extracted from command line parameters to the server
    id_send = send(tcp_client_sock, argv[1], strlen(argv[1]) + 1, 0);
    if (id_send < 0) {
        fprintf(stderr, "Cound not send client ID to server!");
        return -1;
    }

    // disenables Nagle algorithm for server connection
    setsockopt(tcp_client_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&nagle_flag,
               sizeof(int));

    // sets stdin in the file descriptor set
    FD_SET(STDIN_FILENO, &server_fds);
    // sets communication socket in the file descriptor set
    FD_SET(tcp_client_sock, &server_fds);

    fd_max = tcp_client_sock;  // updates maximum file descriptor

    while (ok) {
        server_fds_aux =
            server_fds;  // copies the file descriptors in an auxiliary variable

        // extracts the file descriptors ready for reading or writing
        selection = select(fd_max + 1, &server_fds_aux, NULL, NULL, NULL);
        if (selection < 0) {
            fprintf(stderr, "Could not select file descriptors!");
            return -1;
        }

        for (int i = 0; i <= fd_max; ++i) {
            // verifies whether the file descriptor is set from the selection
            if (FD_ISSET(i, &server_fds_aux)) {
                // case for stdin file descriptor
                if (i == STDIN_FILENO) {
                    // gets the message from standard input
                    memset(message, 0, MESSAGE_LEN);
                    fgets(message, MESSAGE_LEN - 1, stdin);

                    // treats the exit command, that disconnects and closes the
                    // client
                    if (strncmp(message, "exit", 4) == 0) {
                        ok = 0;
                        break;
                    }

                    // treats the subscribe command by sending a subscription
                    // message represented through a struct subscription
                    // variable to the server
                    if (strncmp(message, "subscribe", 9) == 0) {
                        char *token = strtok(message, delim);

                        // sets subscription type
                        subscribe.type_subscription = SUBSCRIBE;

                        token = strtok(NULL, delim);
                        if (token == NULL) {
                            fprintf(stderr, "Invalid TCP client command!");
                            return -1;
                        }
                        // sets subscription topic
                        memcpy(subscribe.topic, token, strlen(token) + 1);

                        token = strtok(NULL, delim);
                        if (token == NULL) {
                            fprintf(stderr, "Invalid TCP client command!");
                            return -1;
                        }
                        // sets sf subscription flag
                        subscribe.sf = atoi(token);

                        // sends the subscription message to the server
                        subscription_send =
                            send(tcp_client_sock, &subscribe,
                                 sizeof(struct subscription), 0);
                        if (subscription_send < 0) {
                            fprintf(stderr,
                                    "Cound not send client subscription to "
                                    "server!");
                            return -1;
                        }

                        // prints feedback line
                        printf("Subscribed to topic.\n");

                        // treats the unsubscribe command by sending a
                        // subscription message represented through a struct
                        // subscription variable to the server
                    } else if (strncmp(message, "unsubscribe", 11) == 0) {
                        char *token = strtok(message, delim);

                        // sets subscription type
                        unsubscribe.type_subscription = UNSUBSCRIBE;

                        token = strtok(NULL, delim);
                        if (token == NULL) {
                            fprintf(stderr, "Invalid TCP client command!");
                            return -1;
                        }
                        // sets subscription topic
                        memcpy(unsubscribe.topic, token, strlen(token) + 1);

                        // sends the subscription message to the server
                        subscription_send =
                            send(tcp_client_sock, &unsubscribe,
                                 sizeof(struct subscription), 0);
                        if (subscription_send < 0) {
                            fprintf(stderr,
                                    "Cound not send client subscription to "
                                    "server!");
                            return -1;
                        }

                        // prints feedback line
                        printf("Unsubscribed from topic.\n");

                    } else {
                        // treats invalid input command
                        fprintf(stderr,
                                "Invalid TCP client command: subscribe <TOPIC> "
                                "<SF> or unsubscribe <TOPIC> or exit!");
                        return -1;
                    }
                }

                // case for server file descriptor
                if (i == tcp_client_sock) {
                    memset(message, 0, MESSAGE_LEN);

                    // receives a message from server
                    server_msg = recv(tcp_client_sock, message,
                                      sizeof(struct tcp_package), 0);
                    if (server_msg < 0) {
                        fprintf(stderr,
                                "Cound not receive message from server!");
                        return -1;
                    }

                    // when no bytes are received, closes the connection
                    if (server_msg == 0) {
                        ok = 0;
                        break;
                    }

                    // extracts the tcp package received from the server
                    struct tcp_package *recv_msg =
                        (struct tcp_package *)message;

                    return_flag = 0;
                    type = convert_payload(recv_msg, &return_flag);
                    if (return_flag == 1) {
                        return -1;
                    }

                    // prints receiving message feedback line
                    printf("%s:%hu - %s - %s - %s\n", recv_msg->udp_ip,
                           recv_msg->udp_port, recv_msg->topic, type,
                           recv_msg->content);
                }
            }
        }
    }

    // closes communication socket
    close(tcp_client_sock);

    return 0;
}
