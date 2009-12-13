
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <expat.h>
#include "whichway_internal.h"

typedef struct _File File;
typedef struct _Node Node;
typedef struct _Way Way;

struct _File {
    char *file;
    int fd;
    char *content;
    int size;
};

struct _Node {
    int id;
    Node *next;
    Node *prev;
};

struct _Way {
    Node *start;
    Node *end;
    int oneway;
    int size;
};

/* Global variables */
int depth;
int node_count;
int way_count;
RoutingIndex ri;
Way way;

void
start(void *data, const char *el, const char **attr) {
  int i;

  if (!strcmp(el, "node")) {
      node_count++;
  }
  else if (!strcmp(el, "way")) {
      way_count++;

      /* Check all the attributes for this node */
      for (i = 0; attr[i]; i += 2) {
          way.size = 0;
          way.start = NULL;
          way.end = NULL;
          way.oneway = 0;
      }
  }
  else if (!strcmp(el, "tag") && way.size != -1) {
      if (!strcmp(attr[0], "k") && !strcmp(attr[1], "oneway") &&
              !strcmp(attr[2], "v") && 
              (!strcmp(attr[3], "yes") || !strcmp(attr[3], "true")) ) {
          // Current way is oneway
          way.oneway = 1;
      }
  }
  else if (!strcmp(el, "nd") && way.size != -1) {
      // Add a node to the current way
      way.size++;
      if (!way.start) {
          way.start = malloc(sizeof(Node));
          way.end = way.start;
          way.start->prev = NULL;
          way.start->next = NULL;
      } else {
          way.end->next = malloc(sizeof(Node));
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
end(void *data, const char *el) {
    int i, index;
    Node *cn;

    if (!strcmp(el, "way")) {
        if (way.start->id == 302797085) {
            printf("way.size = %d\n", way.size);
        }
        // Add the parsed way into the routing index
        index = ri.size;

        if (way.oneway)
            ri.size = ri.size + (way.size-1);
        else 
            ri.size = ri.size + 2*(way.size-1);
        ri.ways = realloc(ri.ways, ri.size * sizeof(RoutingWay));

        cn = way.start;
        while (cn->next) {
            ri.ways[index].from.id = cn->id;
            ri.ways[index].to.id = cn->next->id;
            index++;
            if (!way.oneway) {
                ri.ways[index].from.id = cn->next->id;
                ri.ways[index].to.id = cn->id;
                index++;
            }
            cn = cn->next;
        }

        // Free the nodes
        cn = way.start;
        while (cn) {
            Node *next;
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


    /* Set up the XML parser */
    XML_Parser p = XML_ParserCreate(NULL);
    if (!p) {
        fprintf(stderr, "Couldn't allocate memory for parser\n");
        exit(-1);
    }

    XML_SetElementHandler(p, start, end);

    int done;
    int len;

    depth = 0;
    node_count = 0;
    way_count = 0;
    ri.size = 0;
    ri.ways = NULL;
    way.size = -1;

    /* Parse the XML document */
    if (! XML_Parse(p, osmfile.content, osmfile.size, done)) {
        fprintf(stderr, "Parse error at line %d:\n%s\n",
                (int)XML_GetCurrentLineNumber(p),
                XML_ErrorString(XML_GetErrorCode(p)));
        exit(-1);
    }

    printf("Number of nodes: %d\n", ri.size);
    printf("Number of ways: %d\n", way_count);
    for (i = 0; i < 10; i++)
        printf("Way %d: (%d %d)\n", i, ri.ways[i].from.id, ri.ways[i].to.id);
}

