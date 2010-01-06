
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
    int i, j;
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

    int *p;
    p = (int *)routingfile.content;
    ri.nrof_ways = *p++;

    ri.nrof_nodes = *p++;

    RoutingWay *pw = (RoutingWay *)p;
    ri.ways = pw;
    pw = pw + ri.nrof_ways;

    ri.nodes = (RoutingNode *)pw;

    ri.tagsets = (RoutingTagSet *)(ri.nodes + ri.nrof_nodes);


    printf("Number of nodes: %d\n", ri.nrof_nodes);
    printf("Number of ways: %d\n", ri.nrof_ways);

    for (i = 0; i < 5; i++) {
        RoutingTagSet *ts;
        printf("Way %d: %d (%lf %lf) - %d (%lf %lf) : ", i, 
                ri.ways[i].from, ri.nodes[ri.ways[i].from].lat, ri.nodes[ri.ways[i].from].lon, 
                ri.ways[i].next, ri.nodes[ri.ways[i].next].lat, ri.nodes[ri.ways[i].next].lon);
        ts = (void *)ri.tagsets + ri.ways[i].tagset;
        for (j = 0; j < ts->size; j++) {
            printf("%d ", ts->tags[j]);
        }
        printf("\n");
        
        int k = ri.nrof_ways - 1 - i;
        printf("Way %d: %d (%lf %lf) - %d (%lf %lf) : ", k, 
                ri.ways[k].from, ri.nodes[ri.ways[k].from].lat, ri.nodes[ri.ways[k].from].lon, 
                ri.ways[k].next, ri.nodes[ri.ways[k].next].lat, ri.nodes[ri.ways[k].next].lon);
        ts = (void *)ri.tagsets + ri.ways[k].tagset;
        for (j = 0; j < ts->size; j++) {
            printf("%d ", ts->tags[j]);
        }
        printf("\n");
    }


    // Test routing
    Route *route;
    int from[] = {5499470, 292874624, 5499470, 292874624, 292874634, 424189, 424181};
    int to[] = {277299251, 292820169, 609217, 523288845, 31659052, 424181, 424189};

    RoutingProfile profile;
    for (i = 0; i < NROF_TAGS; i++) {
        profile.penalty[i] = 1.0;
    }

    for (i = 0; i < 7; i++) {
        printf("Test route from %d to %d\n", from[i], to[i]);
        route = ww_routing_astar(&ri, &profile, from[i], to[i]);
        if (!route) {
            printf("No route found.\n");
        } else {
            printf("Optimal route: %d nodes, %lf m\n", route->nrof_nodes, route->length);
            int k;
            /*
            for (k = 0; k < route->nrof_nodes; k++) {
                printf("%d ", route->nodes[k].id);
            }
            printf("\n");
            */
        }
        printf("\n");
    }
    
    // Test finding closest nodes
    RoutingNode **nodes_sorted_by_lat = ww_nodes_get_sorted_by_lat(&ri);

    printf("Created list of nodes sorted by latitude.\n");

    double lat, lon;
    RoutingNode *nd;
    lat = 59.4271;
    lon = 17.837627433342;
    nd = ww_find_closest_node(&ri, nodes_sorted_by_lat, lat, lon);

    printf("Node closest to (%lf, %lf): %d (%lf, %lf)\n", lat, lon, nd->id, nd->lat, nd->lon);
}

