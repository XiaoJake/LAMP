#include "utils/CommonFunctions.h"
#include "utils/CommonStructs.h"

#include <gtsam/sam/RangeFactor.h>
#include <gtsam/slam/BetweenFactor.h>
#include <gtsam/slam/PriorFactor.h>

bool PoseGraph::TrackFactor(const Factor& factor) {
  return TrackFactor(factor.ToMsg());
}

bool PoseGraph::TrackFactor(const EdgeMessage& msg) {
  if (msg.type == pose_graph_msgs::PoseGraphEdge::PRIOR) {
    TrackPrior(msg);
    return false;
  }

  if (edges_.find(msg) != edges_.end()) {
    ROS_DEBUG_STREAM("Edge of type " << msg.type << " from key "
                                     << gtsam::DefaultKeyFormatter(msg.key_from)
                                     << " to key "
                                     << gtsam::DefaultKeyFormatter(msg.key_to)
                                     << " already exists.");
    return false;
  }

  gtsam::Pose3 delta = utils::MessageToPose(msg);

  if (delta.rotation().matrix().determinant() > 1.01 && b_first_){
    ROS_WARN_STREAM("[Track Factor] Determinant non unary, FIRST TIME is " << delta.rotation().matrix().determinant());
    b_first_ = false;
  } else if (delta.rotation().matrix().determinant() > 1.01){
    ROS_WARN_STREAM("[Track Factor] Determinant non unary, is " << delta.rotation().matrix().determinant());
  }

  Eigen::Quaterniond quat(delta.rotation().matrix());
  // quat = quat.normalized();
  delta.rotation().matrix() = quat.normalized().toRotationMatrix();
  ROS_INFO_STREAM("[Track Factor] Quat normalized is: " << quat.normalized().x() << ", "  << quat.normalized().y() << ", " << quat.normalized().z() << ", "  << quat.normalized().w());

  ROS_WARN_STREAM("[Track Factor] Determinant pose normalization in TrackFactor, is " << delta.rotation().matrix().determinant() << ".\n matrix is: " << delta.rotation().matrix());
  Gaussian::shared_ptr noise = utils::MessageToCovariance(msg);

  if (msg.type == pose_graph_msgs::PoseGraphEdge::ODOM) {
    // Add to posegraph
    ROS_DEBUG_STREAM("Adding Odom edge for key "
                     << gtsam::DefaultKeyFormatter(msg.key_from) << " to key "
                     << gtsam::DefaultKeyFormatter(msg.key_to));
    nfg_.add(gtsam::BetweenFactor<gtsam::Pose3>(
        gtsam::Symbol(msg.key_from), gtsam::Symbol(msg.key_to), delta, noise));
  } else if (msg.type == pose_graph_msgs::PoseGraphEdge::LOOPCLOSE) {
    ROS_DEBUG_STREAM("Adding loop closure edge for key "
                     << gtsam::DefaultKeyFormatter(msg.key_from) << " to key "
                     << gtsam::DefaultKeyFormatter(msg.key_to));
    nfg_.add(gtsam::BetweenFactor<gtsam::Pose3>(
        gtsam::Symbol(msg.key_from), gtsam::Symbol(msg.key_to), delta, noise));
  } else if (msg.type == pose_graph_msgs::PoseGraphEdge::ARTIFACT) {
    ROS_DEBUG_STREAM("Adding artifact edge for key "
                     << gtsam::DefaultKeyFormatter(msg.key_from) << " to key "
                     << gtsam::DefaultKeyFormatter(msg.key_to));
    nfg_.add(gtsam::BetweenFactor<gtsam::Pose3>(
        gtsam::Symbol(msg.key_from), gtsam::Symbol(msg.key_to), delta, noise));
  } else if (msg.type == pose_graph_msgs::PoseGraphEdge::UWB_RANGE) {
    ROS_DEBUG_STREAM("Adding UWB range factor for key "
                     << gtsam::DefaultKeyFormatter(msg.key_from) << " to key "
                     << gtsam::DefaultKeyFormatter(msg.key_to));
    double range = msg.range;
    double sigmaR = msg.range_error;
    gtsam::noiseModel::Base::shared_ptr rangeNoise =
        gtsam::noiseModel::Isotropic::Sigma(1, sigmaR);
    nfg_.add(gtsam::RangeFactor<gtsam::Pose3, gtsam::Pose3>(
        gtsam::Symbol(msg.key_from),
        gtsam::Symbol(msg.key_to),
        range,
        rangeNoise));
  } else if (msg.type == pose_graph_msgs::PoseGraphEdge::UWB_BETWEEN) {
    ROS_DEBUG_STREAM("Adding UWB between factor for key "
                     << gtsam::DefaultKeyFormatter(msg.key_from) << " to key "
                     << gtsam::DefaultKeyFormatter(msg.key_to));
    nfg_.add(gtsam::BetweenFactor<gtsam::Pose3>(
        gtsam::Symbol(msg.key_from), gtsam::Symbol(msg.key_to), delta, noise));
  } else {
    ROS_DEBUG_STREAM("Cannot add edge of unknown type "
                     << msg.type << " between keys "
                     << gtsam::DefaultKeyFormatter(msg.key_from) << " and "
                     << gtsam::DefaultKeyFormatter(msg.key_to) << ".");
  }

  edges_.insert(msg);
  edges_new_.insert(msg);
  return true;
}

bool PoseGraph::TrackFactor(gtsam::Symbol key_from,
                            gtsam::Symbol key_to,
                            int type,
                            const gtsam::Pose3& transform,
                            const gtsam::SharedNoiseModel& covariance) {
  Factor factor;
  factor.key_from = key_from;
  factor.key_to = key_to;
  factor.type = type;
  factor.transform = transform;
  factor.covariance = covariance;
  return TrackFactor(factor.ToMsg());
}

bool PoseGraph::TrackNode(const Node& node) {
  return TrackNode(node.ToMsg());
}

bool PoseGraph::TrackNode(const NodeMessage& msg, bool b_do_values) {
  auto pose = utils::MessageToPose(msg);

  if (pose.rotation().matrix().determinant() > 1.01 && b_first_){
    ROS_WARN_STREAM("[Track Node] Determinant non unary, FIRST TIME is " << pose.rotation().matrix().determinant());
    b_first_ = false;
  } else if (pose.rotation().matrix().determinant() > 1.01){
    ROS_WARN_STREAM("[Track Node] Determinant non unary, is " << pose.rotation().matrix().determinant());
  }

  Eigen::Quaterniond quat(pose.rotation().matrix());
  // quat = quat.normalize();
  pose.rotation().matrix() = quat.normalized().toRotationMatrix();
  ROS_INFO_STREAM("[Track Node] Quat normalized is: " << quat.normalized().x() << ", "  << quat.normalized().y() << ", " << quat.normalized().z() << ", "  << quat.normalized().w());
  
  ROS_WARN_STREAM("[Track Node] Determinant pose normalization in TrackNode, is " << pose.rotation().matrix().determinant() << "\n. matrix is: " << pose.rotation().matrix());

  if (b_do_values){
    if (values_.exists(msg.key)) {
      values_.update(msg.key, pose);
    }
    else {
      values_.insert(msg.key, pose);
    }
    if (values_new_.exists(msg.key)) {
      values_new_.update(msg.key, pose);
    }
    else {
      values_new_.insert(msg.key, pose);
    }
  }

  keyed_stamps[msg.key] = msg.header.stamp;
  
  // make copy to modify ID
  if (nodes_.find(msg) == nodes_.end()) {
    NodeMessage m = msg;
    if (m.ID.empty() && !symbol_id_map.empty())
      m.ID = symbol_id_map(msg.key);
    nodes_.insert(m);
    nodes_new_.insert(m);
    return true;
  }
  else {
    nodes_.erase(nodes_.find(msg));
    nodes_.insert(msg);
    return true;
  }
  return false;
}

bool PoseGraph::TrackNode(const ros::Time& stamp,
                          gtsam::Symbol key,
                          const gtsam::Pose3& pose,
                          const gtsam::SharedNoiseModel& covariance) {
  if (values_.exists(key)) {
    values_.update(key, pose);
  }
  else {
    values_.insert(key, pose);
  }
  if (values_new_.exists(key)) {
    values_new_.update(key, pose);
  }
  else {
    values_new_.insert(key, pose);
  }
  
  NodeMessage msg =
      utils::GtsamToRosMsg(stamp, fixed_frame_id, key, pose, covariance);
  return TrackNode(msg, false);
}

bool PoseGraph::TrackPrior(const EdgeMessage& msg) {
  if (msg.type != pose_graph_msgs::PoseGraphEdge::PRIOR) {
    TrackFactor(msg);
    return false;
  }
  if (priors_.find(msg) != priors_.end()) {
    // prior already exists
    return false;
  }

  // create precisions // TODO - the precision should come from the message
  // shouldn't it? As we will use priors for different applications
  // gtsam::Vector6 prior_precisions;
  // prior_precisions.head<3>().setConstant(0.0);
  // prior_precisions.tail<3>().setConstant(10.0);
  // // TODO(Yun) create parameter for this
  // static const gtsam::SharedNoiseModel& prior_noise =
  //     gtsam::noiseModel::Diagonal::Precisions(prior_precisions);

  ROS_DEBUG_STREAM("Adding prior factor for key "
                   << gtsam::DefaultKeyFormatter(msg.key_from));
  gtsam::Pose3 delta = utils::MessageToPose(msg);
  Gaussian::shared_ptr noise = utils::MessageToCovariance(msg);
  gtsam::PriorFactor<gtsam::Pose3> factor(
      gtsam::Symbol(msg.key_from), delta, noise);
  nfg_.add(factor);
  priors_new_.insert(msg);
  priors_.insert(msg);
  return true;
}

bool PoseGraph::TrackPrior(const Factor& factor) {
  return TrackPrior(factor.ToMsg());
}

bool PoseGraph::TrackPrior(const Node& node) {
  Factor prior;
  prior.covariance = node.covariance;
  prior.key_from = node.key;
  prior.key_to = node.key;
  prior.type = pose_graph_msgs::PoseGraphEdge::PRIOR;
  prior.transform = node.pose;
  return TrackPrior(prior.ToMsg());
}

bool PoseGraph::TrackPrior(gtsam::Symbol key,
                           const gtsam::Pose3& pose,
                           const gtsam::SharedNoiseModel& covariance) {
  Factor prior;
  prior.covariance = covariance;
  prior.key_from = key;
  prior.key_to = key;
  prior.type = pose_graph_msgs::PoseGraphEdge::PRIOR;
  prior.transform = pose;
  return TrackPrior(prior.ToMsg());
}

// DEPRECATED!!
void PoseGraph::AddNewValues(const gtsam::Values& new_values) {
  // Main values variable
  for (auto v : new_values) {
    if (!values_.tryInsert(v.key, v.value).second) {
      values_.update(v.key, v.value);
    }
    if (!values_new_.tryInsert(v.key, v.value).second) {
      values_new_.update(v.key, v.value);
    }
  }
}

// DEPRECATED!!
void PoseGraph::AddNewFactors(const gtsam::NonlinearFactorGraph& nfg) {
  nfg_.add(nfg);
}

void PoseGraph::Initialize(gtsam::Symbol initial_key,
                           const gtsam::Pose3& pose,
                           const Diagonal::shared_ptr& covariance) {
  nfg_ = gtsam::NonlinearFactorGraph();
  values_ = gtsam::Values();

  b_first_ = true;

  Node prior;
  prior.key = initial_key;
  prior.pose = pose;
  prior.covariance = covariance;

  TrackPrior(prior);
  TrackNode(prior);
}

void PoseGraph::InsertKeyedScan(gtsam::Symbol key,
                                const PointCloud::ConstPtr& scan) {
  keyed_scans.insert(std::pair<gtsam::Symbol, PointCloud::ConstPtr>(key, scan));
}

void PoseGraph::InsertKeyedStamp(gtsam::Symbol key, const ros::Time& stamp) {
  keyed_stamps.insert(std::pair<gtsam::Symbol, ros::Time>(key, stamp));
}

void PoseGraph::InsertStampedOdomKey(double seconds, gtsam::Symbol key) {
  stamp_to_odom_key.insert(std::pair<double, gtsam::Symbol>(seconds, key));
}
