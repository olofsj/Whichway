
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "whichway_internal.h"

Route *
ww_routing_astar(RoutingIndex *ri, RoutingNode *from, RoutingNode *to) {
    Route *res = malloc(sizeof(Route));
    res->length = 0;
    res->nrof_nodes = 0;
    res->nodes = NULL;

    return res;
}

