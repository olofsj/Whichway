
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include "whichway_internal.h"

int
main(int argc, char **argv)
{
    File routingfile;
    struct stat st;
    char *filename, *outfilename;
    int i;
    RoutingIndex ri;
    
    
    printf("Whichway Test\n");

    if (argc > 1) {
        filename = argv[1];
    } else {
        printf("Input file must be specified.\n");
        return 0;
    }


    routingfile.file = strdup(filename);
    routingfile.fd = -1;
    routingfile.size = 0;

    /* Open file descriptor and stat the file to get size */
    routingfile.fd = open(routingfile.file, O_RDONLY);
    if (routingfile.fd < 0)
        return 0;
    if (fstat(routingfile.fd, &st) < 0) {
        close(routingfile.fd);
        return 0;
    }
    routingfile.size = st.st_size;

    /* mmap file contents */
    routingfile.content = mmap(NULL, routingfile.size, PROT_READ, MAP_SHARED, routingfile.fd, 0);
    if ((routingfile.content == MAP_FAILED) || (routingfile.content  == NULL)) {
        close(routingfile.fd);
        return 0;
    }

    printf("filesize: %d\n", routingfile.size);
    printf("sizeof(rw): %d\n", (int)sizeof(RoutingWay));

    ri.size = routingfile.size/sizeof(RoutingWay);
    ri.ways = (RoutingWay *)routingfile.content;


    /*
    printf("Number of ways: %d\n", ri.size);
    for (i = 0; i < 2; i++) {
        printf("Way %d: %d (%lf %lf) - %d (%lf %lf) [%d %d %d] %lf\n", i, 
                ri.ways[i].from.id, ri.ways[i].from.lat, ri.ways[i].from.lon, 
                ri.ways[i].to.id, ri.ways[i].to.lat, ri.ways[i].to.lon,
                ri.ways[ri.ways[i].next-1].from.id,
                ri.ways[ri.ways[i].next].from.id,
                ri.ways[ri.ways[i].next+1].from.id,
                ri.ways[i].length);
        int k = ri.size - 1 - i;
        printf("Way %d: %d (%lf %lf) - %d (%lf %lf) [%d %d %d] %lf\n", k, 
                ri.ways[k].from.id, ri.ways[k].from.lat, ri.ways[k].from.lon, 
                ri.ways[k].to.id, ri.ways[k].to.lat, ri.ways[k].to.lon,
                ri.ways[ri.ways[k].next-1].from.id,
                ri.ways[ri.ways[k].next].from.id,
                ri.ways[ri.ways[k].next+1].from.id,
                ri.ways[k].length);
    }
    */

    Route *route;
    int from[3] = {5499470, 292874624, 5499470};
    int to[3] = {277299251, 292820169, 609217};

    for (i = 0; i < 3; i++) {
        printf("Test route from %d to %d\n", from[i], to[i]);
        route = ww_routing_astar(&ri, from[i], to[i]);
        if (!route) {
            printf("No route found.\n");
        } else {
            printf("Optimal route: %d nodes, %lf m\n", route->nrof_nodes, route->length);
            int k;
            for (k = 0; k < route->nrof_nodes; k++) {
                printf("%d ", route->nodes[k].id);
            }
            printf("\n");
        }
    }
    
    //for (i = 0; i < 10; i++)
        //printf("Node %d: %d (%lf %lf)\n", i, nodes[i]->id, nodes[i]->lat, nodes[i]->lon);
}

