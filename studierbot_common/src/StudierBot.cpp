#include "StudierBot.h"
#include "params.h"

#include <iostream>
using namespace std;

StudierBot::StudierBot()
{
  _track               = TRACK;
  _wheelBase           = WHEELBASE;
  _gearRatio           = GEARRATIO;
  _wheelCircumference  = WHEELCICRUMFERENCE;
  _vMax                = _motor.getRPMMax() / _gearRatio * WHEELCICRUMFERENCE;

  _diagonal            = sqrt(_wheelBase*_wheelBase + _track*_track);
  _cosa                = cos(_track / _diagonal);

  /*if(subscribeJoy)
  {
    _joySub = _nh.subscribe<sensor_msgs::Joy>("joy", 1, &StudierBot::joyCallback, this);
    cout << "Subscribed to joy" << endl;
  }
  else*/
  {
    _velSub = _nh.subscribe("vel/teleop", 1, &StudierBot::velocityCallback, this);
    cout << "Subscribed to vel/teleop" << endl;
  }
}

void StudierBot::run()
{
  ros::Rate rate(25);
  _lastCmd = ros::Time::now();

  bool run = true;
  while(run)
  {
    ros::spinOnce();

    ros::Duration dt = ros::Time::now() - _lastCmd;
    bool lag = (dt.toSec()>1.0);
    if(lag)
    {
      ROS_WARN_STREAM("Lag detected ... exiting robot control node");
    }

    run = ros::ok() && !lag;

    rate.sleep();
  }

  _motor.stop();
}

/*void StudierBot::joyCallback(const sensor_msgs::Joy::ConstPtr& joy)
{
  // Assignment of joystick axes to motor commands
  double linear  = joy->axes[1];
  double angular = joy->axes[0];
  double speed   = (joy->axes[3]+1.0)/2.0;

  double rpmMax = _motor.getRPMMax();

  int left  = rpmMax * speed * (-linear-angular);
  int right = rpmMax * speed * (linear-angular);

  if(left>rpmMax)   left  = rpmMax;
  if(left<-rpmMax)  left  = -rpmMax;
  if(right>rpmMax)  right = rpmMax;
  if(right<-rpmMax) right = -rpmMax;

  _motor.setRPM(left, right);

  _lastCmd = ros::Time::now();
}*/

void StudierBot::velocityCallback(const geometry_msgs::TwistStamped& cmd)
{
  double vl, vr;

  twistToTrackspeed(&vl, &vr, cmd.twist.linear.x, cmd.twist.angular.z);

  double rpmLeft  = trackspeedToTicksPerTurn(vl) * 60.0;
  double rpmRight = trackspeedToTicksPerTurn(vr) * 60.0;

  _motor.setRPM(rpmLeft, rpmRight);

  _lastCmd = ros::Time::now();
}

void StudierBot::twistToTrackspeed(double *vl, double *vr, double v, double omega) const
{
  *vr =  -1 * (v + omega * _diagonal / (2.0 * _cosa));
  *vl =       (v - omega * _diagonal / (2.0 * _cosa));
}

void StudierBot::trackspeedToTwist(double vl, double vr, double *v, double *omega) const
{
  *v     = (vl + vr) / 2.0;
  *omega = (vr - vl) * _cosa / (2.0 * _diagonal);
}

double StudierBot::trackspeedToTicksPerTurn(double v) const
{
  return (v / _wheelCircumference) * _gearRatio;
}
