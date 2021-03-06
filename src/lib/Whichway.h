
#ifndef WHICHWAY_H_
#define WHICHWAY_H_

typedef struct _RoutingNode RoutingNode;
typedef struct _Route Route;
typedef struct _RoutingIndex RoutingIndex;
typedef struct _RoutingWay RoutingWay;
typedef struct _RoutingTagSet RoutingTagSet;
typedef struct _RoutingProfile RoutingProfile;

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
#define TAG_KEYS { "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "highway", "traffic_calming", "traffic_calming", "traffic_calming", "traffic_calming", "traffic_calming", "traffic_calming", "traffic_calming", "traffic_calming", "smoothness", "smoothness", "smoothness", "smoothness", "smoothness", "smoothness", "smoothness", "smoothness" }
#define TAG_VALUES { "motorway", "motorway_link", "trunk", "trunk_link", "primary", "primary_link", "secondary", "secondary_link", "tertiary", "unclassified", "road", "residential", "living_street", "service", "track", "pedestrian", "raceway", "services", "bus_guideway", "path", "cycleway", "footway", "bridleway", "byway", "steps", "mini_roundabout", "stop", "traffic_signals", "crossing", "motorway_junction", "incline", "incline_steep", "ford", "bus_stop", "turning_circle", "construction", "proposed", "emergency_access_point", "speed_camera", "yes", "bump", "chicane", "cushion", "hump", "rumble_strip", "table", "choker", "excellent", "good", "intermediate", "bad", "very_bad", "horrible", "very_horrible", "impassable" }
#define NROF_TAGS 55


struct _RoutingNode {
    unsigned int id;       // OSM id
    struct {
        unsigned int start; // The index of the first way leading from this node
        unsigned int end;   // The index of the first way not belonging to this node
    } way;
    double lat;
    double lon;
} __attribute__ ((__packed__));

struct _Route {
    int nrof_nodes;
    double length;
    RoutingNode *nodes;
};

struct _RoutingIndex {
    unsigned int nrof_ways;
    unsigned int nrof_nodes;
    RoutingWay *ways;
    RoutingNode *nodes;
    RoutingTagSet *tagsets;
};

struct _RoutingWay {
    unsigned int next; // The index of the node this leads to
    unsigned char tagset;
} __attribute__ ((__packed__));

struct _RoutingTagSet {
    unsigned int size;
    TAG tags[0];
} __attribute__ ((__packed__));

struct _RoutingProfile {
    char *name;
    double penalty[NROF_TAGS]; // A penalty for each tag
    double max_route_length; // Give up if no route shorter than this is found
};

Route * ww_routing_astar(RoutingIndex *ri, RoutingProfile *profile, 
        int *from_ids, double to_lat, double to_lon, double tolerance);

RoutingNode * ww_find_closest_node(RoutingIndex *ri, RoutingNode **sorted_by_lat, double lat, double lon);

int * ww_find_nodes(RoutingIndex *ri, RoutingNode **sorted_by_lat, double lat, double lon, double radius);

RoutingNode ** ww_nodes_get_sorted_by_lat(RoutingIndex *ri);

#endif /* WHICHWAY_H_ */
