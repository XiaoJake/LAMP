// Includes
#include "ArtifactHandler.h"

/**
 * Constructor
 * Initialize
 * LoadParameters
 * RegisterCallbacks      -     Not sure here
 * ComputeTransform
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 * 
 */

/**
#ifndef COMMON_STRUCTS_H
#define COMMON_STRUCTS_H

#include <geometry_utils/Transform3.h>
#include <geometry_utils/GeometryUtilsROS.h>

// Typedef for 6x6 covariance matrices (x, y, z, roll, pitch, yaw).
typedef geometry_utils::MatrixNxNBase<double, 3> Mat33;
typedef geometry_utils::MatrixNxNBase<double, 6> Mat66;
typedef geometry_utils::MatrixNxNBase<double, 12> Mat1212;

// GTSAM edge types
typedef std::pair<gtsam::Symbol, gtsam::Symbol> Edge;
typedef std::pair<gtsam::Symbol, gtsam::Symbol> ArtifactEdge;
typedef std::pair<gtsam::Symbol, gtsam::Pose3> Prior; 


// Struct definition 
struct FactorData {
  bool b_has_data; // False if there is no data
  std::string type; // odom, artifact, loop clsoure
  // Vector for possible multiple factors
  std::vector<gtsam::Pose3> transforms; // The transform (for odom, loop closures etc.) and pose for TS
  std::vector<Mat1212> covariances; // Covariances for each transform 
  std::vector<std::pair<ros::Time, ros::Time>> time_stamps; // Time when the measurement as acquired
  // TODO - use ros::Time or something else?

  std::vector<gtsam::Key> artifact_key; // key for the artifacts
};

#endif
 */ 

// Constructor
ArtifactHandler::ArtifactHandler()
                : largest_artifact_id_(0),
                  use_artifact_loop_closure_(false) {
                    // TODO Initialize artifact_data_
                    // artifact_data_ = 
                  }

/*! \brief Initialize parameters and callbacks. 
 * n - Nodehandle
 * Returns bool
 */
bool ArtifactHandler::Initialize(const ros::NodeHandle& n){
  name_ = ros::names::append(n.getNamespace(), "Artifact");

  if (!LoadParameters(n)) {
    ROS_ERROR("%s: Failed to load artifact parameters.", name_.c_str());
    return false;
  }

  if (!RegisterCallbacks(n, ???)) {
    ROS_ERROR("%s: Failed to register artifact callback.", name_.c_str());
    return false;
  }

  return true;
}

/*! \brief Load artifact parameters. 
 * n - Nodehandle
 * Returns bool
 */
bool ArtifactHandler::LoadParameters(const ros::NodeHandle& n) {
  if (!pu::Get("frame_id/artifacts_in_global", artifacts_in_global_))
    return false;
  if (!pu::Get("use_artifact_loop_closure", use_artifact_loop_closure_)) return false;

  //Get the artifact prefix from launchfile to set initial unique artifact ID
  bool b_initialized_artifact_prefix_from_launchfile = true;
  std::string artifact_prefix;
  unsigned char artifact_prefix_converter[1];
  if (!pu::Get("artifact_prefix", artifact_prefix)){
     b_initialized_artifact_prefix_from_launchfile = false;
     ROS_ERROR("Could not find node ID assosiated with robot_namespace");
  }
  
  if (b_initialized_artifact_prefix_from_launchfile){
    std::copy( artifact_prefix.begin(), artifact_prefix.end(), artifact_prefix_converter);
    artifact_prefix_ = artifact_prefix_converter[0];
  }
  return true; 
}

/*! \brief Register callbacks. 
 * n - Nodehandle
 * Returns bool
 */
bool ArtifactHandler::RegisterCallbacks(const ros::NodeHandle& n, bool from_log) {
  // Create a local nodehandle to manage callback subscriptions.
  ros::NodeHandle nl(n);

  if (from_log)
    return RegisterLogCallbacks(n);
  else
    return RegisterOnlineCallbacks(n);
}

/*! \brief Compute transform from Artifact message.
 * Returns Transform
 */
Eigen::Vector3d ArtifactHandler::ComputeTransform(const core_msgs::Artifact& msg) {
  // Get artifact position 
  Eigen::Vector3d artifact_position;
  artifact_position << msg.point.point.x, msg.point.point.y, msg.point.point.z;

  Eigen::Vector3d R_artifact_position; // In robot frame

  // Check if artifact is published in global frame 
  // And convert to local frame to include in pose graph 
  if (artifacts_in_global_) { // Already in fixed frame
    // ---------------------------Should not come here------------------------------
    // gtsam::Key pose_key = loop_closure_.GetKeyAtTime(msg.point.header.stamp);
    // // TODO Needs loop closure
    // geometry_utils::Transform3 global_pose = loop_closure_.GetPoseAtKey(pose_key);

    // // Transform artifact pose from global frame to body frame 
    // Eigen::Matrix<double, 3, 3> R_global = global_pose.rotation.Eigen();
    // Eigen::Matrix<double, 3, 1> T_global = global_pose.translation.Eigen();
    // // std::cout << "Global robot position is: " << T_global[0] << ", " << T_global[1] << ", " << T_global[2] << std::endl;
    // // std::cout << "Global robot rotation is: " << R_global << std::endl;
    // R_artifact_position = R_global.transpose() * (artifact_position - T_global); // Apply transform
    // -------------------------------------------------------------------------------
  } else {
    R_artifact_position = artifact_position;
  }

  std::cout << "Artifact position in robot frame is: " << R_artifact_position[0] << ", "
            << R_artifact_position[1] << ", " << R_artifact_position[2]
            << std::endl;
  return R_artifact_position;
}

/*! \brief  Get artifacts key and if not create one.
 * Returns Artifacts Key
 */
gtsam::Key ArtifactHandler::GetArtifactKey(const core_msgs::Artifact& msg) {
  // Get the artifact id
  std::string artifact_id = msg.parent_id; // Note that we are looking at the parent id here
  
  // Artifact key
  gtsam::Key cur_artifact_key;
  bool b_is_new_artifact = false;

  // get artifact id / key -----------------------------------------------
  // Check if the ID of the object already exists in the object hash
  if (use_artifact_loop_closure_ && !IsNewArtifact(artifact_id) && 
      msg.label != "cellphone") {
    // Take the ID for that object - no reconciliation in the pose-graph of a cell phone (for now)
    cur_artifact_key = artifact_id2key_hash[artifact_id];
    std::cout << "artifact previously observed, artifact id " << artifact_id 
              << " with key in pose graph " 
              << gtsam::DefaultKeyFormatter(cur_artifact_key) << std::endl;
  } else {
    // New artifact - increment the id counters
    b_is_new_artifact = true;
    cur_artifact_key = gtsam::Symbol(artifact_prefix_, largest_artifact_id_);
    ++largest_artifact_id_;
    std::cout << "new artifact observed, artifact id " << artifact_id 
              << " with key in pose graph " 
              << gtsam::DefaultKeyFormatter(cur_artifact_key) << std::endl;
    // update hash
    artifact_id2key_hash[artifact_id] = cur_artifact_key;

    ArtifactInfo artifactinfo(msg.parent_id);
    // create hashmap for artifact id to artifact_info
    artifact_id_to_info[artifact_id] = artifactinfo;
  }
  return cur_artifact_key;
}

/*! \brief  Checks if artifact is a new one.
 * Returns  True if new or false otherwise 
 */
bool ArtifactHandler::IsNewArtifact(const std::string artifact_id) {
  return artifact_id2key_hash.find(artifact_id) == artifact_id2key_hash.end();
}

/*! \brief  Callback for Artifacts.
  * Returns  Void
  */
void ArtifactHandler::ArtifactCallback(const core_msgs::Artifact& msg) {
  // Subscribe to artifact messages, include in pose graph, publish global position 
  // Artifact information
  std::cout << "Artifact message received is for id " << msg.id << std::endl;
  std::cout << "\t Parent id: " << msg.parent_id << std::endl;
  std::cout << "\t Confidence: " << msg.confidence << std::endl;
  std::cout << "\t Position:\n[" << msg.point.point.x << ", "
            << msg.point.point.y << ", " << msg.point.point.z << "]"
            << std::endl;
  std::cout << "\t Label: " << msg.label << std::endl;

  // Check for NaNs and reject 
  if (std::isnan(msg.point.point.x) || std::isnan(msg.point.point.y) || std::isnan(msg.point.point.z)){
    ROS_WARN("NAN positions input from artifact message - ignoring");
    return;
  }

  // Compute the transformation (from relative / global pose)
  Eigen::Vector3d R_artifact_position = ComputeTransform(msg);

  // Get Artifact key
  gtsam::Key cur_artifact_key = GetArtifactKey(msg);

  // Extract covariance information
  Mat33 cov;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      cov(i, j) = msg.covariance[3*i+j];

  // Generate gtsam pose
  const gtsam::Pose3 R_pose_A 
      = gtsam::Pose3(gtsam::Rot3(), gtsam::Point3(R_artifact_position[0], 
                                                  R_artifact_position[1],
                                                  R_artifact_position[2]));
  
  // Fill artifact_data_
  // Make new data true
  artifact_data_.b_has_data = true;
  // Fill type
  artifact_data_.type = "artifact";
  // Append transform
  artifact_data_.transforms.push_back(R_pose_A);
  // Append covariance
  artifact_data_.covariances.push_back(cov);
  // Append std::pair<ros::Time, ros::Time(0.0)> for artifact
  artifact_data_.time_stamps.push_back(std::make_pair(msg.header.stamp, ros::Time(0.0)));
  // Append the artifact key
  artifact_data_.artifact_key.push_back(cur_artifact_key);
}

/*! \brief  Callback for ArtifactBase.
 * Returns  Void
 */
void ArtifactHandler::ArtifactBaseCallback(const core_msgs::Artifact::ConstPtr& msg) {
  ROS_INFO_STREAM("Artifact message recieved");
  core_msgs::Artifact artifact;
  artifact.header = msg->header;
  artifact.name = msg->name;
  artifact.parent_id = msg->parent_id;
  artifact.seq = msg->seq;
  artifact.hotspot_name = msg->hotspot_name;
  artifact.point = msg->point;
  artifact.covariance = msg->covariance;
  artifact.confidence = msg->confidence;
  artifact.label = msg->label;
  artifact.thumbnail = msg->thumbnail;

  ArtifactInfo artifactinfo(msg->parent_id);
  artifactinfo.msg = artifact;

  std::cout << "Artifact position in world is: " << artifact.point.point.x
            << ", " << artifact.point.point.y << ", " << artifact.point.point.z
            << std::endl;
  std::cout << "Frame ID is: " << artifact.point.header.frame_id << std::endl;

  std::cout << "\t Parent id: " << artifact.parent_id << std::endl;
  std::cout << "\t Confidence: " << artifact.confidence << std::endl;
  std::cout << "\t Position:\n[" << artifact.point.point.x << ", "
            << artifact.point.point.y << ", " << artifact.point.point.z << "]"
            << std::endl;
  std::cout << "\t Label: " << artifact.label << std::endl;

  // Publish artifacts - should be updated from the pose-graph
  loop_closure_.PublishArtifacts();
}

/*! \brief  Gives the factors to be added.
 * Returns  Factors 
 */
void ArtifactHandler::GetData(FactorData artifact_data) {
  return artifact_data_;
}

/*! \brief  Create the publishers to log data.
 * Returns  Values 
 */
bool ArtifactHandler::RegisterLogCallbacks(const ros::NodeHandle& n) {
  ROS_INFO("%s: Registering log callbacks.", name_.c_str());
  return CreatePublishers(n);
}

bool ArtifactHandler::CreatePublishers(const ros::NodeHandle& n) {
  // Create a local nodehandle to manage callback subscriptions.
  ros::NodeHandle nl(n);

  // Create publisher for artifact

  return true;
}

/*! \brief Register Online callbacks. 
 * n - Nodehandle
 * Returns bool
 */
bool RegisterOnlineCallbacks(const ros::NodeHandle& n) {
  ROS_INFO("%s: Registering online callbacks.", name_.c_str());

  // Create a local nodehandle to manage callback subscriptions.
  ros::NodeHandle nl(n);

  if (!b_is_basestation_ && !b_use_lo_frontend_){
    artifact_sub_ = nl.subscribe(
        "artifact_relative", 10, &ArtifactCallback, this);
  }

  // Create pose-graph callbacks for base station
  if(b_is_basestation_){
    int num_robots = robot_names_.size();
    // init size of subscribers
    // loop through each robot to set up subscriber
    for (size_t i = 0; i < num_robots; i++) {
      ros::Subscriber artifact_base_sub =
          nl.subscribe("/" + robot_names_[i] + "/blam_slam/artifact_global",
                       10,
                       &ArtifactBaseCallback,
                       this);
      Subscriber_artifactList_.push_back(artifact_base_sub);
      ROS_INFO_STREAM(i);
    }    
  }

  if (!b_is_front_end_){
    ros::Subscriber artifact_base_sub =
        nl.subscribe("artifact_global_sub",
                      10,
                      &ArtifactBaseCallback,
                      this);
    Subscriber_artifactList_.push_back(artifact_base_sub);
  }
  return CreatePublishers(n);  
}

/*! \brief  Updates the global pose of an artifact 
 * Returns  Void
 */
void UpdateGlobalPose(gtsam::Key artifact_key ,gtsam::Pose3 global_pose) {
  std::unordered_map<std::string, gtsam::Key>::const_iterator it = artifact_id2key_hash.find(artifact_key);
  std::string artifact_id;
  if (it == artifact_id2key_hash.end())
    std::cout << "Key not found in the Artifact id to key map.";
  else
    artifact_id = artifact_id2key_hash->first; 

  artifact_id_to_info_[artifact_id].global_pose = global_pose;
}