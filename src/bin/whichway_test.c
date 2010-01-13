
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
        printf("Way %d: %d (%lf %lf) : ", i, 
                ri.ways[i].next, ri.nodes[ri.ways[i].next].lat, ri.nodes[ri.ways[i].next].lon);
        ts = (void *)ri.tagsets + ri.ways[i].tagset;
        for (j = 0; j < ts->size; j++) {
            printf("%d ", ts->tags[j]);
        }
        printf("\n");
        
        int k = ri.nrof_ways - 1 - i;
        printf("Way %d: %d (%lf %lf) : ", k, 
                ri.ways[k].next, ri.nodes[ri.ways[k].next].lat, ri.nodes[ri.ways[k].next].lon);
        ts = (void *)ri.tagsets + ri.ways[k].tagset;
        for (j = 0; j < ts->size; j++) {
            printf("%d ", ts->tags[j]);
        }
        printf("\n");
    }


    // Test routing
    Route *route;
    int from[] = {31659011, 292832071, 503509195, 292832070, 503509194, 503509193, -1};
    double to_lat = 59.427165484589999; 
    double to_lon = 17.811964056444001;

    RoutingProfile profile;
    for (i = 0; i < NROF_TAGS; i++) {
        profile.penalty[i] = 1.0;
    }
    profile.max_route_length = 20000;

    printf("Test routing\n");
    route = ww_routing_astar(&ri, &profile, from, to_lat, to_lon, 50.0);
    if (!route) {
        printf("No route found.\n");
    } else {
        printf("Optimal route: %d nodes, %lf m\n", route->nrof_nodes, route->length);
        int k;
    }
    printf("\n");

    // Test finding closest nodes
    RoutingNode **nodes_sorted_by_lat = ww_nodes_get_sorted_by_lat(&ri);

    printf("Created list of nodes sorted by latitude.\n");

    double lat, lon;
    RoutingNode *nd;
    lat = 59.4271;
    lon = 17.837627433342;
    nd = ww_find_closest_node(&ri, nodes_sorted_by_lat, lat, lon);

    printf("Node closest to (%lf, %lf): %d (%lf, %lf)\n", lat, lon, nd->id, nd->lat, nd->lon);

    int *nodes;
    nodes = ww_find_nodes(&ri, nodes_sorted_by_lat, lat, lon, 50);
}

