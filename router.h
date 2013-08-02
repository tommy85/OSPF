//
//  router.h
//  router
//
//  Created by Haochen Ding on 2013-03-28.
//  Copyright (c) 2013 Haochen Ding. All rights reserved.
//

#define NBR_ROUTER 5 /* for simplicity we consider only 5 routers */

struct pkt_HELLO
{
	unsigned int router_id; /* id of the router who sends the HELLO PDU */
	unsigned int link_id; /* id of the link through which it is sent */
};

struct pkt_LSPDU
{
	unsigned int sender; /* sender of the LS PDU */
	unsigned int router_id; /* router id */
	unsigned int link_id; /* link id */
	unsigned int cost; /* cost of the link */
	unsigned int via; /* id of the link through which the LS PDU is sent */
};

struct pkt_INIT
{
	unsigned int router_id; /* id of the router that send the INIT PDU */
};

struct link_cost
{
	unsigned int link; /* link id */
	unsigned int cost; /* associated cost */
};

struct circuit_DB
{
	unsigned int nbr_link; /* number of links attached to a router */
	struct link_cost linkcost[NBR_ROUTER];
	/* we assume that at most NBR_ROUTER links are attached to each router */
};

struct router
{
    int router_id;
    int nse_port;
    int router_port;
    char nse_host[15];
    FILE *log_file;
    
};

struct circuit_DB_entry
{
    int link_id;
    int link_cost;
};

struct topology_DB_entry
{
    int destination;
    int nbr_link;
    struct link_entry *link_entry;
};

struct link_entry
{
    int link;
    int cost;
    int router_id;
};

struct rib_entry
{
    int via;
    int cost;
};

/* the initialization of the router */
struct router *router_create(int router_id, char *nse_host, int nse_port, int router_port);

/* the starting function of the router */
void router_start(struct router *router);

/* the initialization of the UDP socket */
void socket_init(struct router *router);

/* the function to send INIT packet */
void send_init(struct router *router);

/* the function to receive the circuit database */
void receive_circuit_DB(struct router *router);

/* the function to send the hello packets */
void send_hello(struct router *router);

/* the function to receive the hello packets and ls PDU packets */
void rcv_hello_ls(struct router *router);

/* the function to forward the ls packets */
void forward(struct pkt_LSPDU *ls, struct router *router);

/* the finalization of the UDP socket */
void socket_final(struct router *router);
