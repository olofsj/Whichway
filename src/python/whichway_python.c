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
#include "whichway_internal.h"

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
    RoutingIndex *routingindex;
    File *routingfile;
} whichway_Router;

static PyObject *find_route(whichway_Router *self, PyObject *args) {
    int from_id, to_id;
    int k;
    Route *route;
    PyObject *list;

    // Get arguments
    if (!PyArg_ParseTuple(args, "(ii)", &from_id, &to_id))
        return NULL;

    // Calculate route
    route = ww_routing_astar(self->routingindex, from_id, to_id);

    // Build return value
    list = PyList_New(0);
    for (k = 0; k < route->nrof_nodes; k++) {
        PyList_Append(list, Py_BuildValue("i", route->nodes[k].id));
    }

    return list;
}

static int whichway_router_init(whichway_Router *self, PyObject *args, PyObject *kwds)
{
    struct stat st;
    char *filename;

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

    if (self->routingindex)
        free(self->routingindex);

    self->ob_type->tp_free((PyObject*)self);
}

static PyMethodDef whichway_router_methods[] = {
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
