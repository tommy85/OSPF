//
//  main.c
//  router
//
//  Created by Haochen Ding on 2013-03-28.
//  Copyright (c) 2013 Haochen Ding. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "router.h"

int main(int argc, const char * argv[])
{

    if (argc != 5) {
        printf("Invalid arguments !!!\n");
        return 0;
    }
    
    char nse_host[15];
    
//    if (nse_host == NULL) {
//        printf("Not enough memory space for new nse_host string !!!\n");
//        return 0;
//    }
    
	int router_id = atoi(argv[1]);
    strcpy(nse_host, argv[2]);
	int nse_port = atoi(argv[3]);
	int router_port = atoi(argv[4]);
    
    struct router *router = router_create(router_id, nse_host, nse_port, router_port);
    

    router_start(router);
    
    return 0;
}

