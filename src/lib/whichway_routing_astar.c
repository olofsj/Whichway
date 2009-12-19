
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "whichway_internal.h"

typedef struct _AStarScore AStarScore;

struct _AStarScore {
    RoutingWay *way;
    int index;
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

    if (m1->way->from.id > m2->way->from.id)
        return 1;
    if (m1->way->from.id < m2->way->from.id)
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
    Route *res = malloc(sizeof(Route));
    res->length = 0;
    res->nrof_nodes = 0;
    res->nodes = NULL;

    if (!score) 
        return res;

    res->length = score->g;
    res->nrof_nodes = 1;
    AStarScore *sc = score;
    while (sc->came_from) {
        res->nrof_nodes++;
        sc = sc->came_from;
    }

    printf("Path length: %d %lf\n", res->nrof_nodes, res->length);

    res->nodes = malloc(res->nrof_nodes * sizeof(RoutingNode));
    res->nodes[res->nrof_nodes-1].id = score->way->from.id;
    res->nodes[res->nrof_nodes-1].lat = score->way->from.lat;
    res->nodes[res->nrof_nodes-1].lon = score->way->from.lon;
    printf("Node %d: %d\n", res->nrof_nodes-1, res->nodes[res->nrof_nodes-1].id);
    sc = score;
    int i = res->nrof_nodes-2;
    while (sc->came_from) {
        res->nodes[i].id = sc->came_from->way->from.id;
        res->nodes[i].lat = sc->came_from->way->from.lat;
        res->nodes[i].lon = sc->came_from->way->from.lon;
        printf("Node %d: %d\n", i, res->nodes[i].id);
        sc = sc->came_from;
        i--;
    }

    return res;
}

Route * ww_routing_astar(RoutingIndex *ri, int from_id, int to_id) {
    List *closedset, *openset;
    AStarScore *sc;
    RoutingNode *from, *to;
    int from_index, to_index;

    from_index = routing_index_find_node(ri, from_id);
    to_index = routing_index_find_node(ri, to_id);
    from = &(ri->ways[from_index].from);
    to = &(ri->ways[to_index].from);

    printf("Routing from %d (%d) to %d (%d)\n", from_id, from_index,
            to_id, to_index);

    if (from_index < 0 || to_index < 0)
        return NULL;

    sc = malloc(sizeof(AStarScore));
    sc->index = routing_index_find_node(ri, from_id);
    sc->way = &(ri->ways[sc->index]);
    sc->came_from = NULL;
    sc->g = 0;
    sc->h = distance(from->lat, from->lon, to->lat, to->lon);
    sc->f = sc->h;
    printf("Start node f = %lf\n", sc->f);

    closedset = NULL;
    openset = list_prepend(NULL, sc);
    printf("List size = %d\n", list_count(openset));

    while (openset) {
        printf("openset size = %d\n", list_count(openset));
        printf("closedset size = %d\n", list_count(closedset));
        sc = pop_min_f_from_list(&openset);
        printf("Node %d f = %lf\n", sc->way->from.id, sc->f);

        if (sc->way->from.id == to->id) {
            printf("Optimal route found\n");
            // Found the optimal route
            Route *result = reconstruct_path(sc);

            // Clean up
            free(sc);
            List *l = openset;
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

        closedset = list_prepend(closedset, sc);

        printf("openset size = %d\n", list_count(openset));
        printf("closedset size = %d\n", list_count(closedset));

        RoutingWay *w = sc->way;
        int w_index = sc->index;
        while (w_index < ri->size && w->from.id == sc->way->from.id && w->next >= 0) {
            printf("%d %d\n", w->from.id, w->next);
            printf("%d %d\n", w->from.id, ri->ways[w->next].from.id);
            AStarScore *sc2 = malloc(sizeof(AStarScore));
            sc2->index = w->next;
            sc2->way = &(ri->ways[sc2->index]);

            if (list_find(closedset, sc2, score_compare_cb)) {
                free(sc2);
                w = &(ri->ways[++w_index]);
                continue;
            }

            double tentative_g_score = sc->g + distance(w->from.lat, w->from.lon, 
                    ri->ways[w->next].from.lat, ri->ways[w->next].from.lon);
            
            int tentative_is_better = 0;

            List *l = list_find(openset, sc2, score_compare_cb);
            if (!l) {
                openset = list_prepend(openset, sc2);
                tentative_is_better = 1;
            } else {
                free(sc2);
                sc2 = l->data;
                if (tentative_g_score < sc2->g) {
                    tentative_is_better = 1;
                }
            }

            if (tentative_is_better == 1) {
                sc2->came_from = sc;
                sc2->g = tentative_g_score;
                sc2->h = distance(w->from.lat, w->from.lon, to->lat, to->lon);
                sc2->f = sc2->g + sc2->f;
            }

            w = &(ri->ways[++w_index]);
        }
    }
 
    // No route found

    // Clean up
    List *l = openset;
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

    return NULL;

}

