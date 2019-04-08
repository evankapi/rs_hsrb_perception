/**
 * Modified RoboSherlock Process Manager
 * Optimized for the Suturo Perception Pipelines
 * @author Fenja Kollasch
 */
#include <rs_hsrb_perception/SuturoProcessManager.h>

SuturoProcessManager::SuturoProcessManager(ros::NodeHandle n, const std::string savePath, std::string &name) :
    savePath(savePath),
    nh(n),
    name(name)
{
    outInfo("A RoboSherlock process manager optimized for the Suturo perception was created.");
    signal(SIGINT, RSProcessManager::signalHandler);
    outInfo("Creating resource manager");
    uima::ResourceManager &resourceManager = uima::ResourceManager::createInstance((name+"/RoboSherlock").c_str());

    switch(OUT_LEVEL)
    {
        case OUT_LEVEL_NOOUT:
        case OUT_LEVEL_ERROR:
            resourceManager.setLoggingLevel(uima::LogStream::EnError);
            break;
        case OUT_LEVEL_INFO:
            resourceManager.setLoggingLevel(uima::LogStream::EnWarning);
            break;
        case OUT_LEVEL_DEBUG:
            resourceManager.setLoggingLevel(uima::LogStream::EnMessage);
            break;
    }
    regions = std::vector<std::string>();
}



void SuturoProcessManager::init(std::string &pipeline) {
    outInfo("Initializing Engine...");
    signal(SIGINT, RSProcessManager::signalHandler);

    std::string pipelinePath;
    rs::common::getAEPaths(pipeline, pipelinePath);
    engine.init(pipelinePath, false);

    uima::ErrorInfo errorInfo;
    mongo::client::GlobalInstance instance;
}

void SuturoProcessManager::run(std::map<std::string, boost::any> args, std::vector<ObjectDetectionData>& detectionData) {
    outInfo("Running the Suturo Process Manager");
    outInfo("Setting up runtime parameters...");
    if(args.find("visualize") != args.end()) {
        visualize = boost::any_cast<bool>(&args["visualize"]);
        //@Todo: Create visualizer here
    }
    if(args.find("regions") != args.end()) {
        outInfo("Custom regions are displayed");
        regions = boost::any_cast<std::vector<std::string>>(args["regions"]);
        filter_regions = true;
    }
    outInfo("Analysis engine starts processing");
    engine.process();
    uima::CAS* tcas = engine.getCas();
    rs::SceneCas cas(*tcas);
    pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud_ptr(new pcl::PointCloud<pcl::PointXYZRGBA>);
    cas.get(VIEW_CLOUD, *cloud_ptr);

    rs::Scene scene = cas.getScene();
    std::vector<rs::ObjectHypothesis> clusters;
    scene.identifiables.filter(clusters);
    for (auto &cluster : clusters) {
        outInfo("Cluster region: " << cluster.region());
        if(!filter_regions || std::find(regions.begin(), regions.end(), cluster.region()) != regions.end()) {
            getClusterFeatures(cluster, detectionData);
        } else {
            outInfo("Object was ignored because it seems to be placed on the wrong surface");
        }
    }
}


void SuturoProcessManager::getClusterFeatures(rs::ObjectHypothesis cluster, std::vector<ObjectDetectionData> &data) {

    std::vector<rs::Geometry> geometry;
    cluster.annotations.filter(geometry);

    if(!geometry.empty()) {
        std::vector<rs::PoseAnnotation> poses;
        cluster.annotations.filter(poses);

        std::vector<rs::Classification> classification;
        cluster.annotations.filter(classification);

        std::vector<rs::ClassConfidence> confi;
        cluster.annotations.filter(confi);

        std::vector<rs::Shape> shapes;
        cluster.annotations.filter(shapes);

        geometry_msgs::PoseStamped poseStamped;
        std::string objClass;
        std::string knownObjClass;
        float confidence = 0;
        float knownObjConfidence = 0;
        ObjectDetectionData odd;
        if(!poses.empty()) {
            rs_hsrb_perception::conversion::from(poses[0].world.get(), poseStamped);
        } else {
            ROS_WARN("Warning: No pose information was perceived");
        }
        if(!classification.empty()){
            objClass = classification[0].classname.get();
            knownObjClass = classification[1].classname.get();
            outInfo("OBJCLASSNAME >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>: " << classification[0].classname.get());
            outInfo("OBJCLASSNAME >>>>>>>>>KNOWN>>>>>>>KNOWN>>>>>>>: " << classification[1].classname.get());
        } else {
            ROS_WARN("Warning: No object class was perceived");
        }
        if(!confi.empty()){
            confidence = confi[0].score.get();
            knownObjConfidence = confi[1].score.get();
            outInfo("OBJCLASSCONFI <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<: " << confi[0].score.get());
            outInfo("OBJCLASSCONFI <<<<<<<<<KNOWN<<<<<<<KNOWN<<<<<<: " << confi[1].score.get());
        } else {
            ROS_WARN("Warning: No confidence was perceived");
        }

        rs_hsrb_perception::conversion::makeObjectDetectionData(poseStamped, geometry[0],
                rs_hsrb_perception::conversion::decode_shape(shapes), objClass, confidence,
                knownObjClass, knownObjConfidence, odd);
        data.push_back(odd);


    } else {
        ROS_WARN("Object Feature detection was unsuccessful. No geometries were recognized for this object.");
    }

}
