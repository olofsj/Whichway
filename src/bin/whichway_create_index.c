
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <expat.h>
#include <math.h>
#include "whichway_internal.h"

typedef int (*List_Compare_Cb) (const void *a, const void *b);

typedef struct _File File;
typedef struct _WayNode WayNode;
typedef struct _Way Way;
typedef struct _List List;
typedef struct _Node Node;

struct _File {
    char *file;
    int fd;
    char *content;
    int size;
};

struct _WayNode {
    int id;
    WayNode *next;
    WayNode *prev;
};

struct _Way {
    WayNode *start;
    WayNode *end;
    int oneway;
    int size;
    TAG_HIGHWAY type;
};

struct _List {
    List *next;
    void *data;
};

struct _Node {
    int id;
    double lat;
    double lon;
    int ways;
};

TAG_HIGHWAY used_highways[] = {secondary, secondary_link, tertiary, unclassified, road, residential, living_street, service, track, pedestrian, path, cycleway, footway, byway};
int nrof_used_highways = 14;

/* Global variables */
int depth;
int node_count;
List *node_list;
Node **nodes;
Node **nodes_ways_from;
Node **nodes_ways_to;
int way_count;
RoutingIndex ri;
Way way;

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

int
node_sort_cb(const void *n1, const void *n2)
{
    const Node *m1 = NULL;
    const Node *m2 = NULL;

    if (!n1) return(1);
    if (!n2) return(-1);

    m1 = n1;
    m2 = n2;

    if (m1->id > m2->id)
        return 1;
    if (m1->id < m2->id)
        return -1;
    return 0;
}

List *
list_sorted_insert(List *list, void *data, List_Compare_Cb compare) {
    List *cn;
    List *l;

    l = malloc(sizeof(List));
    l->data = data;

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
        //if (el->node->id < cn->next->node->id) {
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

Node *
node_find(Node** nodes, int id, int low, int high) {
    int mid;

    if (high < low)
        return NULL; // not found

    mid = low + ((high - low) / 2);
    if (nodes[mid]->id > id) {
        return node_find(nodes, id, low, mid-1);
    } else if (nodes[mid]->id < id) {
        return node_find(nodes, id, mid+1, high);
    } else {
        return nodes[mid];
    }
}

Node *get_node(int id) {
    return node_find(nodes, id, 0, node_count-1);
}


void
nodeparser_start(void *data, const char *el, const char **attr) {
  int i;

  if (!strcmp(el, "node")) {
      Node *node;

      node_count++;

      node = malloc(sizeof(Node));
      node->ways = 0;

      /* Check all the attributes for this node */
      for (i = 0; attr[i]; i += 2) {
          if (!strcmp(attr[i], "id")) 
              sscanf(attr[i+1], "%d", &(node->id));
          if (!strcmp(attr[i], "lat")) 
              sscanf(attr[i+1], "%lf", &(node->lat));
          if (!strcmp(attr[i], "lon")) 
              sscanf(attr[i+1], "%lf", &(node->lon));
      }

      node_list = list_sorted_insert(node_list, node, node_sort_cb);
  }

  depth++;
}

void
nodeparser_end(void *data, const char *el) {

    depth--;
}

void
wayparser_start(void *data, const char *el, const char **attr) {
  int i;

  if (!strcmp(el, "way")) {
      way.size = 0;
      way.start = NULL;
      way.end = NULL;
      way.oneway = 0;
      way.type = no_highway;
  }
  else if (!strcmp(el, "tag") && way.size != -1) {
      if (!strcmp(attr[0], "k") && !strcmp(attr[1], "oneway") &&
              !strcmp(attr[2], "v") && 
              (!strcmp(attr[3], "yes") || !strcmp(attr[3], "true")) ) {
          // Current way is oneway
          way.oneway = 1;
      }
      if (!strcmp(attr[0], "k") && !strcmp(attr[1], "highway") &&
              !strcmp(attr[2], "v")) {
          // Highway tag
          for (i = 0; i < NROF_TAG_HIGHWAY_VALUES; i++) {
              if (!strcmp(attr[3], TAG_HIGHWAY_VALUES[i])) {
                  way.type = i;
              }
          }
      }
  }
  else if (!strcmp(el, "nd") && way.size != -1) {
      // Add a node to the current way
      way.size++;
      if (!way.start) {
          way.start = malloc(sizeof(WayNode));
          way.end = way.start;
          way.start->prev = NULL;
          way.start->next = NULL;
      } else {
          way.end->next = malloc(sizeof(WayNode));
          way.end->next->next = NULL;
          way.end->next->prev = way.end;
          way.end = way.end->next;
      }

      /* Check all the attributes for this node */
      for (i = 0; attr[i]; i += 2) {
          if (!strcmp(attr[i], "ref")) 
              sscanf(attr[i+1], "%d", &(way.end->id));
      }
  }

  depth++;
}

void
wayparser_end(void *data, const char *el) {
    int i, index;
    WayNode *cn;
    Node *nd;

    if (!strcmp(el, "way")) {
        for (i = 0; i < nrof_used_highways; i++) {
            if (used_highways[i] == way.type) {
                way_count++;

                // Add the parsed way into the routing index
                index = ri.size;

                if (way.oneway)
                    ri.size = ri.size + (way.size-1);
                else 
                    ri.size = ri.size + 2*(way.size-1);
                ri.ways = realloc(ri.ways, ri.size * sizeof(RoutingWay));

                cn = way.start;
                while (cn->next) {
                    ri.ways[index].type = way.type;
                    ri.ways[index].from.id = cn->id;
                    ri.ways[index].to.id = cn->next->id;

                    nd = get_node(ri.ways[index].from.id);
                    if (nd) {
                        ri.ways[index].from.lat = nd->lat;
                        ri.ways[index].from.lon = nd->lon;
                    }
                    // Store pointer for easy access later
                    nodes_ways_from = realloc(nodes_ways_from, (index+1)*sizeof(Node *));
                    nodes_ways_from[index] = nd;
                    nd->ways++;

                    nd = get_node(ri.ways[index].to.id);
                    if (nd) {
                        ri.ways[index].to.lat = nd->lat;
                        ri.ways[index].to.lon = nd->lon;
                    }
                    // Store pointer for easy access later
                    nodes_ways_to = realloc(nodes_ways_to, (index+1)*sizeof(Node *));
                    nodes_ways_to[index] = nd;
                    nd->ways++;

                    ri.ways[index].length = distance(
                            ri.ways[index].from.lat,
                            ri.ways[index].from.lon,
                            ri.ways[index].to.lat,
                            ri.ways[index].to.lon);

                    index++;

                    if (!way.oneway) {
                        ri.ways[index].type = way.type;
                        ri.ways[index].from.id = cn->next->id;
                        ri.ways[index].to.id = cn->id;

                        nd = get_node(ri.ways[index].from.id);
                        if (nd) {
                            ri.ways[index].from.lat = nd->lat;
                            ri.ways[index].from.lon = nd->lon;
                        }
                        // Store pointer for easy access later
                        nodes_ways_from = realloc(nodes_ways_from, (index+1)*sizeof(Node *));
                        nodes_ways_from[index] = nd;
                        nd->ways++;

                        nd = get_node(ri.ways[index].to.id);
                        if (nd) {
                            ri.ways[index].to.lat = nd->lat;
                            ri.ways[index].to.lon = nd->lon;
                        }
                        // Store pointer for easy access later
                        nodes_ways_to = realloc(nodes_ways_to, (index+1)*sizeof(Node *));
                        nodes_ways_to[index] = nd;
                        nd->ways++;

                        ri.ways[index].length = ri.ways[index-1].length;
                        index++;
                    }

                    cn = cn->next;
                }
            }
        }

        // Free the nodes
        cn = way.start;
        while (cn) {
            WayNode *next;
            next = cn->next;
            free(cn);
            cn = next;
        }
        way.size = -1;
    }

    depth--;
}


int
main(int argc, char **argv)
{
    File osmfile;
    struct stat st;
    char *filename;
    int i;
    int done;
    int len;
    List *cn;
    
    
    printf("Whichway Create Index\n");

    if (argc > 1)
        filename = argv[1];
    else {
        printf("No file specified.\n");
        return 0;
    }


    osmfile.file = strdup(filename);
    osmfile.fd = -1;
    osmfile.size = 0;

    /* Open file descriptor and stat the file to get size */
    osmfile.fd = open(osmfile.file, O_RDONLY);
    if (osmfile.fd < 0)
        return 0;
    if (fstat(osmfile.fd, &st) < 0) {
        close(osmfile.fd);
        return 0;
    }
    osmfile.size = st.st_size;

    /* mmap file contents */
    osmfile.content = mmap(NULL, osmfile.size, PROT_READ, MAP_SHARED, osmfile.fd, 0);
    if ((osmfile.content == MAP_FAILED) || (osmfile.content  == NULL)) {
        close(osmfile.fd);
        return 0;
    }

    printf("filesize: %d\n", osmfile.size);


    // Set up an XML parser for the nodes
    XML_Parser nodeparser = XML_ParserCreate(NULL);
    if (!nodeparser) {
        fprintf(stderr, "Couldn't allocate memory for parser\n");
        exit(-1);
    }

    XML_SetElementHandler(nodeparser, nodeparser_start, nodeparser_end);

    depth = 0;
    node_count = 0;
    node_list = NULL;

    /* Parse the XML document */
    if (! XML_Parse(nodeparser, osmfile.content, osmfile.size, done)) {
        fprintf(stderr, "Parse error at line %d:\n%s\n",
                (int)XML_GetCurrentLineNumber(nodeparser),
                XML_ErrorString(XML_GetErrorCode(nodeparser)));
        exit(-1);
    }

    // Create an indexed sorted list of nodes
    nodes = malloc(node_count * sizeof(Node *));
    i = 0;
    cn = node_list;
    while (cn) {
        nodes[i++] = (Node *)(cn->data);
        cn = cn->next;
    }

    // Set up an XML parser for the ways
    XML_Parser wayparser = XML_ParserCreate(NULL);
    if (!wayparser) {
        fprintf(stderr, "Couldn't allocate memory for parser\n");
        exit(-1);
    }

    XML_SetElementHandler(wayparser, wayparser_start, wayparser_end);

    depth = 0;
    way_count = 0;
    ri.size = 0;
    ri.ways = NULL;
    way.size = -1;
    nodes_ways_from = NULL;
    nodes_ways_to = NULL;

    /* Parse the XML document */
    if (! XML_Parse(wayparser, osmfile.content, osmfile.size, done)) {
        fprintf(stderr, "Parse error at line %d:\n%s\n",
                (int)XML_GetCurrentLineNumber(wayparser),
                XML_ErrorString(XML_GetErrorCode(wayparser)));
        exit(-1);
    }

    // Postprocess the routing index, ordering the ways and getting the 
    // index of the target way

    


    printf("Number of nodes: %d\n", node_count);
    printf("Number of ways: %d\n", way_count);
    for (i = 0; i < 10; i++)
        printf("Way %d: %d (%lf %lf) - %d (%lf %lf) %lf: %s\n", i, 
                ri.ways[i].from.id, ri.ways[i].from.lat, ri.ways[i].from.lon, 
                ri.ways[i].to.id, ri.ways[i].to.lat, ri.ways[i].to.lon,
                ri.ways[i].length, TAG_HIGHWAY_VALUES[ri.ways[i].type]);
    //for (i = 0; i < 10; i++)
        //printf("Node %d: %d (%lf %lf)\n", i, nodes[i]->id, nodes[i]->lat, nodes[i]->lon);
}

