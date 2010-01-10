
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "whichway_internal.h"

typedef struct _AStarScore AStarScore;

struct _AStarScore {
    RoutingWay *way;
    RoutingNode *node;
    int n_index;
    AStarScore *came_from;
    double g;
    double h;
    double f;
};

int
score_compare_cb(const void *n1, const void *n2)
{
    const AStarScore *m1 = NULL;
    const AStarScore *m2 = NULL;

    if (!n1) return(1);
    if (!n2) return(-1);

    m1 = n1;
    m2 = n2;

    if (m1->n_index > m2->n_index)
        return 1;
    if (m1->n_index < m2->n_index)
        return -1;
    return 0;
}

AStarScore * pop_min_f_from_list(List **list_p) {
    AStarScore *min_sc, *sc;
    List *list, *l, *min_l;

    list = *list_p;
    if (!list)
        return NULL;
    
    // Find the minimum score in the list
    min_sc = list->data;
    min_l = list;
    l = list;
    while (l = l->next) {
        sc = l->data;
        if (sc->f < min_sc->f) {
            min_sc = sc;
            min_l = l;
        }
    }

    // Remove from the list
    if (min_l->prev && min_l->next) {
        min_l->prev->next = min_l->next;
        min_l->next->prev = min_l->prev;
    } else if (min_l->prev) {
        min_l->prev->next = NULL;
    } else if (min_l->next) {
        min_l->next->prev = NULL;
        *list_p = min_l->next;
    } else {
        *list_p = NULL;
    }
    
    free(min_l);

    return min_sc;
}

Route * reconstruct_path(AStarScore *score) {
    int i, nrof_nodes;
    AStarScore *sc;
    Route *res;

    res = malloc(sizeof(Route));
    res->length = 0;
    res->nrof_nodes = 0;
    res->nodes = NULL;

    // If score is null, return an empty route
    if (!score) 
        return res;

    // Get the number of nodes
    nrof_nodes = 1;
    for (sc = score; sc->came_from; sc = sc->came_from) {
        nrof_nodes++;
    }
    res->nrof_nodes = nrof_nodes;

    // Add all nodes to the route
    res->nodes = malloc(nrof_nodes * sizeof(RoutingNode));
    res->nodes[nrof_nodes-1].id = score->node->id;
    res->nodes[nrof_nodes-1].lat = score->node->lat;
    res->nodes[nrof_nodes-1].lon = score->node->lon;
    for (sc = score, i = nrof_nodes-2; sc->came_from; sc = sc->came_from, i--) {
        res->nodes[i].id = sc->came_from->node->id;
        res->nodes[i].lat = sc->came_from->node->lat;
        res->nodes[i].lon = sc->came_from->node->lon;
        res->length += distance(sc->node->lat, sc->node->lon, 
                sc->came_from->node->lat, sc->came_from->node->lon);
    }

    return res;
}

Route * ww_routing_astar(RoutingIndex *ri, RoutingProfile *profile, 
        int *from_ids, double to_lat, double to_lon, double tolerance) {
    List *closedset, *openset, *l;
    AStarScore *sc;
    RoutingNode *from, *to;
    int from_index, to_index, from_id, i;
    Route *result = NULL;

    // Set up the lists of open and closed nodes
    closedset = NULL;
    openset = NULL;

    for (i = 0, from_id = from_ids[i]; from_id >= 0; from_id = from_ids[++i]) {
        // Find the start nodes in the index
        from_index = routing_index_find_node(ri, from_id);
        from = &(ri->nodes[from_index]);

        if (from_index < 0 || to_index < 0)
            continue;

        // Set up the score for the start nodes
        sc = malloc(sizeof(AStarScore));
        sc->n_index = from_index;
        sc->node = &(ri->nodes[sc->n_index]);
        sc->way = NULL;
        sc->came_from = NULL;
        sc->g = 0;
        sc->h = distance(from->lat, from->lon, to_lat, to_lon);
        sc->f = sc->h;
        openset = list_prepend(openset, sc);
    }

    // Loop until there are no more nodes in the open set
    while (openset) {
        sc = pop_min_f_from_list(&openset);
        closedset = list_prepend(closedset, sc);

        if (sc->h <= tolerance) {
            // Found the optimal route
            result = reconstruct_path(sc);

            // Finished, goto the end for cleanup
            goto end;
        }

        // Give up is longer than maximum length
        if (sc->g > profile->max_route_length)
            goto end;

        // Check if node is a dead end
        if (sc->node->way < 0)
            continue;

        // Check all the nodes connected to this node
        int w_index;
        for (w_index = sc->node->way; (w_index < ri->nrof_ways) && (ri->ways[w_index].from == sc->n_index); w_index++) {
            RoutingWay *w = &(ri->ways[w_index]);
            RoutingTagSet *tagset = (void *)ri->tagsets + w->tagset;

            AStarScore *sc2 = malloc(sizeof(AStarScore));
            sc2->n_index = w->next;
            sc2->node = &(ri->nodes[sc2->n_index]);
            sc2->way = w;

            // Check if node is in closed set, if so skip it
            if (list_find(closedset, sc2, score_compare_cb)) {
                free(sc2);
                continue;
            }

            // Calculate the tentative new score
            double tentative_g_score = sc->g + effective_distance(profile, tagset, sc->node->lat, sc->node->lon, 
                    sc2->node->lat, sc2->node->lon);
            

            // Is it better than the previous best path?
            int tentative_is_better = 0;

            l = list_find(openset, sc2, score_compare_cb);
            if (!l) {
                // Node not previously visited
                openset = list_prepend(openset, sc2);
                tentative_is_better = 1;
            } else {
                // Node visited, check if new path is better
                free(sc2);
                sc2 = l->data;
                if (tentative_g_score < sc2->g) {
                    tentative_is_better = 1;
                }
            }

            // If new path is better, update the score
            if (tentative_is_better == 1) {
                sc2->came_from = sc;
                sc2->g = tentative_g_score;
                sc2->h = distance(sc2->node->lat, sc2->node->lon, to_lat, to_lon);
                sc2->f = sc2->g + sc2->h;
            }
        }
    }
 
end:
    // Clean up
    l = openset;
    while (l) {
        List *ll = l->next;
        free(l->data);
        free(l);
        l = ll;
    }
    l = closedset;
    while (l) {
        List *ll = l->next;
        free(l->data);
        free(l);
        l = ll;
    }

    return result;

}

