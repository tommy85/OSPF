//
//  router.c
//  router
//
//  Created by Haochen Ding on 2013-03-28.
//  Copyright (c) 2013 Haochen Ding. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "router.h"

struct router *router;
struct addrinfo *localinfo;
struct addrinfo *remoteinfo;
int UDP_socket;
int nbr_link;

struct circuit_DB_entry *circuit_db;
struct topology_DB_entry topology_db[NBR_ROUTER];
struct rib_entry rib[NBR_ROUTER];

struct router *
router_create(int id, char *host, int port1,int port2)
{
    
    /* member initialization */
    struct router *router = malloc(sizeof(struct router));
    
    router->router_id = id;

    memcpy(router->nse_host, host, strlen(host) + 1);
    
    
    router->nse_port = port1;
    router->router_port = port2;
    
    // create the log file name
    char str1[] = "router";
    char str2[] = ".log";
    char router_id[1];

    sprintf(router_id, "%d", router->router_id);
    
    char log_name[6];


    strcpy(log_name, str1);
    strcat(log_name, router_id);
    strcat(log_name, str2);
    

    // open the log file
    router->log_file = fopen(log_name, "w+t");
    if (router->log_file == NULL) {
        printf("Opening file error !!!\n");
    }

    return router;
}


void router_start(struct router *router)
{

    socket_init(router);

    send_init(router);

    receive_circuit_DB(router);
    
    send_hello(router);

    rcv_hello_ls(router);
    
    socket_final(router);

}

void socket_init(struct router *router)
{
    int status;
    struct addrinfo hints;
    char router_port[5];
    char nse_port[5];
    sprintf(router_port, "%d", router->router_port); // convert int port to string port for getaddrinfo to use
    sprintf(nse_port, "%d", router->nse_port);


    
    /* get the address info of local machine */
    
    memset(&hints, 0, sizeof(hints));  // make sure the struct is empty
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;    // use UDP datagram
    hints.ai_flags = AI_PASSIVE;       // fill in my IP for me

    if ((status = getaddrinfo(NULL, router_port, &hints, &localinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    
    // create the UDP socket for sending and receiving
    UDP_socket = socket(localinfo->ai_family, localinfo->ai_socktype, localinfo->ai_protocol);
    
    if (UDP_socket == -1) {
        fprintf(stderr, "socket creating error !!!\n");
        close(UDP_socket);
        exit(1);
    }
    
    //bind the socket to local machine
    status = bind(UDP_socket, localinfo->ai_addr, localinfo->ai_addrlen);

    if (status == -1) {
        fprintf(stderr, "socket binding error !!!\n");
        close(UDP_socket);
        exit(1);
    }
    
    /* get the address info of NSE emulator machine */
    
    memset(&hints, 0, sizeof(hints));  // make sure the struct is empty
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;    // use UDP datagram
    
    if ((status = getaddrinfo(router->nse_host, nse_port, &hints, &remoteinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        close(UDP_socket);
        exit(1);
    }
    
    // connect local UDP socket to the nse emulator
    status = connect(UDP_socket, remoteinfo->ai_addr, remoteinfo->ai_addrlen);
    
    if (status == -1) {
        fprintf(stderr, "socket conneting error !!!\n");
        close(UDP_socket);
        exit(1);
    }
    
    
    
}

void send_init(struct router *router)
{
    int bytes_sent;
    
    // initialize the INIT packet
    struct pkt_INIT *init = malloc(sizeof(struct pkt_INIT));
    init->router_id = router->router_id;
    
    // send the INIT packet
    bytes_sent = (int) send(UDP_socket, init, sizeof(struct pkt_INIT), 0);
    
    // check if the packet couldn't be sent or be sent incompletely
    if (bytes_sent == -1) {
        fprintf(stderr, "INIT packet sending error !!!\n");
        exit(1);
    } else if (bytes_sent != sizeof(struct pkt_INIT)) {
        fprintf(stderr, "INIT packet sending incomplete !!!\n");
    }
    
    fprintf(router->log_file, "pkt_INIT sent, R%d\n", router->router_id);
}


void receive_circuit_DB(struct router *router)
{
    int bytes_received, i, j;
    
    // allocate memory space for the incoming packet
    struct circuit_DB *circuit_database = malloc(sizeof(struct circuit_DB));
    
    // receive the circuit_database packet
    bytes_received = (int) recv(UDP_socket, circuit_database, sizeof(struct circuit_DB), 0);
    
    // check if the packet couldn't be received or be received incompletely or connection lost
    if (bytes_received == -1) {
        fprintf(stderr, "circuit_DB packet sending error !!!\n");
        exit(1);
    }
    else if (bytes_received != sizeof(struct circuit_DB)) {
        fprintf(stderr, "circuit_DB packet sending incomplete !!!\n");
    }
    
    fprintf(router->log_file, "circuit_DB received, %d links\n", circuit_database->nbr_link);
    
    
    /* create the circuit database using an array */
    nbr_link = circuit_database->nbr_link;
    
    circuit_db = malloc(circuit_database->nbr_link * sizeof(struct circuit_DB_entry));
    
    for (i = 0; i < nbr_link; i++) {
        circuit_db[i].link_id = circuit_database->linkcost[i].link;
        circuit_db[i].link_cost = circuit_database->linkcost[i].cost;
        fprintf(router->log_file, "circuit_DB received, link %d, cost %d\n", circuit_database->linkcost[i].link, circuit_database->linkcost[i].cost);
    }
    
    /* initialize and update the topology database */
    for (i = 0; i < NBR_ROUTER; i++) {
        topology_db[i].destination = 0;
        topology_db[i].nbr_link = 0;
        
        topology_db[i].link_entry = malloc(NBR_ROUTER * sizeof(struct link_entry));
        
        for (j = 0; j < NBR_ROUTER; j++) {
            topology_db[i].link_entry[j].link = 0;
            topology_db[i].link_entry[j].cost = 0;
            topology_db[i].link_entry[j].router_id = 0;
        }
    }
    
    topology_db[0].destination = router->router_id;
    topology_db[0].nbr_link = nbr_link;
    
    for (i = 0; i < nbr_link; i++) {
        topology_db[0].link_entry[i].link = circuit_database->linkcost[i].link;
        topology_db[0].link_entry[i].cost = circuit_database->linkcost[i].cost;
    }
    
    for (i = 0; i < NBR_ROUTER; i++) {
        rib[i].cost = -1;
        rib[i].via = 0;
    }
    
    rib[router->router_id - 1].via = router->router_id;
    rib[router->router_id - 1].cost = 0;
    
    
    free(circuit_database);
}

void send_hello(struct router *router)
{
    int i, bytes_sent;
    
    // create a hello packet
    struct pkt_HELLO *hello = malloc(sizeof(struct pkt_HELLO));
    
    // initialize and send the hello packet to each neighbour
    
    for (i = 0; i < nbr_link; i++) {
        hello->link_id = circuit_db[i].link_id;
        hello->router_id = router->router_id;
        
        bytes_sent = (int) send(UDP_socket, hello, sizeof(struct pkt_HELLO), 0);
        
        // check if the packet couldn't be sent or be sent incompletely
        if (bytes_sent == -1) {
            fprintf(stderr, "HELLO packet sending error !!!\n");
            exit(1);
        } else if (bytes_sent != sizeof(struct pkt_HELLO)) {
            fprintf(stderr, "HELLO packet sending incomplete !!!\n");
        }
        
        fprintf(router->log_file, "pkt_HELLO sent, from R%d, via link %d\n", router->router_id, hello->link_id);
        
    }
}

void rcv_hello_ls(struct router *router)
{
    int bytes_received, i, j, counter, n;
    fd_set readfds;
    struct timeval tv;
    
    
    int len = 5 * sizeof(int); // the maximum possible buffer length
    char buffer[len];
    
    for(;;)
    {
        FD_ZERO(&readfds);
        FD_SET(UDP_socket, &readfds);
        n = UDP_socket + 1;
        tv.tv_sec = 3;
        
        int rv = select(n, &readfds, NULL, NULL, &tv);
        
        if (rv == 0) {
            break;
        }
        
        // receive the packet
        bytes_received = (int) recv(UDP_socket, buffer, len, 0);
        
        
        if (bytes_received == 2 * sizeof(int)) {
            // hello packet
            
            struct pkt_HELLO *hello = (struct pkt_HELLO*) buffer;
            fprintf(router->log_file, "pkt_HELLO received, from R%d, via link %d\n", hello->router_id, hello->link_id);
            
            // initialize the rib
            rib[hello->router_id - 1].via = hello->link_id;
            
            // create a ls PDU packet and initialize it
            for (i = 0; i < nbr_link; i++) {
                struct pkt_LSPDU *ls = malloc(sizeof(struct pkt_LSPDU));
                ls->sender = router->router_id;
                ls->router_id = router->router_id;
                ls->link_id = circuit_db[i].link_id;
                ls->cost = circuit_db[i].link_cost;
                ls->via = hello->link_id;
                
                // initialize the rib
                if (circuit_db[i].link_id == hello->link_id) {
                    rib[hello->router_id - 1].cost = circuit_db[i].link_cost;
                }
                
                // send the ls PDU packet
                int status = (int) send(UDP_socket, ls, sizeof(struct pkt_LSPDU), 0);
                
                if (status == -1) {
                    printf("LSPDU sending error !!!");
                    exit(1);
                }
                
                fprintf(router->log_file, "pkt_LSPDU sent, sender R%d, router_id %d, link id %d, cost %d, via %d\n", ls->sender, ls->router_id, ls->link_id, ls->cost, ls->via);
                
            }
            
        } else if (bytes_received == 5 * sizeof(int)) {
            // ls PDU packet
            
            
            struct pkt_LSPDU *ls = (struct pkt_LSPDU*) buffer;
            
            // ignore the packet
            for (i = 0; i < NBR_ROUTER; i++) {
                
                counter = 0;
                
                if (ls->sender == topology_db[i].destination) {
                    for (j = 0; j < NBR_ROUTER; j++) {
                        if (ls->link_id == topology_db[i].link_entry[j].link) {
                            rib[ls->router_id - 1].cost = ls->cost;
                            rib[ls->router_id - 1].via = ls->via;
                            break;
                        }
                        
                        counter++;
                    }
                    
                    if (counter != NBR_ROUTER) {
                        break;
                    } else {
                        for (j = 0; j < NBR_ROUTER; j++) {
                            if (topology_db[i].link_entry[j].link == 0) {
                                topology_db[i].link_entry[j].link = ls->link_id;
                                topology_db[i].link_entry[j].cost = ls->cost;
                                topology_db[i].link_entry[j].router_id = ls->router_id;
                                topology_db[i].nbr_link = topology_db[i].nbr_link + 1;
                                forward(ls, router);
                                break;
                            }
                        }
                        break;
                    }
                } else if (topology_db[i].destination == 0) {
                    topology_db[i].destination = ls->sender;
                    topology_db[i].nbr_link = topology_db[i].nbr_link + 1;
                    topology_db[i].link_entry[0].link = ls->link_id;
                    topology_db[i].link_entry[0].cost = ls->cost;
                    topology_db[i].link_entry[0].router_id = ls->router_id;
                    forward(ls, router);
                    break;
                }
            }
            
            
             
        } else {
            break;
        }
    }
}

void forward(struct pkt_LSPDU *ls, struct router *router)
{
    int i, bytes_sent;
    
    for (i = 0; i < nbr_link; i++) {
        if (ls->via == circuit_db[i].link_id) {
            continue;
        } else {
            if ((ls->cost < rib[ls->router_id - 1].cost && rib[ls->router_id - 1].via != 0) || (rib[ls->router_id - 1].via == 0)) {
                rib[ls->router_id - 1].cost = ls->cost;
                rib[ls->router_id - 1].via = ls->via;
            }
            
            ls->sender = router->router_id;
            ls->via = circuit_db[i].link_id;
            
            bytes_sent = (int) send(UDP_socket, ls, sizeof(struct pkt_LSPDU), 0);
            
            // check if the packet couldn't be sent or be sent incompletely
            if (bytes_sent == -1) {
                fprintf(stderr, "LSPDU packet forwarding error !!!\n");
                exit(1);
            } else if (bytes_sent != sizeof(struct pkt_LSPDU)) {
                fprintf(stderr, "LSPDU packet forwarding incomplete !!!\n");
            }
            
            fprintf(router->log_file, "pkt_LSPDU forward, sender R%d, router_id %d, link id %d, cost %d, via %d\n", ls->sender, ls->router_id, ls->link_id, ls->cost, ls->via);
        }
    }
}

void socket_final(struct router *router)
{
    int i,j;
    
    /* print the topology database */
    fprintf(router->log_file, "# Topology database\n");
    
    for (i = 0; i < NBR_ROUTER; i++) {
        if (topology_db[i].destination == 0) {
            break;
        } else {
            fprintf(router->log_file, "R%d -> R%d nbr link %d\n",router->router_id, topology_db[i].destination, topology_db[i].nbr_link);
            for (j = 0; j <NBR_ROUTER; j++) {
                if (topology_db[i].link_entry[j].link == 0) {
                    break;
                } else {
                    fprintf(router->log_file, "R%d -> R%d link %d cost %d\n", router->router_id, topology_db[i].destination, topology_db[i].link_entry[j].link, topology_db[i].link_entry[j].cost);
                }
            }
        }
    }
    
    /* print the RIB */
    fprintf(router->log_file, "# RIB\n");
    for (i = 0; i < NBR_ROUTER; i++) {
        if (i == router->router_id - 1) {
            fprintf(router->log_file, "R%d -> R%d -> Local, 0\n", router->router_id, router->router_id);
        } else {
            fprintf(router->log_file, "R%d -> R%d -> R%d, %d\n", router->router_id, i + 1, rib[i].via, rib[i].cost);
        }
    }
    
    // free the address info
    free(localinfo);
    free(remoteinfo);
    
    fclose(router->log_file);
    close(UDP_socket);
}
