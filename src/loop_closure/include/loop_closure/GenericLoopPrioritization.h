/**
 * @file   GenericLoopPrioritization.h
 * @brief  Prioritize loop closures by the order received
 * @author Yun Chang
 */
#pragma once

#include <gtsam/inference/Symbol.h>
#include <pose_graph_msgs/KeyedScan.h>
#include <ros/console.h>
#include <ros/ros.h>
#include <map>
#include "loop_closure/PointCloudUtils.h"

#include "loop_closure/LoopPrioritization.h"

namespace lamp_loop_closure {

class GenericLoopPrioritization : public LoopPrioritization {
  typedef pcl::PointCloud<pcl::PointXYZI> PointCloud;
  typedef pcl::PointCloud<pcl::PointXYZI>::ConstPtr PointCloudConstPtr;
  typedef pcl::PointCloud<pcl::Normal> Normals;

 public:
  GenericLoopPrioritization();
  ~GenericLoopPrioritization();

  bool Initialize(const ros::NodeHandle& n) override;

  bool LoadParameters(const ros::NodeHandle& n) override;

  bool CreatePublishers(const ros::NodeHandle& n) override;

  bool RegisterCallbacks(const ros::NodeHandle& n) override;

 protected:
  void PopulatePriorityQueue() override;

  void PublishBestCandidates() override;

  void KeyedScanCallback(const pose_graph_msgs::KeyedScan::ConstPtr& scan_msg);

  void ComputeIcpObservability(PointCloud::ConstPtr cloud,
                               Eigen::Matrix<double, 3, 1>* eigenvalues) const;

  void ProcessTimerCallback(const ros::TimerEvent& ev);

  // Store keyed scans
  std::map<gtsam::Key, PointCloudConstPtr> keyed_scans_;

  // Define subscriber
  ros::Subscriber keyed_scans_sub_;

  // Timer
  ros::Timer update_timer_;

  // Paramters
  double min_observability_;  // Discard any candidate with observability
                              // below threshold
  double normals_radius_;     // radius used for cloud normal computation

  bool choose_best_;  // Send only best candidate
};

}  // namespace lamp_loop_closure