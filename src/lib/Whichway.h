
#ifndef WHICHWAY_H_
#define WHICHWAY_H_

typedef struct _RoutingNode RoutingNode;
typedef struct _Route Route;
typedef struct _RoutingIndex RoutingIndex;
typedef struct _RoutingWay RoutingWay;

typedef enum { no_highway, motorway, motorway_link, trunk, trunk_link, primary, primary_link, secondary, secondary_link, tertiary, unclassified, road, residential, living_street, service, track, pedestrian, raceway, services, bus_guideway, path, cycleway, footway, bridleway, byway, steps, mini_roundabout, stop, traffic_signals, crossing, motorway_junction, incline, incline_steep, ford, bus_stop, turning_circle, construction, proposed, emergency_access_point, speed_camera } TAG_HIGHWAY;

typedef enum { no_traffic_calming, yes, bump, chicane, cushion, hump, rumble_strip, table, choker } TAG_TRAFFIC_CALMING;

typedef enum { no_smoothness, excellent, good, intermediate, bad, very_bad, horrible, very_horrible, impassable } TAG_SMOOTHNESS;


struct _RoutingNode {
    int id;
    double lat;
    double lon;
};

struct _Route {
    int nrof_nodes;
    double length;
    RoutingNode *nodes;
};

struct _RoutingIndex {
    int size;
    RoutingWay *ways;
};

struct _RoutingWay {
    RoutingNode from;
    RoutingNode to;
    TAG_HIGHWAY type;
    double length;
    int next;
};


Route * ww_routing_astar(RoutingIndex *ri, int from_id, int to_id);

#endif /* WHICHWAY_H_ */
