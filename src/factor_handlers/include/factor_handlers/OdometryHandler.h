
/*
 * Copyright Notes
 *
 * Authors: Matteo Palieri      (matteo.palieri@jpl.nasa.gov)
 *          Kamak Ebadi         (kamak.ebadi@jpl.nasa.gov)
 *          Nobuhiro Funabiki   (nobuhiro.funabiki@jpl.nasa.gov)
*/

#ifndef ODOMETRY_HANDLER_H
#define ODOMETRY_HANDLER_H

// Includes
#include <factor_handlers/LampDataHandlerBase.h>

// Class Definition 
class OdometryHandler : public LampDataHandlerBase{
    
    public:
     
        OdometryHandler();
        ~OdometryHandler();  

        //Initialize(const ros::NodeHandle& n);  

        //RegisterOnlineCallbacks(const ros::NodeHandle& n);    

    protected: 

        // Odometry Subscribers 
        ros::Subscriber lidar_odom_sub_;
        ros::Subscriber visual_odom_sub_;
        ros::Subscriber wheel_odom_sub_;

        // Odometry Callbacks 
        void LidarOdometryCallback(const nav_msgs::Odometry::ConstPtr& msg); 
        void VisualOdometryCallback(const nav_msgs::Odometry::ConstPtr& msg);
        void WheelOdometryCallback(const nav_msgs::Odometry::ConstPtr& msg);

        // Odometry Storages 
        std::vector<geometry_msgs::PoseStamped> lidar_odometry_deque_; 
        std::vector<geometry_msgs::PoseStamped> visual_odometry_deque_;
        std::vector<geometry_msgs::PoseStamped> wheel_odometry_deque_; 
    


    private:    
};

#endif