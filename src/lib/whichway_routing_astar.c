
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "whichway_internal.h"

typedef struct _AStarScore AStarScore;

struct _AStarScore {
    RoutingWay *way;
    double g;
    double h;
    double f;
};

AStarScore * pop_min_f_from_list(List **list_p) {
    AStarScore *min_sc, *sc;
    double min;
    List *list, *l, *min_l;

    list = *list_p;
    if (!list)
        return NULL;
    
    // Find the minimum score in the list
    min_sc = list->data;
    min = min_sc->f;
    min_l = list;
    l = list->next;
    while (l) {
        sc = l->data;
        if (sc->f < min_sc->f) {
            min_sc = sc;
            min = min_sc->f;
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

Route * ww_routing_astar(RoutingIndex *ri, RoutingNode *from, RoutingNode *to) {
    Route *res = malloc(sizeof(Route));
    res->length = 0;
    res->nrof_nodes = 0;
    res->nodes = NULL;

    List *closedset, *openset;
    AStarScore *sc;


    printf("Routing from %d (%d) to %d (%d)\n", from->id, routing_index_find_node(ri, from->id),
            to->id, routing_index_find_node(ri, to->id));

    sc = malloc(sizeof(AStarScore));
    sc->way = &(ri->ways[routing_index_find_node(ri, from->id)]);
    sc->g = 0;
    sc->h = distance(from->lat, from->lon, to->lat, to->lon);
    sc->f = sc->h;
    printf("Start node f = %lf\n", sc->f);

    closedset = NULL;
    openset = list_prepend(NULL, sc);
    printf("List size = %d\n", list_count(openset));

    while (openset) {
        sc = pop_min_f_from_list(&openset);
        printf("Node %d f = %lf\n", sc->way->from.id, sc->f);
        printf("List size = %d\n", list_count(openset));


    }

    /*
     closedset := the empty set                 % The set of nodes already evaluated.     
     openset := set containing the initial node % The set of tentative nodes to be evaluated.
     g_score[start] := 0                        % Distance from start along optimal path.
     h_score[start] := heuristic_estimate_of_distance(start, goal)
     f_score[start] := h_score[start]           % Estimated total distance from start to goal through y.
     while openset is not empty
         x := the node in openset having the lowest f_score[] value
         if x = goal
             return reconstruct_path(came_from,goal)
         remove x from openset
         add x to closedset
         foreach y in neighbor_nodes(x)
             if y in closedset
                 continue
             tentative_g_score := g_score[x] + dist_between(x,y)
 
             if y not in openset
                 add y to openset
 
                 tentative_is_better := true
             elseif tentative_g_score < g_score[y]
                 tentative_is_better := true
             else
                 tentative_is_better := false
             if tentative_is_better = true
                 came_from[y] := x
                 g_score[y] := tentative_g_score
                 h_score[y] := heuristic_estimate_of_distance(y, goal)
                 f_score[y] := g_score[y] + h_score[y]
                 */
 
    return res;
}

