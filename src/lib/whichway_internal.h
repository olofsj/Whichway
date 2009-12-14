
typedef struct _RoutingIndex RoutingIndex;
typedef struct _RoutingNode RoutingNode;
typedef struct _RoutingWay RoutingWay;

typedef enum { no_highway, motorway, motorway_link, trunk, trunk_link, primary, primary_link, secondary, secondary_link, tertiary, unclassified, road, residential, living_street, service, track, pedestrian, raceway, services, bus_guideway, path, cycleway, footway, bridleway, byway, steps, mini_roundabout, stop, traffic_signals, crossing, motorway_junction, incline, incline_steep, ford, bus_stop, turning_circle, construction, proposed, emergency_access_point, speed_camera } TAG_HIGHWAY;
char *TAG_HIGHWAY_VALUES[] = { "no", "motorway", "motorway_link", "trunk", "trunk_link", "primary", "primary_link", "secondary", "secondary_link", "tertiary", "unclassified", "road", "residential", "living_street", "service", "track", "pedestrian", "raceway", "services", "bus_guideway", "path", "cycleway", "footway", "bridleway", "byway", "steps", "mini_roundabout", "stop", "traffic_signals", "crossing", "motorway_junction", "incline", "incline_steep", "ford", "bus_stop", "turning_circle", "construction", "proposed", "emergency_access_point", "speed_camera" };
#define NROF_TAG_HIGHWAY_VALUES 40

typedef enum { no_traffic_calming, yes, bump, chicane, cushion, hump, rumble_strip, table, choker } TAG_TRAFFIC_CALMING;
char *TAG_TRAFFIC_CALMING_VALUES[] = { "no", "yes", "bump", "chicane", "cushion", "hump", "rumble_strip", "table", "choker" };
#define NROF_TAG_TRAFFIC_CALMING_VALUES 9

typedef enum { no_smoothness, excellent, good, intermediate, bad, very_bad, horrible, very_horrible, impassable } TAG_SMOOTHNESS;
char *TAG_SMOOTHNESS_VALUES[] = { "no", "excellent", "good", "intermediate", "bad", "very_bad", "horrible", "very_horrible", "impassable" };
#define NROF_TAG_SMOOTHNESS_VALUES 9

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
    TAG_HIGHWAY type;
};

