
#ifndef WHICHWAY_H_
#define WHICHWAY_H_

typedef struct _RoutingNode RoutingNode;
typedef struct _Route Route;
typedef struct _RoutingIndex RoutingIndex;
typedef struct _RoutingWay RoutingWay;
typedef struct _RoutingTagSet RoutingTagSet;

typedef enum { highway_motorway, highway_motorway_link, highway_trunk,
    highway_trunk_link, highway_primary, highway_primary_link,
    highway_secondary, highway_secondary_link, highway_tertiary,
    highway_unclassified, highway_road, highway_residential,
    highway_living_street, highway_service, highway_track, highway_pedestrian,
    highway_raceway, highway_services, highway_bus_guideway, highway_path,
    highway_cycleway, highway_footway, highway_bridleway, highway_byway,
    highway_steps, highway_mini_roundabout, highway_stop,
    highway_traffic_signals, highway_crossing, highway_motorway_junction,
    highway_incline, highway_incline_steep, highway_ford, highway_bus_stop,
    highway_turning_circle, highway_construction, highway_proposed,
    highway_emergency_access_point, highway_speed_camera, traffic_calming_yes,
    traffic_calming_bump, traffic_calming_chicane, traffic_calming_cushion,
    traffic_calming_hump, traffic_calming_rumble_strip, traffic_calming_table,
    traffic_calming_choker, smoothness_excellent, smoothness_good,
    smoothness_intermediate, smoothness_bad, smoothness_very_bad,
    smoothness_horrible, smoothness_very_horrible, smoothness_impassable } TAG;


struct _RoutingNode {
    int id;       // OSM id
    int way;      // The index of the first way leading from this node
    double lat;
    double lon;
};

struct _Route {
    int nrof_nodes;
    double length;
    RoutingNode *nodes;
};

struct _RoutingIndex {
    int nrof_ways;
    int nrof_nodes;
    RoutingWay *ways;
    RoutingNode *nodes;
    RoutingTagSet *tagsets;
};

struct _RoutingWay {
    int from; // The index of the node this way leads from
    int next; // The index of the node this leads to
    int tagset;
};

struct _RoutingTagSet {
    int size;
    TAG tags[0];
};

Route * ww_routing_astar(RoutingIndex *ri, int from_id, int to_id);

RoutingNode * ww_find_closest_node(RoutingIndex *ri, RoutingNode **sorted_by_lat, double lat, double lon);

RoutingNode ** ww_nodes_get_sorted_by_lat(RoutingIndex *ri);

#endif /* WHICHWAY_H_ */
