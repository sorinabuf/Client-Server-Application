#include "utils.h"

using namespace std;

/**
 * @brief Treats the connection of a new client.
 */
int same_id_client(
    unordered_set<string> &clients_ids, char client_id[ID_LEN],
    unordered_map<int, string> &clients_array,
    unordered_set<string> &disconnected_ids,
    unordered_map<string, list<struct tcp_package>> &saved_packages,
    int client_sock, int send_package, struct sockaddr_in client_addr) {
    // memorizes the clients' id in order to point out the
    // id is being used
    clients_ids.insert(client_id);

    // maps the socket of the client to the id
    clients_array[client_sock] = client_id;

    // prints feedback line
    printf("New client %s connected from %s:%hu.\n", client_id,
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // verifies whether the new client has been previosuly
    // disconnected
    if (disconnected_ids.find(client_id) != disconnected_ids.end()) {
        // eliminates the id from disconnection list
        disconnected_ids.erase(client_id);

        // verifies whether there are any packages saved for
        // the client
        if (saved_packages.find(client_id) != saved_packages.end()) {
            // sends the packages stored for the given id to
            // the client
            for (struct tcp_package tcp_pck : saved_packages[client_id]) {
                send_package =
                    send(client_sock, &tcp_pck, sizeof(struct tcp_package), 0);

                if (send_package < 0) {
                    fprintf(stderr,
                            "Could not send package to "
                            "client!");
                    return -1;
                }
            }
        }

        // once the client reconnected, removes it from the
        // map of saved packages in order to optimize space
        // usage
        saved_packages.erase(client_id);
    }

    return 0;
}

/**
 * @brief Calculates maximum file descriptor by iterating through the list of
 * file descriptors and choosing the biggest still active value.
 */
int max_file_descriptor(int fd_max, fd_set *clients_fds) {
    int max = INT_MIN;
    for (int j = 3; j <= fd_max; ++j) {
        if (FD_ISSET(j, clients_fds)) {
            max = j;
        }
    }
    return max;
}

/**
 * @brief Converts payload of a datagram based on the given data type and
 * updates the tcp package content field.
 */
int udp_convert_content(struct udp_datagram *udp_package,
                        struct tcp_package *tcp_pck) {
    char *content;
    // case data type string
    if (udp_package->type == STRING) {
        // updates content of the tcp package
        memcpy((*tcp_pck).content, udp_package->content,
               strlen(udp_package->content) + 1);

        // case data type int
    } else if (udp_package->type == INT) {
        content = udp_package->content + 1;

        // casts content to uint32 type variable
        uint32_t integerContent = *(uint32_t *)content;
        integerContent = ntohl(integerContent);

        int newIntegerContent = integerContent;
        // verifies sign byte and updates the int number
        // consequently
        if (udp_package->content[0] == 1) {
            newIntegerContent = -integerContent;
        }

        // updates content of the tcp package
        memset((*tcp_pck).content, '\0', 1501);
        snprintf((*tcp_pck).content, sizeof((*tcp_pck).content), "%d",
                 newIntegerContent);

        // case data type short real
    } else if (udp_package->type == SHORT_REAL) {
        content = udp_package->content;

        // casts content to uint16 type variable
        uint16_t shortContent = *(uint16_t *)content;
        shortContent = ntohs(shortContent);

        // converts short variable to float based on the
        // convention specified
        float floatShortContent = (shortContent * 1.0) / 100;

        // updates content of the tcp package
        memset((*tcp_pck).content, '\0', 1501);
        snprintf((*tcp_pck).content, sizeof((*tcp_pck).content), "%0.2f",
                 floatShortContent);

        // case data type float
    } else if (udp_package->type == FLOAT) {
        content = udp_package->content + 1;

        // casts content to uint32 type variable
        uint32_t floatContent = *(uint32_t *)content;
        floatContent = ntohl(floatContent);

        // extracts number of decimals from the datagram
        // content
        char *decimals = content + sizeof(uint32_t);
        uint8_t nrDecimals = *(uint8_t *)decimals;

        // converts int variable to float based on the
        // convention specified
        float newFloatContent = floatContent * 1.0;
        for (int i = 0; i < nrDecimals; ++i) {
            newFloatContent = newFloatContent / 10;
        }

        // verifies sign byte and updates the float number
        // consequently
        if (udp_package->content[0] == 1) {
            newFloatContent = -newFloatContent;
        }

        // updates content of the tcp package
        memset((*tcp_pck).content, '\0', 1501);
        snprintf((*tcp_pck).content, sizeof((*tcp_pck).content), "%f",
                 newFloatContent);

    } else {
        // treats invalid payload type
        fprintf(stderr, "Invalid payload type: 0, 1, 2 or 3!");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    // disenables output buffering
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    char message[MESSAGE_LEN], client_id[ID_LEN];
    char *ip;
    int udp_client_sock, udp_bind, tcp_client_sock, tcp_bind, tcp_listen,
        ok = 1, selection, client_sock, flag = 1, receive_id, send_package,
        receive_datagram, receive_segment, return_flag;
    struct sockaddr_in udp_addr, tcp_addr, client_addr;
    socklen_t socklen = sizeof(client_addr);

    // associates the socket of a client with the client's id
    unordered_map<int, string> clients_array;

    // associates the existing topics to the sockets of the clients
    // subscribed to the respective topics
    unordered_map<string, unordered_set<int>> topics;

    // used for storing the active clients' ids in every moment of time and
    // for preventing the connection of two clients with the same ids
    unordered_set<string> clients_ids;

    // associates the existing topics to the sockets of the clients
    // subscribed to the respective topics, with sf flag on
    unordered_map<string, unordered_set<string>> topics_sf;

    // used for storing the received packages of the mapping ids while the
    // clients were disconnected
    unordered_map<string, list<struct tcp_package>> saved_packages;

    // used for memorizing the current disconnected ids for
    // store-and-forward type of subscriptions
    unordered_set<string> disconnected_ids;

    // verifies the number of parameters
    if (argc != 2) {
        fprintf(stderr,
                "Incorrect number of arguments: ./server <PORT_DORIT>!");
        return -1;
    }

    // file descriptors set through which data may be received
    fd_set clients_fds;
    fd_set clients_fds_aux;

    // maximum active file descriptor
    int fd_max;

    // resets the file descriptors set
    FD_ZERO(&clients_fds);
    FD_ZERO(&clients_fds_aux);

    // creates the udp client socket used for communication
    udp_client_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_client_sock < 0) {
        fprintf(stderr, "Could not create socket!");
        return -1;
    }

    // completes the family, port and address of the udp client the server
    // will listen to
    udp_addr.sin_family = AF_INET;

    udp_addr.sin_port =
        htons(atoi(argv[1]));  // port extracted from command line parameters

    udp_addr.sin_addr.s_addr = INADDR_ANY;

    // associates the udp socket with the given port
    udp_bind =
        bind(udp_client_sock, (struct sockaddr *)&udp_addr, sizeof(udp_addr));
    if (udp_bind < 0) {
        fprintf(stderr, "Binding error!");
        return -1;
    }

    // creates the tcp client socket used for communication
    tcp_client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_client_sock < 0) {
        fprintf(stderr, "Could not create socket!");
        return -1;
    }

    // completes the family, port and address of the tcp client the server
    // will listen to
    tcp_addr.sin_family = AF_INET;

    tcp_addr.sin_port =
        htons(atoi(argv[1]));  // port extracted from command line parameters

    tcp_addr.sin_addr.s_addr = INADDR_ANY;

    // associates the tcp socket with the given port
    tcp_bind =
        bind(tcp_client_sock, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr));
    if (tcp_bind < 0) {
        fprintf(stderr, "Binding error!");
        return -1;
    }

    // makes the tcp socket passive in order to receive connection requests
    // from clients
    tcp_listen = listen(tcp_client_sock, INT_MAX);
    if (tcp_listen < 0) {
        fprintf(stderr, "Listening error!");
        return -1;
    }

    // sets stdin in the file descriptor set
    FD_SET(STDIN_FILENO, &clients_fds);
    // sets communication sockets in the file descriptor set
    FD_SET(udp_client_sock, &clients_fds);
    FD_SET(tcp_client_sock, &clients_fds);

    // updates maximum file descriptor
    if (udp_client_sock > tcp_client_sock) {
        fd_max = udp_client_sock;
    } else {
        fd_max = tcp_client_sock;
    }

    while (ok) {
        clients_fds_aux = clients_fds;  // copies the file descriptors in an
                                        // auxiliary variable

        // extracts the file descriptors ready for reading or writing
        selection = select(fd_max + 1, &clients_fds_aux, NULL, NULL, NULL);
        if (selection < 0) {
            fprintf(stderr, "Could not select file descriptors!");
            return -1;
        }

        for (int i = 0; i <= fd_max; ++i) {
            // verifies whether the file descriptor is set from the
            // selection
            if (FD_ISSET(i, &clients_fds_aux)) {
                // case for stdin file descriptor
                if (i == STDIN_FILENO) {
                    // gets the message from standard input
                    memset(message, 0, MESSAGE_LEN);
                    fgets(message, MESSAGE_LEN - 1, stdin);

                    // treats the exit command, that disconnects and closes
                    // the server and the connected clients
                    if (strncmp(message, "exit", 4) == 0) {
                        ok = 0;
                        break;

                    } else {
                        // treats invalid input command
                        fprintf(stderr, "Invalid server command: exit!");
                        return -1;
                    }

                    // case for tcp client file descriptor
                } else if (i == tcp_client_sock) {
                    // accepts connection from a client
                    client_sock =
                        accept(tcp_client_sock, (struct sockaddr *)&client_addr,
                               &socklen);
                    if (client_sock < 0) {
                        fprintf(stderr, "Could not accept client!");
                        return -1;
                    }

                    // disenables Nagle algorithm for server connection
                    setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY,
                               (char *)&flag, sizeof(int));

                    // sets client socket in the file descriptor set
                    FD_SET(client_sock, &clients_fds);

                    // updates maximum file descriptor
                    if (client_sock > fd_max) {
                        fd_max = client_sock;
                    }

                    // receives id of the new client
                    memset(client_id, 0, ID_LEN);
                    receive_id = recv(client_sock, client_id, ID_LEN, 0);
                    if (receive_id < 0) {
                        fprintf(stderr, "Could not receive new client's id!");
                        return -1;
                    }

                    // verifies whether a client with the same id is already
                    // connected
                    if (clients_ids.find(client_id) == clients_ids.end()) {
                        return_flag = same_id_client(
                            clients_ids, client_id, clients_array,
                            disconnected_ids, saved_packages, client_sock,
                            send_package, client_addr);
                        if (return_flag == -1) {
                            return -1;
                        }

                    } else {
                        // if a client with the same received id is already
                        // connected, closes the duplicate client newly
                        // created socket end eliminates it from the file
                        // descriptors set
                        close(client_sock);
                        FD_CLR(client_sock, &clients_fds);

                        // prints feedback line
                        printf("Client %s already connected.\n", client_id);

                        fd_max = max_file_descriptor(fd_max, &clients_fds);
                    }

                    // case for udp client file descriptor
                } else if (i == udp_client_sock) {
                    // receives udp datagram from udp client
                    memset(message, 0, MESSAGE_LEN);

                    receive_datagram = recvfrom(
                        udp_client_sock, message, sizeof(struct udp_datagram),
                        0, (struct sockaddr *)&client_addr, &socklen);
                    if (receive_datagram < 0) {
                        fprintf(stderr,
                                "Could not receive a datagram from a UDP "
                                "client!");
                        return -1;
                    }

                    // cast and stores the udp package
                    struct udp_datagram *udp_package =
                        (struct udp_datagram *)message;

                    // declares a new tcp structure variable that will be
                    // sent to the tcp clients subscribed to the received
                    // topic message
                    struct tcp_package tcp_pck;

                    return_flag = udp_convert_content(udp_package, &tcp_pck);
                    if (return_flag == -1) {
                        return -1;
                    }

                    // completes topic field of the tcp message structure
                    memcpy(tcp_pck.topic, udp_package->topic,
                           strlen(udp_package->topic) + 1);

                    // converts udp the IPv4 format address to string
                    ip = inet_ntoa(client_addr.sin_addr);

                    // completes udp ip field of the tcp message structure
                    memcpy(tcp_pck.udp_ip, ip, strlen(ip) + 1);

                    // completes udp port field of the tcp message structure
                    tcp_pck.udp_port = client_addr.sin_port;

                    // completes message type field of the tcp message
                    // structure
                    tcp_pck.type = udp_package->type;

                    // verifies whether the received topic has a list of
                    // subscribed clients associated
                    if (topics.find(udp_package->topic) != topics.end()) {
                        // iterates through the topic's file descriptors
                        for (int index : topics[udp_package->topic]) {
                            // verifies whether the file descriptor is still
                            // active
                            if (FD_ISSET(index, &clients_fds)) {
                                // send tcp package to an active file
                                // descriptor
                                send_package =
                                    send(index, &tcp_pck,
                                         sizeof(struct tcp_package), 0);
                                if (send_package < 0) {
                                    fprintf(stderr,
                                            "Could not send package to "
                                            "client!");
                                    return -1;
                                }
                            }
                        }
                    }

                    // verifies whether the received topic has a list of
                    // subscribed clients associated with the
                    // store-and-forward option selected
                    if (topics_sf.find(udp_package->topic) != topics_sf.end()) {
                        // iterates through the ids of the clients
                        for (string id : topics_sf[udp_package->topic]) {
                            // verifies if the id is diconnected
                            if (disconnected_ids.find(id) !=
                                disconnected_ids.end()) {
                                // verifies whether there are any package
                                // already saved
                                if (saved_packages.find(id) ==
                                    saved_packages.end()) {
                                    // creates the list of saved packages
                                    // for the given id
                                    list<struct tcp_package> newList;
                                    newList.push_back(tcp_pck);
                                    saved_packages[id] = newList;
                                } else {
                                    // updates the list of saved packages
                                    // for the given id
                                    saved_packages[id].push_back(tcp_pck);
                                }
                            }
                        }
                    }

                    // case for receiving tcp segment
                } else {
                    // receives segment from tcp client
                    memset(message, 0, MESSAGE_LEN);

                    receive_segment = recv(i, message, MESSAGE_LEN, 0);
                    if (receive_segment < 0) {
                        fprintf(stderr,
                                "Could not receive a segment from a TCP "
                                "client!");
                        return -1;
                    }

                    // when no bytes are received, closes the connection
                    // with the client end eliminates it from the file
                    // descriptors set
                    if (receive_segment == 0) {
                        close(i);
                        FD_CLR(i, &clients_fds);

                        // marks the id as disconnected
                        disconnected_ids.insert(clients_array[i]);
                        clients_ids.erase(clients_array[i]);

                        // prints feedback line
                        printf("Client %s disconnected.\n",
                               clients_array[i].c_str());

                        fd_max = max_file_descriptor(fd_max, &clients_fds);

                    } else {
                        // stores and casts message to subscription type
                        // variable
                        struct subscription *client_subscription =
                            (struct subscription *)message;

                        // case client subscribing
                        if (client_subscription->type_subscription ==
                            SUBSCRIBE) {
                            // verifies whether the topic of subscription
                            // already has a list of clients associated
                            if (topics.find(client_subscription->topic) ==
                                topics.end()) {
                                // creates the list of client associated to
                                // the given topic
                                unordered_set<int> new_clients;
                                new_clients.insert(i);
                                topics[client_subscription->topic] =
                                    new_clients;
                            } else {
                                // updates the list of client associated to
                                // the given topic
                                topics[client_subscription->topic].insert(i);
                            }

                            // verifies the store-and-forward type of
                            // subscription
                            if (client_subscription->sf == 1) {
                                // verifies whether the topic of
                                // subscription already has a list of
                                // clients associated with store-and-forward
                                // option selected
                                if (topics_sf.find(
                                        client_subscription->topic) ==
                                    topics_sf.end()) {
                                    // creates the list of client associated
                                    // to the given topic
                                    unordered_set<string> new_saved_clients;
                                    new_saved_clients.insert(clients_array[i]);
                                    topics_sf[client_subscription->topic] =
                                        new_saved_clients;
                                } else {
                                    // updates the list of client associated
                                    // to the given topic
                                    topics_sf[client_subscription->topic]
                                        .insert(clients_array[i]);
                                }
                            }

                            // case client unsubscribing
                        } else {
                            // verifies whether the topic of subscription
                            // has a list of clients associated
                            if (topics.find(client_subscription->topic) !=
                                topics.end()) {
                                // verifies whether the client is found in
                                // the list of clients associated to the
                                // topic
                                if (topics[client_subscription->topic].find(
                                        i) !=
                                    topics[client_subscription->topic].end()) {
                                    // erases the client
                                    topics[client_subscription->topic].erase(i);
                                }
                            }

                            // verifies whether the topic of subscription
                            // has a list of clients associated with
                            // store-and-forward option on
                            if (topics_sf.find(client_subscription->topic) !=
                                topics_sf.end()) {
                                // verifies whether the client is found in
                                // the list of clients associated to the
                                // topic
                                if (topics_sf[client_subscription->topic].find(
                                        clients_array[i]) !=
                                    topics_sf[client_subscription->topic]
                                        .end()) {
                                    // erases the client
                                    topics_sf[client_subscription->topic].erase(
                                        clients_array[i]);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // disconnects and closes the server and the connected clients
    for (int i = 3; i <= fd_max; ++i) {
        if (FD_ISSET(i, &clients_fds)) {
            close(i);
        }
    }

    return 0;
}
