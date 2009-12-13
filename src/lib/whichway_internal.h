
typedef struct _RoutingIndex RoutingIndex;
typedef struct _RoutingNode RoutingNode;
typedef struct _RoutingWay RoutingWay;

struct _RoutingIndex {
    int size;
    RoutingWay *ways;
};

struct _RoutingNode {
    int id;
    double lat;
    double lon;
};

struct _RoutingWay {
    RoutingNode from;
    RoutingNode to;
};

