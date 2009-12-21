
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

typedef struct _WayNode WayNode;
typedef struct _Way Way;

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

char *TAG_HIGHWAY_VALUES[] = { "no", "motorway", "motorway_link", "trunk", "trunk_link", "primary", "primary_link", "secondary", "secondary_link", "tertiary", "unclassified", "road", "residential", "living_street", "service", "track", "pedestrian", "raceway", "services", "bus_guideway", "path", "cycleway", "footway", "bridleway", "byway", "steps", "mini_roundabout", "stop", "traffic_signals", "crossing", "motorway_junction", "incline", "incline_steep", "ford", "bus_stop", "turning_circle", "construction", "proposed", "emergency_access_point", "speed_camera" };
#define NROF_TAG_HIGHWAY_VALUES 40
char *TAG_TRAFFIC_CALMING_VALUES[] = { "no", "yes", "bump", "chicane", "cushion", "hump", "rumble_strip", "table", "choker" };
#define NROF_TAG_TRAFFIC_CALMING_VALUES 9
char *TAG_SMOOTHNESS_VALUES[] = { "no", "excellent", "good", "intermediate", "bad", "very_bad", "horrible", "very_horrible", "impassable" };
#define NROF_TAG_SMOOTHNESS_VALUES 9

TAG_HIGHWAY used_highways[] = { motorway, motorway_link, trunk, trunk_link, primary, primary_link, secondary, secondary_link, tertiary, unclassified, road, residential, living_street, service, track, pedestrian, services, path, cycleway, footway, bridleway, byway };
int nrof_used_highways = 22;

/* Global variables */
int depth;
int node_count;
List *node_list;
RoutingNode **nodes;
List *way_list;
RoutingIndex ri;
Way way;

int
node_sort_cb(const void *n1, const void *n2)
{
    const RoutingNode *m1 = NULL;
    const RoutingNode *m2 = NULL;

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

int
way_sort_cb(const void *n1, const void *n2)
{
    const RoutingWay *m1 = NULL;
    const RoutingWay *m2 = NULL;

    if (!n1) return(1);
    if (!n2) return(-1);

    m1 = n1;
    m2 = n2;

    if (m1->from > m2->from)
        return 1;
    if (m1->from < m2->from)
        return -1;
    return 0;
}

RoutingNode *
node_find(RoutingNode** nodes, int id, int low, int high) {
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

RoutingNode *get_node(int id) {
    return node_find(nodes, id, 0, node_count-1);
}


void
nodeparser_start(void *data, const char *el, const char **attr) {
  int i;

  if (!strcmp(el, "node")) {
      RoutingNode *node;

      node_count++;

      node = malloc(sizeof(RoutingNode));
      node->way = 0;

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
    RoutingNode *nd;

    if (!strcmp(el, "way")) {
        for (i = 0; i < nrof_used_highways; i++) {
            // Add the parsed way into the list of ways
            if (used_highways[i] == way.type) {

                if (way.oneway)
                    ri.nrof_ways = ri.nrof_ways + (way.size-1);
                else 
                    ri.nrof_ways = ri.nrof_ways + 2*(way.size-1);

                // Add each segment of the way into the index
                cn = way.start;
                while (cn->next) {
                    RoutingWay *w = malloc(sizeof(RoutingWay));

                    w->type = way.type;
                    w->from = cn->id;
                    w->next = cn->next->id;
                    way_list = list_sorted_insert(way_list, w, way_sort_cb);

                    // Mark nodes as used
                    nd = get_node(cn->id);
                    if (nd)
                        nd->way++;
                    nd = get_node(cn->next->id);
                    if (nd)
                        nd->way++;

                    // if not oneway, add the reverse way as well
                    if (!way.oneway) {
                        RoutingWay *w = malloc(sizeof(RoutingWay));

                        w->type = way.type;
                        w->from = cn->next->id;
                        w->next = cn->id;
                        way_list = list_sorted_insert(way_list, w, way_sort_cb);

                        // Mark nodes as used
                        nd = get_node(cn->id);
                        if (nd)
                            nd->way++;
                        nd = get_node(cn->next->id);
                        if (nd)
                            nd->way++;
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
    char *filename, *outfilename;
    int i;
    int done;
    int len;
    List *cn, *l;
    RoutingWay *w;
    RoutingNode *nd;
    
    
    printf("Whichway Create Index\n");

    if (argc > 2) {
        filename = argv[1];
        outfilename = argv[2];
    } else {
        printf("Input and output files must be specified.\n");
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
    nodes = malloc(node_count * sizeof(RoutingNode *));
    i = 0;
    cn = node_list;
    while (cn) {
        nodes[i++] = (RoutingNode *)(cn->data);
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
    ri.nrof_ways = 0;
    ri.nrof_nodes = 0;
    ri.ways = NULL;
    ri.nodes = NULL;
    way.size = -1;
    way_list = NULL;

    /* Parse the XML document */
    if (! XML_Parse(wayparser, osmfile.content, osmfile.size, done)) {
        fprintf(stderr, "Parse error at line %d:\n%s\n",
                (int)XML_GetCurrentLineNumber(wayparser),
                XML_ErrorString(XML_GetErrorCode(wayparser)));
        exit(-1);
    }

    // Postprocess the routing index, ordering the ways and getting the 
    // index of the target way

    // Create a sorted list of nodes in the routing index
    ri.nrof_nodes = 0;
    l = node_list;
    while (l) {
        nd = l->data;
        if (nd->way > 0) {
            ri.nrof_nodes++;
        }
        l = l->next;
    }

    ri.nodes = malloc(ri.nrof_nodes * sizeof(RoutingNode));

    i = 0;
    l = node_list;
    while (l) {
        nd = l->data;
        if (nd->way > 0) {
            nd->way = -1;
            memcpy(&(ri.nodes[i]), nd, sizeof(RoutingNode));
            i++;
        }
        l = l->next;
    }

    // Update the ways in the routing index with this sorted list of nodes
    ri.ways = malloc(ri.nrof_ways * sizeof(RoutingWay));
    l = way_list;
    i = 0;
    while (l) {
        w = l->data;
        w->from = routing_index_find_node(&ri, w->from);
        w->next = routing_index_find_node(&ri, w->next);
        memcpy(&(ri.ways[i]), w, sizeof(RoutingWay));
        l = l->next; i++;
    }

    // Update the nodes in the index with the index of the first way 
    // leading from that node
    for (i = 0; i < ri.nrof_ways; i++) {
        if (ri.nodes[ri.ways[i].from].way == -1) {
            ri.nodes[ri.ways[i].from].way = i;
        }
    }

    // Write the routing index to disk
    FILE *fp;
    fp = fopen(outfilename, "w");
    if (!fp) {
        fprintf(stderr, "Can't open output file for writing.\n");
        exit(-1);
    }
    fwrite(&(ri.nrof_ways), sizeof(int), 1, fp);
    fwrite(&(ri.nrof_nodes), sizeof(int), 1, fp);
    fwrite(ri.ways, sizeof(RoutingWay), ri.nrof_ways, fp);
    fwrite(ri.nodes, sizeof(RoutingNode), ri.nrof_nodes, fp);
    fclose(fp);

    printf("Number of nodes: %d\n", ri.nrof_nodes);
    printf("Number of ways: %d\n", ri.nrof_ways);

    for (i = 0; i < 5; i++) {
        printf("Node %d: %d (%lf %lf) %d\n", i, ri.nodes[i].id, ri.nodes[i].lat, ri.nodes[i].lon, ri.nodes[i].way);
        printf("Way %d: %d (%lf %lf) - %d (%lf %lf): %s\n", i, 
                ri.ways[i].from, ri.nodes[ri.ways[i].from].lat, ri.nodes[ri.ways[i].from].lon, 
                ri.ways[i].next, ri.nodes[ri.ways[i].next].lat, ri.nodes[ri.ways[i].next].lon, 
                TAG_HIGHWAY_VALUES[ri.ways[i].type]);
        int k = ri.nrof_ways - 1 - i;
        printf("Way %d: %d (%lf %lf) - %d (%lf %lf): %s\n", k, 
                ri.ways[k].from, ri.nodes[ri.ways[k].from].lat, ri.nodes[ri.ways[k].from].lon, 
                ri.ways[k].next, ri.nodes[ri.ways[k].next].lat, ri.nodes[ri.ways[k].next].lon, 
                TAG_HIGHWAY_VALUES[ri.ways[k].type]);
        k = ri.nrof_nodes - 1 - i;
        printf("Node %d: %d (%lf %lf)\n", k, ri.nodes[k].id, ri.nodes[k].lat, ri.nodes[k].lon, ri.nodes[k].way);
    }

    //for (i = 0; i < 10; i++)
        //printf("Node %d: %d (%lf %lf)\n", i, nodes[i]->id, nodes[i]->lat, nodes[i]->lon);
}

