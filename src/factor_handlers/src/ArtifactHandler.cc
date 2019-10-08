// Includes
#include "factor_handlers/ArtifactHandler.h"

/**
 * Constructor             - Done
 * Initialize              - Done
 * LoadParameters          - Done
 * RegisterCallbacks       - Not sure here
 * ComputeTransform        - Done
 * GetData                 - Done
 * PublishArtifacts        - Not Done
 * RegisterOnlineCallbacks - b_is_basestation_ and b_use_lo_frontend_ and b_is_front_end_ in lamp. else Nearly Done
 * GetArtifactKey          - NA
 * ArtifactCallback        - Check if this needs more
 * ArtifactBaseCallback    - Need to check this
 * RegisterLogCallbacks    - Done
 * CreatePublishers        - Not Done
 * UpdateGlobalPose        - Final check
 * Tests                   - Test needed.
 * covariances             - Make Covariance SharedNoiseModel 
 */

// Constructor
ArtifactHandler::ArtifactHandler()
                : largest_artifact_id_(0),
                  use_artifact_loop_closure_(false) {
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
  // Need to change this
  if (!RegisterCallbacks(n, false)) {
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
  
  std::cout << "Artifact position in robot frame is: " << artifact_position[0] << ", "
            << artifact_position[1] << ", " << artifact_position[2]
            << std::endl;
  return artifact_position;
}

/*! \brief  Get artifacts ID from artifact key
 * Returns Artifacts ID
 */
std::string ArtifactHandler::GetArtifactID(gtsam::Key artifact_key) {
  std::string artifact_id;
  for (auto it = artifact_id2key_hash.begin(); it != artifact_id2key_hash.end(); ++it) {
    if (it->second == artifact_key) {
      artifact_id = it->first; 
      return artifact_id;
    }
  } 
  std::cout << "Artifact ID not found for key" << gtsam::DefaultKeyFormatter(artifact_key) << std::endl;
  return "";
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

  // Get the transformation
  Eigen::Vector3d R_artifact_position = ComputeTransform(msg);

  // Get Artifact key
  // gtsam::Key cur_artifact_key = GetArtifactKey(msg);
  // Get the artifact id
  std::string artifact_id = msg.parent_id; // Note that we are looking at the parent id here
  
  // Artifact key
  gtsam::Key cur_artifact_key;
  bool b_is_new_artifact = false;

  // get artifact id / key -----------------------------------------------
  // Check if the ID of the object already exists in the object hash
  if (use_artifact_loop_closure_ && artifact_id2key_hash.find(artifact_id) != artifact_id2key_hash.end() && 
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
  }
  // Generate gtsam pose
  const gtsam::Pose3 R_pose_A 
      = gtsam::Pose3(gtsam::Rot3(), gtsam::Point3(R_artifact_position[0], 
                                                  R_artifact_position[1],
                                                  R_artifact_position[2]));

  // Fill ArtifactInfo hash
  ArtifactInfo artifactinfo(msg.parent_id);
  artifactinfo.msg = msg;

  // keep track of artifact info: add to hash if not added
  if (artifact_key2info_hash_.find(cur_artifact_key) == artifact_key2info_hash_.end()) {
    // ROS_INFO_STREAM("New artifact detected with id" << artifact.id);
    artifact_key2info_hash_[cur_artifact_key] = artifactinfo;
  } else {
    ROS_INFO("Existing artifact detected");
    artifact_key2info_hash_[cur_artifact_key] = artifactinfo;
  }

  // Extract covariance information
  gtsam::Matrix33 cov;
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      cov(i+3, j+3) = msg.covariance[3*i+j];

  gtsam::SharedNoiseModel noise =
      gtsam::noiseModel::Gaussian::Covariance(cov);

  // Fill artifact_data_
  // Make new data true
  artifact_data_.b_has_data = true;
  // Fill type
  artifact_data_.type = "artifact";
  // Append transform
  artifact_data_.transforms.push_back(R_pose_A);
  // Append covariance
  artifact_data_.covariances.push_back(noise);
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
  artifactinfo.msg = artifact;           // TODO check this

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
  // loop_closure_.PublishArtifacts();      // TODO Need publish function
}

/*! \brief  Gives the factors to be added.
 * Returns  Factors 
 */
FactorData ArtifactHandler::GetData() {
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
bool ArtifactHandler::RegisterOnlineCallbacks(const ros::NodeHandle& n) {
  ROS_INFO("%s: Registering online callbacks.", name_.c_str());

  // Create a local nodehandle to manage callback subscriptions.
  // ros::NodeHandle nl(n);

  // if (!b_is_basestation_ && !b_use_lo_frontend_ && !b_is_front_end_){
  //   artifact_sub_ = nl.subscribe(
  //       "artifact_relative", 10, &ArtifactCallback, this);
  // }

  // // Create pose-graph callbacks for base station
  // if(b_is_basestation_){
  //   int num_robots = robot_names_.size();
  //   // init size of subscribers
  //   // loop through each robot to set up subscriber
  //   for (size_t i = 0; i < num_robots; i++) {
  //     ros::Subscriber artifact_base_sub =
  //         nl.subscribe("/" + robot_names_[i] + "/blam_slam/artifact_global",
  //                      10,
  //                      &ArtifactBaseCallback,
  //                      this);
  //     Subscriber_artifactList_.push_back(artifact_base_sub);
  //   }    
  // }

  // if (!b_is_front_end_){
  //   ros::Subscriber artifact_base_sub =
  //       nl.subscribe("artifact_global_sub",
  //                     10,
  //                     &ArtifactBaseCallback,
  //                     this);
  //   Subscriber_artifactList_.push_back(artifact_base_sub);
  // }
  return CreatePublishers(n);  
}

/*! \brief  Updates the global pose of an artifact 
 * Returns  Void
 */
void ArtifactHandler::UpdateGlobalPose(gtsam::Key artifact_key ,gtsam::Pose3 global_pose) {
  if (artifact_key2info_hash_.find(artifact_key) != artifact_key2info_hash_.end()) {
    artifact_key2info_hash_[artifact_key].global_pose = global_pose;
  } else {
    std::cout << "Key not found in the Artifact id to key map.";
  }
}

 /*! \brief  Publish Artifact. Need to see if publish_all is still relevant
  * I am considering publishing if we have the key and the pose without 
  * any further processing.
  * TODO Resolve the frame and transform between world and map
  * in output message. 
  * Returns  Void
  */
void ArtifactHandler::PublishArtifacts(gtsam::Key artifact_key ,gtsam::Pose3 global_pose) {
  // Get the artifact pose
  Eigen::Vector3d artifact_position = global_pose.translation().vector();
  std::string artifact_label;
  gtsam::Symbol artifact_symbol_key = gtsam::Symbol(artifact_key);

  if (!(artifact_symbol_key.chr() == 'l' || 
        artifact_symbol_key.chr() == 'm' || 
        artifact_symbol_key.chr() == 'n' || 
        artifact_symbol_key.chr() == 'o' || 
        artifact_symbol_key.chr() == 'p' || 
        artifact_symbol_key.chr() == 'q')){
    ROS_WARN("ERROR - have a non-landmark ID");
    ROS_INFO_STREAM("Bad ID is " << gtsam::DefaultKeyFormatter(artifact_key));
    return;
  }

  // Using the artifact key to publish that artifact
  ROS_INFO("Publishing the new artifact");
  ROS_INFO_STREAM("Artifact key to publish is " << gtsam::DefaultKeyFormatter(artifact_key));

  // Check that the key exists
  if (artifact_key2info_hash_.count(artifact_key) == 0) {
    ROS_WARN("Artifact key is not in hash, nothing to publish");
    return;
  }

  // Get label 
  artifact_label = artifact_key2info_hash_[artifact_key].msg.label;
  
  // Increment update count
  artifact_key2info_hash_[artifact_key].num_updates++;

  std::cout << "Number of updates of artifact is: "
            << artifact_key2info_hash_[artifact_key].num_updates
            << std::endl;

  // Fill artifact message
  core_msgs::Artifact new_msg = artifact_key2info_hash_[artifact_key].msg;
  
  // Update the time
  new_msg.header.stamp = ros::Time::now();

  // Fill the new message positions
  new_msg.point.point.x = artifact_position[0];
  new_msg.point.point.y = artifact_position[1];
  new_msg.point.point.z = artifact_position[2];
  // TODO Need to check the frame id and transform
  // new_msg.point.header.frame_id = fixed_frame_id_;
  // Transform to world frame from map frame
  // new_msg.point = tf_buffer_.transform(
    // new_msg.point, "world", new_msg.point.header.stamp, "world");

  // Print out
  // Transform at time of message
  std::cout << "Artifact position in world is: " << new_msg.point.point.x
            << ", " << new_msg.point.point.y << ", " << new_msg.point.point.z
            << std::endl;
  std::cout << "Frame ID is: " << new_msg.point.header.frame_id << std::endl;

  std::cout << "\t Parent id: " << new_msg.parent_id << std::endl;
  std::cout << "\t Confidence: " << new_msg.confidence << std::endl;
  std::cout << "\t Position:\n[" << new_msg.point.point.x << ", "
            << new_msg.point.point.y << ", " << new_msg.point.point.z << "]"
            << std::endl;
  std::cout << "\t Label: " << new_msg.label << std::endl;

  // Publish
  artifact_pub_.publish(new_msg);
}