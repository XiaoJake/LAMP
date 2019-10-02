/**
 *  @brief Test cases for talker class
 *
 *  This file shows an example usage of gtest.
 */

#include <gtest/gtest.h>
#include <factor_handlers/OdometryHandler.h>

TEST(OdomHandlerTest, EnqueueLidarOdometryMsgs) {
  ros::NodeHandle nh, pnh("~");
  OdometryHandler myOdometryHandler; 
  myOdometryHandler.Initialize(nh);
  // std::string phrase("Hello World");
  // std_msgs::String msg = talker.composeGreetingMsg(phrase);
  // EXPECT_EQ(phrase, msg.data);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  ros::init(argc, argv, "test_odometry_handler");
  return RUN_ALL_TESTS();
}
