#include <Python.h>
#include "structmember.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <time.h>
#include "whichway_internal.h"

char *tag_keys[] = TAG_KEYS;
char *tag_values[] = TAG_VALUES;

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    RoutingIndex *routingindex;
    RoutingNode **nodes_sorted_by_lat;
    RoutingProfile *profile;
    RoutingProfile **profiles;
    File *routingfile;
} whichway_Router;

static PyObject *set_profile(whichway_Router *self, PyObject *args) {
    char *name;
    int nrof_profiles, i;
    RoutingProfile *p;

    // Get arguments
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;

    for (i = 0, p = self->profiles[0]; p; p = self->profiles[++i]) {
        if ( !strcmp(p->name, name) ) {
            // Profile exists, select this one
            self->profile = p;
            break;
        }
    }
    nrof_profiles = i;

    if (!p) {
        // No such profile, add new one
        self->profiles = realloc(self->profiles, (nrof_profiles+2)*sizeof(RoutingProfile *));
        self->profiles[nrof_profiles] = malloc(sizeof(RoutingProfile));
        self->profiles[nrof_profiles+1] = NULL;
        self->profile = self->profiles[nrof_profiles];

        self->profile->name = strdup(name);
        self->profile->max_route_length = 10000.0;
        for (i = 0; i < NROF_TAGS; i++) {
            self->profile->penalty[i] = 1.0;
        }
    }

    return Py_True;
}

static PyObject *set_max_route_length(whichway_Router *self, PyObject *args) {
    double max_route_length;

    // Get arguments
    if (!PyArg_ParseTuple(args, "d", &max_route_length))
        return NULL;
    
    self->profile->max_route_length = max_route_length;

    return Py_True;
}

static PyObject *set_penalty(whichway_Router *self, PyObject *args) {
    char *key, *value;
    int i;
    double penalty;

    // Get arguments
    if (!PyArg_ParseTuple(args, "ssd", &key, &value, &penalty))
        return NULL;
    
    for (i = 0; i < NROF_TAGS; i++) {
        if ( !strcmp(key, tag_keys[i]) && !strcmp(value, tag_values[i])) {
            self->profile->penalty[i] = penalty;
            return Py_True;
        }
    }

    return Py_False;
}

static PyObject *find_nodes(whichway_Router *self, PyObject *args) {
    double lat, lon, radius;
    int *nodes, k;
    PyObject *list;

    // Get arguments
    if (!PyArg_ParseTuple(args, "(dd)d", &lat, &lon, &radius))
        return NULL;

    struct timeval start;
    struct timeval end;
    double t;
    gettimeofday(&start, NULL);

    // Find all nodes within radius of (lat, lon)
    nodes = ww_find_nodes(self->routingindex, self->nodes_sorted_by_lat, lat, lon, radius);

    gettimeofday(&end, NULL);
    t = ((double)end.tv_sec + (((double)end.tv_usec) / 1000000))
        - ((double)start.tv_sec + (((double)start.tv_usec) / 1000000));
    printf("Time to find nodes: %lf s.\n", t);

    if (!nodes)
        return NULL;

    // Build return value
    list = PyList_New(0);
    for (k = 0; nodes[k] >= 0; k++) {
        PyList_Append(list, Py_BuildValue("i", nodes[k]));
    }
    free(nodes);

    return list;
}

static PyObject *find_closest_node(whichway_Router *self, PyObject *args) {
    double lat, lon;
    RoutingNode *closest;

    // Get arguments
    if (!PyArg_ParseTuple(args, "(dd)", &lat, &lon))
        return NULL;

    struct timeval start;
    struct timeval end;
    double t;
    gettimeofday(&start, NULL);

    // Find closest node
    closest = ww_find_closest_node(self->routingindex, self->nodes_sorted_by_lat, lat, lon);

    gettimeofday(&end, NULL);
    t = ((double)end.tv_sec + (((double)end.tv_usec) / 1000000))
        - ((double)start.tv_sec + (((double)start.tv_usec) / 1000000));
    printf("Time to find closest node: %lf s.\n", t);

    if (!closest)
        return NULL;

    return Py_BuildValue("i", closest->id);
}

static PyObject *find_route(whichway_Router *self, PyObject *args) {
    int *from_ids, k, from_size;
    double to_lat, to_lon, tolerance;
    Route *route;
    PyObject *id_list, *lat_list, *lon_list, *dict, *from_list;

    // Get arguments
    if (!PyArg_ParseTuple(args, "O(dd)d", &from_list, &to_lat, &to_lon, &tolerance))
        return NULL;

    if (!PyList_Check(from_list))
        return NULL;

    // Build a C array from the Python list object
    from_size = PyList_Size(from_list);
    from_ids = malloc((from_size +1) * sizeof(int));
    for (k = 0; k < from_size; k++) {
        PyObject *list_item = PyList_GetItem(from_list, k);
        if (!PyInt_Check(list_item)) {
            free(from_ids);
            return NULL;
        }
        from_ids[k] = PyInt_AsLong(list_item);
    }
    from_ids[from_size] = -1;

    struct timeval start;
    struct timeval end;
    double t;
    gettimeofday(&start, NULL);

    // Calculate route
    route = ww_routing_astar(self->routingindex, self->profile, from_ids, to_lat, to_lon, tolerance);

    free(from_ids);

    gettimeofday(&end, NULL);
    t = ((double)end.tv_sec + (((double)end.tv_usec) / 1000000))
        - ((double)start.tv_sec + (((double)start.tv_usec) / 1000000));
    printf("Time to find route: %lf s.\n", t);

    if (!route)
        return Py_BuildValue("{s:[],s:[],s:[],s:d}", "ids", "lats","lons", "length", 0.0);

    // Build return value
    dict = PyDict_New();
    id_list = PyList_New(0);
    lat_list = PyList_New(0);
    lon_list = PyList_New(0);
    for (k = 0; k < route->nrof_nodes; k++) {
        PyList_Append(id_list, Py_BuildValue("i", route->nodes[k].id));
        PyList_Append(lat_list, Py_BuildValue("d", route->nodes[k].lat));
        PyList_Append(lon_list, Py_BuildValue("d", route->nodes[k].lon));
    }
    PyDict_SetItemString(dict, "ids", id_list);
    PyDict_SetItemString(dict, "lats", lat_list);
    PyDict_SetItemString(dict, "lons", lon_list);
    PyDict_SetItemString(dict, "length", Py_BuildValue("d", route->length));

    if (route->nodes) free(route->nodes);
    free(route);

    return dict;
}

static int whichway_router_init(whichway_Router *self, PyObject *args, PyObject *kwds)
{
    struct stat st;
    char *filename;
    int i;

    // Get arguments
    if (!PyArg_ParseTuple(args, "s", &filename)) {
        PyErr_SetString(PyExc_TypeError, "A routing index file must be specified");
        return -1;
    }

    /* Open file descriptor and stat the file to get size */
    self->routingfile = malloc(sizeof(File));
    self->routingfile->file = strdup(filename);
    self->routingfile->fd = -1;
    self->routingfile->size = 0;
    self->routingfile->fd = open(self->routingfile->file, O_RDONLY);
    if (self->routingfile->fd < 0) {
        PyErr_SetString(PyExc_TypeError, "Can't open file");
        free(self->routingfile);
        self->routingfile = NULL;
        return -1;
    }
    if (fstat(self->routingfile->fd, &st) < 0) {
        PyErr_SetString(PyExc_TypeError, "Not a valid file");
        close(self->routingfile->fd);
        free(self->routingfile);
        self->routingfile = NULL;
        return -1;
    }
    self->routingfile->size = st.st_size;

    /* mmap file contents */
    self->routingfile->content = mmap(NULL, self->routingfile->size, 
            PROT_READ, MAP_SHARED, self->routingfile->fd, 0);
    if ((self->routingfile->content == MAP_FAILED) || (self->routingfile->content  == NULL)) {
        PyErr_SetString(PyExc_TypeError, "Can't read file to memory");
        close(self->routingfile->fd);
        free(self->routingfile);
        self->routingfile = NULL;
        return -1;
    }

    /* Set up the pointers */
    self->routingindex = malloc(sizeof(RoutingIndex));
    int *p;
    p = (int *)self->routingfile->content;
    self->routingindex->nrof_ways = *p++;

    self->routingindex->nrof_nodes = *p++;

    RoutingWay *pw = (RoutingWay *)p;
    self->routingindex->ways = pw;
    pw = pw + self->routingindex->nrof_ways;

    self->routingindex->nodes = (RoutingNode *)pw;

    self->routingindex->tagsets = (RoutingTagSet *)(self->routingindex->nodes
            + self->routingindex->nrof_nodes);

    self->nodes_sorted_by_lat = ww_nodes_get_sorted_by_lat(self->routingindex);

    /* Set up a default profile */
    self->profiles = malloc(2*sizeof(RoutingProfile *));
    self->profiles[0] = malloc(sizeof(RoutingProfile));
    self->profiles[1] = NULL;
    self->profile = self->profiles[0];
    self->profile->name = strdup("default");
    self->profile->max_route_length = 10000.0;
    for (i = 0; i < NROF_TAGS; i++) {
        self->profile->penalty[i] = 1.0;
    }

	return 0;
}

static PyObject *whichway_router_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	whichway_Router *self;

	self = (whichway_Router *)type->tp_alloc(type, 0);
	if (self != NULL) {
        self->routingfile =  NULL;
        self->routingindex =  NULL;
    }
	
	return (PyObject *)self;
}

static void whichway_router_dealloc(whichway_Router *self)
{
    if (self->routingfile) {
        munmap((void *)self->routingfile->content, self->routingfile->size);
        close(self->routingfile->fd);
        free(self->routingfile);
    }

    if (self->profile) 
        free(self->profile);

    if (self->routingindex)
        free(self->routingindex);

    if (self->nodes_sorted_by_lat)
        free(self->nodes_sorted_by_lat);

    self->ob_type->tp_free((PyObject*)self);
}

static PyMethodDef whichway_router_methods[] = {
    {"set_profile", (PyCFunction)set_profile, METH_VARARGS, "Select a profile"},
    {"set_max_route_length", (PyCFunction)set_max_route_length, METH_VARARGS, "Set the maximum length of routes, ie give up searching if no shorter route is found"},
    {"set_penalty", (PyCFunction)set_penalty, METH_VARARGS, "Set the penalty for a given tag"},
    {"find_nodes", (PyCFunction)find_nodes, METH_VARARGS, "Find the node closest to a given coordinate"},
    {"find_closest_node", (PyCFunction)find_closest_node, METH_VARARGS, "Find the node closest to a given coordinate"},
    {"find_route", (PyCFunction)find_route, METH_VARARGS, "Find the best route between two nodes"},
    {NULL, NULL, 0, NULL}
};

static PyTypeObject whichway_RouterType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size */
    "whichway.Router",         /* tp_name */
    sizeof(whichway_Router),   /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)whichway_router_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "Whichway objects",        /* tp_doc */
    0,		                   /* tp_traverse */
    0,		                   /* tp_clear */
    0,		                   /* tp_richcompare */
    0,		                   /* tp_weaklistoffset */
    0,		                   /* tp_iter */
    0,		                   /* tp_iternext */
    whichway_router_methods,   /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)whichway_router_init, /* tp_init */
    0,                         /* tp_alloc */
    whichway_router_new,       /* tp_new */
};

PyMODINIT_FUNC
initwhichway(void)
{
	PyObject* m;

    whichway_RouterType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&whichway_RouterType) < 0)
        return;

	m =  Py_InitModule3("whichway", whichway_router_methods, "A module for calculating routes.");

    Py_INCREF(&whichway_RouterType);
    PyModule_AddObject(m, "Router", (PyObject *)&whichway_RouterType);

}
