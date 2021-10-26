********************************************************************************

                        321CA - BUF SORINA ANAMARIA

                 HOMEWORK NO. 2 - COMMUNICATION PROTOCOLS

        Client-server application TCP and UDP for messages management

********************************************************************************

    The main purpose of this homework is to implement a client-server 
application, where the server is responsible for creating the connections
between the clients, tcp clients are able to subscribe and unsubscribe to
different topics and receive messages relevant to the subjects and udp clients
represent the topic messages generators, already implemented in the homework skel.

    The homework was mainly implemented in the files: "server.cpp", respectively
"subscriber.cpp".

    In order to mentain the application protocol restrictions and conserve 
message sending logic, I created structures, that define the data received
or sent along the message retransmission process, as it follows:

    ‣ Structure of a subcription packet of a tcp client
        struct subscription {
            uint8_t type_subscription;  // type of subscription, that may consist
                                        // in a subscribe of unsubscribe flag
            char topic[50];             // topic to which the client subscribes
            uint8_t sf;                 // store-and-forward option
        } __attribute__((packed));

    ‣ Structure of package sent to a tcp client
        struct tcp_package {
            char topic[50];           // topic of the message
            uint8_t type;             // data type
            char content[1501];       // payload of the package
            unsigned short udp_port;  // port of the udp sender client datagram 
            char udp_ip[16];          // ip of the udp sender client datagram 
        } __attribute__((packed));
    
    ‣ Structure of a datagram received from a udp client
        struct udp_datagram {
            char topic[50];      // topic of the message
            uint8_t type;        // data type
            char content[1501];  // payload of the datagram
        } __attribute__((packed)).

    Along with the structures, a big part of the clients management logic is
encapsulated in the defined data structures:

    ‣ unordered_map<int, string> clients_array ➩
        ➩ associates the socket of a client with the client's id;

    ‣ unordered_map<string, unordered_set<int>> topics 
        ➩ associates the existing topics to the sockets of the clients
        subscribed to the respective topics;

    ‣ unordered_set<string> clients_ids 
        ➩ used for storing the active clients' ids in every moment of time and
        for preventing the connection of two clients with the same ids;

    ‣ unordered_map<string, unordered_set<string>> topics_sf 
        ➩ associates the existing topics to the sockets of the clients
        subscribed to the respective topics, with sf flag on; 

    ‣ unordered_map<string, list<struct tcp_package>> saved_packages 
        ➩ used for storing the received packages of the mapping ids while the
        clients were disconnected;

    ‣ unordered_set<string> disconnected_ids 
        ➩ used for memorizing the current disconnected ids for
        store-and-forward type of subscriptions.

    The main flow of the tcp client logic is described in the "subscriber.cpp"
file, thus:

    • creates the socket used for server communication;

    • realizes the connection with the server;

    • sends the id of the client to the server;

    • defines the file descriptor set which is populated with the stdin 
    descriptor and the communication socket;

    • in an infinite loop, the following actions are performed until a loop exit
    condition is fulfilled:

        ⁃ extracts the file descriptors ready for reading or writing;

        - if stdin is along the active file descriptors set, gets the message
        from standard input; verifies whether it was received an exit command,
        case in which loop is broken and client will get disconnected and closed;
        verifies whether it was received a subscribe or unsubscribe command, 
        situation resulting in the creation of a subscription packet that
        is sent to the server;

        - if the communication socket is along the active file descriptors,
        receives a tcp package from the server, converts the content to the
        specified data type and prints the output according to the displaying
        conventions;
    
    • at the end, outside the loop, the communication socket is closed.

    The main flow of the server logic is described in the "server.cpp" file, 
thus:

    • creates the udp client socket used for communication;

    • associates the udp socket with the given port;

    • creates the tcp client socket used for communication;

    • associates the tcp socket with the given port;

    • makes the tcp socket passive in order to receive connection requests
    from clients;

    • defines the file descriptor set which is populated with the stdin 
    descriptor and the communication sockets;

    • in an infinite loop, the following actions are performed until a loop exit
    condition is fulfilled:

        ⁃ extracts the file descriptors ready for reading or writing;

        - if stdin is along the active file descriptors set, gets the message
        from standard input; verifies whether it was received an exit command,
        case in which loop is broken and server will get disconnected and
        closed, along with the connected clients;

        - if the tcp communication socket is along the active file descriptors,
        accepts the connection from a new client, receives the id of the client
        and treats the scenarios: two clients with the same id and a client 
        reconnected after a period of inactivity which has packets on hold that
        need to be sent;

        - if the udp communication socket is along the active file descriptors,
        receives a datagram from a udp client, completes the fields of a new
        tcp package structure and sends it to the tcp clients subscribed to the
        topic and stores it for later sending for disconnected tcp client with
        store-and-forward flag option on 1;

        - if the message is corresponding from one of the clients, when no bytes
        are received, closes the connection with the client end eliminates it 
        from the file descriptors set, otherwise treats the subscription and 
        unsubscription packages accordingly to the use of the defined data
        structures;
    
    • at the end, outside the loop, the server is closed along with the 
    connected clients.

    For more details related to the actual implementation, verify the comments
along the code :).

********************************************************************************
