
#ifndef WHICHWAY_INTERNAL_H_
#define WHICHWAY_INTERNAL_H_

#include "Whichway.h"

typedef int (*List_Compare_Cb) (const void *a, const void *b);

typedef struct _File File;
typedef struct _List List;

struct _File {
    char *file;
    int fd;
    char *content;
    int size;
};

struct _List {
    List *next;
    void *data;
};

double distance(double from_lat, double from_lon, double to_lat, double to_lon);
List * list_sorted_insert(List *list, void *data, List_Compare_Cb compare);

#endif /* WHICHWAY_INTERNAL_H_ */
