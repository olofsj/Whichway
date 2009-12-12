
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <expat.h>

typedef struct _File File;

struct _File {
    char *file;
    int fd;
    char *content;
    int size;
};

int depth;
int node_count;
int way_count;

void
start(void *data, const char *el, const char **attr) {
  int i;

  if (!strcmp(el, "node")) 
  {
      node_count++;
  }
  else if (!strcmp(el, "way")) 
  {
      way_count++;
  }

  depth++;
}

void
end(void *data, const char *el) {
  depth--;
}


int
main(int argc, char **argv)
{
    File osmfile;
    struct stat st;
    char *filename;
    
    
    printf("Whichway Create Index\n");

    if (argc > 1)
        filename = argv[1];
    else
    {
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
    if (fstat(osmfile.fd, &st) < 0)
    {
        close(osmfile.fd);
        return 0;
    }
    osmfile.size = st.st_size;

    /* mmap file contents */
    osmfile.content = mmap(NULL, osmfile.size, PROT_READ, MAP_SHARED, osmfile.fd, 0);
    if ((osmfile.content == MAP_FAILED) || (osmfile.content  == NULL))
    {
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

    /* Parse the XML document */
    if (! XML_Parse(p, osmfile.content, osmfile.size, done)) {
        fprintf(stderr, "Parse error at line %d:\n%s\n",
                (int)XML_GetCurrentLineNumber(p),
                XML_ErrorString(XML_GetErrorCode(p)));
        exit(-1);
    }

    printf("Number of nodes: %d\n", node_count);
    printf("Number of ways: %d\n", way_count);
}

