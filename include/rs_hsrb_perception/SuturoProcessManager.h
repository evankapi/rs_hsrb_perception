/**
 * Modified RoboSherlock Process Manager
 * Optimized for the Suturo Perception Pipelines
 * @author Fenja Kollasch
 */
#include <robosherlock/types/all_types.h>
#include <robosherlock/scene_cas.h>
#include <robosherlock/utils/time.h>
#include <robosherlock/flowcontrol/RSProcessManager.h>
#include <rs_hsrb_perception/suturo_conversion.h>
#include <rs_hsrb_perception/types/all_types.h>
#include <suturo_perception_msgs/ObjectDetectionData.h>


using namespace suturo_perception_msgs;

class SuturoProcessManager {
private:
    std::string savePath;
    ros::NodeHandle nh;
    std::string name;
    bool visualize = false;
    bool filter_regions = false;
    std::vector<std::string> regions;
    void getClusterFeatures(rs::ObjectHypothesis cluster, std::vector<ObjectDetectionData> &data);
public:
    // Public Parameters
    RSAggregateAnalysisEngine *engine;
    rs::Visualizer visualizer;

    // ROS stuff for image topics
    ros::ServiceServer vis_service;
    image_transport::Publisher image_pub;
    image_transport::ImageTransport image_transport;

    // Functions
    SuturoProcessManager(ros::NodeHandle n, std::string &name);
    SuturoProcessManager(ros::NodeHandle n, std::string savePath, std::string &name);
    ~SuturoProcessManager(){};

    void setup();

    void init(std::string &pipeline);

    void run(std::map<std::string, boost::any> args, std::vector<ObjectDetectionData> &detectionData);

    void setVisualize(bool visualize);

    /**
     * Determines if a vertical plane is visible
     * Necessary for door detection
     * @return
     */
    bool has_vertical_plane();

    bool updateRegionFilter(bool enabled);

    bool visControlCallback(robosherlock_msgs::RSVisControl::Request &req,
                            robosherlock_msgs::RSVisControl::Response &res);
};
