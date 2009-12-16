
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

List *
list_sorted_insert(List *list, void *data, List_Compare_Cb compare) {
    List *cn;
    List *l;

    l = malloc(sizeof(List));
    l->data = data;
    l->next = NULL;

    if (!list) {
        return l;
    }

    // Check the head of the list
    if (compare(data, list->data) <= 0) {
        l->next = list;
        return l;
    }

    // Walk the list
    cn = list;
    while (cn->next) {
        if (compare(data, cn->next->data) <= 0) {
            l->next = cn->next;
            cn->next = l;
            return list;
        }
        cn = cn->next;
    }

    // Not found, append to end
    cn->next = l;
    return list;
}

