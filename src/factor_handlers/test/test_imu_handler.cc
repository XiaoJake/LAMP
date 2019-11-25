#include <factor_handlers/ImuHandler.h>
#include <gtest/gtest.h>

class ImuHandlerTest : public ::testing::Test {

  public:

    ImuHandlerTest() {      
      // Load Params
      system("rosparam load $(rospack find "
             "factor_handlers)/config/imu_parameters.yaml");   

      tolerance_ = 1e-5;

      double t1 = 1.00;
      double t2 = 1.05;
      double t3 = 1.10;

      t1_ros.fromSec(t1);
      t2_ros.fromSec(t2);
      t3_ros.fromSec(t3);

      // 0 degree Pitch
      msg_first.header.stamp = t1_ros;
      msg_first.orientation.x = 0;
      msg_first.orientation.y = 0;
      msg_first.orientation.z = 0;
      msg_first.orientation.w = 1;    

      // 10 degree Pitch
      msg_second.header.stamp = t2_ros;
      msg_second.orientation.x = 0;
      msg_second.orientation.y = 0.0871557;
      msg_second.orientation.z = 0;
      msg_second.orientation.w = 0.9961947;   

      // 20 degree Pitch
      msg_third.header.stamp = t3_ros;
      msg_third.orientation.x = 0;
      msg_third.orientation.y = 0.1736482;
      msg_third.orientation.z = 0;
      msg_third.orientation.w = 0.9848078;     
    }   

    ImuHandler ih;

    double tolerance_;

  protected: 

    void ImuCallback(const ImuMessage::ConstPtr& msg) {  
      ih.ImuCallback(msg);
    }  

    Eigen::Vector3d QuaternionToYpr(const ImuQuaternion& imu_quaternion) const {
      return ih.QuaternionToYpr(imu_quaternion);
    }

    bool SetTimeForImuAttitude(const ros::Time& stamp) {
      return ih.SetTimeForImuAttitude(stamp);
    }  

    bool SetKeyForImuAttitude(const Symbol& key) { 
      return ih.SetKeyForImuAttitude(key);
    } 

    double GetTimeForImuAttitude() {
      return ih.query_stamp_;
    }     

    Symbol GetKeyForImuAttitude() {
      return ih.query_key_;
    }  

    bool InsertMsgInBuffer(const ImuMessage::ConstPtr& msg) {
      return ih.InsertMsgInBuffer(msg);
    }

    int CheckBufferSize() const {  
      return ih.CheckBufferSize();
    }  

    bool ClearBuffer() {
      return ih.ClearBuffer();
    }

    bool GetQuaternionAtTime(const ros::Time& stamp, 
                             ImuQuaternion& imu_quaternion) const {
      return ih.GetQuaternionAtTime(stamp, imu_quaternion);
    }  

    ImuMessage msg_first;
    ImuMessage msg_second;
    ImuMessage msg_third;

    ros::Time t1_ros;
    ros::Time t2_ros;
    ros::Time t3_ros;

  private:

};

/* TEST Initialize */
TEST_F(ImuHandlerTest, TestInitialize) {
  ros::NodeHandle nh;
  bool result = ih.Initialize(nh);
  ASSERT_TRUE(result);
}

/* TEST QuaternionToYpr */
TEST_F(ImuHandlerTest, TestQuaternionToYpr) {
  /*
  ImuQuaternion ---> w,x,y,z
  Roll ------------> rotation about x axis
  Pitch -----------> rotation about y axis
  Yaw -------------> rotation about z axis
  */
  ros::NodeHandle nh("~");
  ih.Initialize(nh);
  ImuQuaternion imu_quaternion;
  // 0 degree Pitch (0 rad)
  imu_quaternion = ImuQuaternion(1,0,0,0); 
  Eigen::Vector3d ypr = QuaternionToYpr(imu_quaternion);
  std::cerr << "------------------------------" << std::endl;
  std::cerr << "Test 0 degree Pitch (0 rad): " << std::endl;
  std::cerr << "Test Yaw: " << ypr[0] << std::endl;
  std::cerr << "Test Pitch: " << ypr[1] << std::endl;
  std::cerr << "Test Roll: " << ypr[2] << std::endl;
  std::cerr << "------------------------------" << std::endl;
  ASSERT_NEAR(ypr[1], 0, ImuHandlerTest::tolerance_);
  // 10 degree Pitch (0.174533 rad)
  imu_quaternion = ImuQuaternion(0.9961947,0,0.0871557,0); 
  ypr = QuaternionToYpr(imu_quaternion);
  std::cerr << "------------------------------" << std::endl;
  std::cerr << "Test 10 degree Pitch (0.174533 rad): " << std::endl;
  std::cerr << "Test Yaw: " << ypr[0] << std::endl;
  std::cerr << "Test Pitch: " << ypr[1] << std::endl;
  std::cerr << "Test Roll: " << ypr[2] << std::endl;
  std::cerr << "------------------------------" << std::endl;
  ASSERT_NEAR(ypr[1], 0.174533, ImuHandlerTest::tolerance_);
  // 20 degree Pitch (0,349066 rad)
  imu_quaternion = ImuQuaternion(0.9848078,0,0.1736482,0); 
  ypr = QuaternionToYpr(imu_quaternion);
  std::cerr << "------------------------------" << std::endl;
  std::cerr << "Test 20 degree Pitch (0,349066 rad): " << std::endl;
  std::cerr << "Test Yaw: " << ypr[0] << std::endl;
  std::cerr << "Test Pitch: " << ypr[1] << std::endl;
  std::cerr << "Test Roll: " << ypr[2] << std::endl;
  std::cerr << "------------------------------" << std::endl;
  ASSERT_NEAR(ypr[1], 0.349066, ImuHandlerTest::tolerance_);
}

/* TEST SetTimeForImuAttitude */
TEST_F(ImuHandlerTest, TestSetTimeForImuAttitude) {
  ros::NodeHandle nh("~");
  ih.Initialize(nh);
  double input = 1.00;
  ros::Time input_stamp; 
  input_stamp.fromSec(input);
  SetTimeForImuAttitude(input_stamp);
  double output = GetTimeForImuAttitude();
  ASSERT_EQ(output, input);
}

/* TEST SetKeyForImuAttitude */
TEST_F(ImuHandlerTest, TestSetKeyForImuAttitude) {
  ros::NodeHandle nh("~");
  ih.Initialize(nh);
  Key input_key = 2;
  Symbol input_key_symbol = Symbol(input_key);
  SetKeyForImuAttitude(input_key_symbol);
  Symbol output = GetKeyForImuAttitude();
  ASSERT_EQ(output, input_key);
}

/* TEST InsertMsgInBuffer */
TEST_F(ImuHandlerTest, TestInsertMsgInBuffer) {
  ros::NodeHandle nh("~");
  ih.Initialize(nh);
  ImuMessage imu_msg; 
  imu_msg.header = msg_first.header; 
  imu_msg.orientation = msg_first.orientation;
  ImuMessage::ConstPtr imu_msg_ptr(new ImuMessage(imu_msg)); 
  bool result = InsertMsgInBuffer(imu_msg_ptr);
  ASSERT_TRUE(result);
}

/* TEST CheckBufferSize*/
TEST_F(ImuHandlerTest, TestCheckBufferSize) {
  ros::NodeHandle nh("~");
  ih.Initialize(nh);
  ASSERT_EQ(CheckBufferSize(),0);
  ImuMessage imu_msg; 
  imu_msg.header = msg_first.header; 
  imu_msg.orientation = msg_first.orientation;
  ImuMessage::ConstPtr imu_msg_ptr(new ImuMessage(imu_msg)); 
  InsertMsgInBuffer(imu_msg_ptr);
  ASSERT_EQ(CheckBufferSize(), 1);
}

/* TEST ClearBuffer*/
TEST_F(ImuHandlerTest, TestClearBuffer) {
  ros::NodeHandle nh("~");
  ih.Initialize(nh);
  ASSERT_EQ(CheckBufferSize(),0);
  ImuMessage imu_msg; 
  imu_msg.header = msg_first.header; 
  imu_msg.orientation = msg_first.orientation;
  ImuMessage::ConstPtr imu_msg_ptr(new ImuMessage(imu_msg)); 
  InsertMsgInBuffer(imu_msg_ptr);
  ASSERT_EQ(CheckBufferSize(), 1);
  ClearBuffer();
  ASSERT_EQ(CheckBufferSize(), 0);
}

/* TEST GetQuaternionAtTime*/
TEST_F(ImuHandlerTest, TestGetQuaternionAtTime) {
  ros::NodeHandle nh("~");
  ih.Initialize(nh);
  sensor_msgs::Imu::ConstPtr imu_ptr1(
      new sensor_msgs::Imu(msg_first));
  sensor_msgs::Imu::ConstPtr imu_ptr2(
      new sensor_msgs::Imu(msg_second));
  sensor_msgs::Imu::ConstPtr imu_ptr3(
      new sensor_msgs::Imu(msg_third));
  ImuCallback(imu_ptr1);
  ImuCallback(imu_ptr2);
  ImuCallback(imu_ptr3);
  Eigen::Quaterniond q;
  bool result = GetQuaternionAtTime(t1_ros, q);
  ASSERT_TRUE(result);
  double t_out = 1.30; 
  ros::Time t_out_ros;
  t_out_ros.fromSec(t_out);
  bool result_out = GetQuaternionAtTime(t_out_ros, q);
  ASSERT_FALSE(result_out);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  ros::init(argc, argv, "test_imu_handler");
  return RUN_ALL_TESTS();
}