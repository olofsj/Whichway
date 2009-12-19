
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "whichway_internal.h"

// Calculate the distance between two points on the earths surface
double distance(double from_lat, double from_lon, double to_lat, double to_lon) {
    // Earth radius
    double lambda, phi, phi_m;
    double R = 6371009;

    phi_m = (to_lat + from_lat)/360.0*M_PI;
    phi = (to_lat - from_lat)/180.0*M_PI;
    lambda = (to_lon - from_lon)/180.0*M_PI;

    return R*sqrt(phi*phi + pow(cos(phi_m)*lambda, 2));
}

List * list_sorted_insert(List *list, void *data, List_Compare_Cb compare) {
    List *cn;
    List *l;

    l = malloc(sizeof(List));
    l->data = data;
    l->next = NULL;
    l->prev = NULL;

    if (!list) {
        return l;
    }

    // Check the head of the list
    if (compare(data, list->data) <= 0) {
        l->next = list;
        list->prev = l;
        return l;
    }

    // Walk the list
    cn = list;
    while (cn->next) {
        if (compare(data, cn->next->data) <= 0) {
            l->next = cn->next;
            cn->next = l;
            l->prev = cn->next;
            l->next->prev = l;
            return list;
        }
        cn = cn->next;
    }

    // Not found, append to end
    cn->next = l;
    l->prev = cn;
    return list;
}

List * list_prepend(List *list, void *data) {
    List *l;

    l = malloc(sizeof(List));
    l->data = data;
    l->next = list;
    l->prev = NULL;

    if (list)
        list->prev = l;

    return l;
}

List * list_append(List *list, void *data) {
    List *l;
    List *ll;

    l = malloc(sizeof(List));
    l->data = data;
    l->next = NULL;
    l->prev = NULL;

    if (!list)
        return l;

    ll = list;
    while (ll) ll = ll->next;
    ll->next = l;
    l->prev = ll;

    return list;
}

List * list_find(List *list, void *data, List_Compare_Cb compare) {
    List *l;

    l = list;
    while (l) {
        if (compare(l->data, data) == 0)
            return l;
        l = l->next;
    }

    return NULL;
}

int list_count(List *list) {
    List *l;
    int count = 0;

    l = list;
    while (l) {
        count ++;
        l = l->next;
    }

    return count;
}

int routing_index_bsearch(RoutingWay* ways, int id, int low, int high) {
    int mid;

    if (high < low)
        return -1; // not found

    mid = low + ((high - low) / 2);
    if (ways[mid].from.id > id) {
        return routing_index_bsearch(ways, id, low, mid-1);
    } else if (ways[mid].from.id < id) {
        return routing_index_bsearch(ways, id, mid+1, high);
    } else {
        return mid;
    }
}

int routing_index_find_node(RoutingIndex* ri, int id) {
    return routing_index_bsearch(ri->ways, id, 0, ri->size-1);
}

