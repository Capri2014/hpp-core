// Copyright (c) 2016, Joseph Mirabel
// Authors: Joseph Mirabel (joseph.mirabel@laas.fr)
//
// This file is part of hpp-core.
// hpp-core is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// hpp-core is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Lesser Public License for more details.  You should have
// received a copy of the GNU Lesser General Public License along with
// hpp-core. If not, see <http://www.gnu.org/licenses/>.

#define BOOST_TEST_MODULE paths
#include <boost/test/included/unit_test.hpp>
#include <boost/test/test_case_template.hpp>
#include <boost/mpl/list.hpp>

#include <boost/shared_ptr.hpp>
// Boost version 1.54
// Cannot include boost CPU timers
// #include <boost/timer/timer.hpp>
// because the original timers are already included by
// the unit test framework
// #include <boost/timer.hh>
#include <hpp/pinocchio/device.hh>
#include <hpp/pinocchio/joint.hh>
#include <hpp/pinocchio/configuration.hh>

#include <hpp/core/problem.hh>
#include <hpp/core/path.hh>
#include <hpp/core/straight-path.hh>
#include <hpp/core/subchain-path.hh>
#include <pinocchio/multibody/joint/joint-variant.hpp>
#include <pinocchio/multibody/geometry.hpp>

#define TOSTR( x ) static_cast< std::ostringstream & >( ( std::ostringstream() << x ) ).str()

using hpp::pinocchio::Device;
using hpp::pinocchio::DevicePtr_t;
using hpp::pinocchio::JointPtr_t;

using namespace hpp::core;
using namespace hpp::pinocchio;

using ::se3::JointModelPX;
using ::se3::JointModelPY;
using ::se3::JointModelPZ;
using ::se3::JointIndex;

DevicePtr_t createRobot ()
{
  DevicePtr_t robot = Device::create ("test");
  const std::string& name = robot->name ();
  ModelPtr_t m = ModelPtr_t(new ::se3::Model());
  GeomModelPtr_t gm = GeomModelPtr_t(new ::se3::GeometryModel());
  robot->setModel(m);
  robot->setGeomModel(gm);
  Transform3f mat; mat.setIdentity ();
  std::string jointName = name + "_x";

  JointModelPX::TangentVector_t max_effort = JointModelPX::TangentVector_t::Constant(JointModelPX::NV,std::numeric_limits<double>::max());
  JointModelPX::TangentVector_t max_velocity = JointModelPX::TangentVector_t::Constant(JointModelPX::NV,std::numeric_limits<double>::max());
  JointModelPX::ConfigVector_t lower_position; lower_position << -4;
  JointModelPX::ConfigVector_t upper_position; upper_position <<  4;

  robot->model().addJoint(0,JointModelPX(), mat,jointName,max_effort,max_velocity,lower_position,upper_position);


  robot->createData();
  robot->createGeomData();

  return robot;



  /* DevicePtr_t robot = Device::create ("test");

  const std::string& name = robot->name ();
  fcl::Transform3f mat; mat.setIdentity ();
  JointPtr_t joint;
  std::string jointName = name + "_x";
  // Translation along x
  fcl::Matrix3f permutation;
  joint = objectFactory.createJointTranslation (mat);
  joint->name (jointName);

  joint->isBounded (0, 1);
  joint->lowerBound (0, -4);
  joint->upperBound (0, +4);

  robot->rootJoint (joint);
  return robot;*/
}

DevicePtr_t createRobot2 ()
{
  DevicePtr_t robot = Device::create ("test");
  const std::string& name = robot->name ();
  ModelPtr_t m = ModelPtr_t(new ::se3::Model());
  GeomModelPtr_t gm = GeomModelPtr_t(new ::se3::GeometryModel());
  robot->setModel(m);
  robot->setGeomModel(gm);
  Transform3f mat; mat.setIdentity ();
  std::string jointName = name + "_x";

  JointModelPX::TangentVector_t max_effort = JointModelPX::TangentVector_t::Constant(JointModelPX::NV,std::numeric_limits<double>::max());
  JointModelPX::TangentVector_t max_velocity = JointModelPX::TangentVector_t::Constant(JointModelPX::NV,std::numeric_limits<double>::max());
  JointModelPX::ConfigVector_t lower_position = JointModelPY::ConfigVector_t::Constant(-4);
  JointModelPX::ConfigVector_t upper_position = JointModelPY::ConfigVector_t::Constant(4);
  JointIndex idJoint = 0;

  for(int i = 0 ; i < 10 ; ++i){
    idJoint = robot->model().addJoint(idJoint,JointModelPX(), mat,jointName + TOSTR(i),max_effort,max_velocity,lower_position,upper_position);
  }


  robot->createData();
  robot->createGeomData();
  return robot;
  /*DevicePtr_t robot = Device::create ("test");

  const std::string& name = robot->name ();
  fcl::Transform3f mat; mat.setIdentity ();
  JointPtr_t joint, parentJoint;
  std::string jointName = name + "_x";
  // Translation along x
  fcl::Matrix3f permutation;
  for (int i = 0; i < 10; ++i) {
    joint = objectFactory.createJointTranslation (mat);
    joint->name (jointName + TOSTR(i));

    joint->isBounded (0, 1);
    joint->lowerBound (0, -4);
    joint->upperBound (0, +4);
    if (i == 0) robot->rootJoint (joint);
    else parentJoint->addChildJoint (joint);
    parentJoint = joint;
  }

  return robot;*/
}

typedef std::pair<value_type, value_type> Pair_t;

std::ostream& operator<< (std::ostream& os, const Pair_t& p) {
  os << "Pair " << p.first << ", " << p.second;
  return os;
}

void printAt(const PathPtr_t& p, ConfigurationOut_t q, value_type t)
{
  (*p)(q, t);
  std::cout << t << ":\t" << q.transpose() << std::endl;
}
void checkAt(const PathPtr_t orig, value_type to,
             const PathPtr_t extr, value_type te) {
  Configuration_t q1 (orig->outputSize()),
                  q2 (orig->outputSize());
  (*orig)(q1, to);
  (*extr)(q2, te);
  BOOST_CHECK_MESSAGE(q2.isApprox(q1),
      "Extracted path is wrong."
      "\nExpected: " << q1.transpose() <<
      "\nGot     : " << q2.transpose()
      );
}

BOOST_AUTO_TEST_CASE (extracted)
{
  DevicePtr_t dev = createRobot();
  BOOST_REQUIRE (dev);
  Problem problem (dev);

  Configuration_t q1 (dev->configSize()); q1 << 0;
  Configuration_t q2 (dev->configSize()); q2 << 1;
  PathPtr_t p1 = (*problem.steeringMethod()) (q1, q2), p2;

  p2 = p1->extract (Pair_t (0.5, 1));
  checkAt (p1, 0.5, p2, 0.0);
  checkAt (p1, 1.0, p2, 0.5);

  p2 = p1->extract (Pair_t (1, 0.5));
  checkAt (p1, 1.0, p2, 0.0);
  checkAt (p1, 0.5, p2, 0.5);

  p2 = p2->extract (Pair_t (0, 0.25));
  checkAt (p1, 1.0, p2, 0.0);
  checkAt (p1, .75, p2, .25);

  p2 = p1->extract (Pair_t (1, 0.5));
  p2 = p2->extract (Pair_t (0.25, 0));
  checkAt (p1, .75, p2, 0.0);
  checkAt (p1, 1.0, p2, .25);
}

BOOST_AUTO_TEST_CASE (subchain)
{
  DevicePtr_t dev = createRobot2(); // 10 translations
  BOOST_REQUIRE (dev);
  Problem problem (dev);

  segments_t intervals;
  intervals.push_back(segment_t (0,3));
  intervals.push_back(segment_t (6,3));

  Configuration_t q1 (Configuration_t::Zero(dev->configSize()));
  Configuration_t q2 (Configuration_t::Ones(dev->configSize()));
  q2.tail<5>().setConstant(-1);

  PathPtr_t p1 = (*problem.steeringMethod()) (q1, q2), p2;
  p2 = SubchainPath::create(p1, intervals);

  BOOST_CHECK(p2->outputSize() == 6);
  Configuration_t q (p2->outputSize());
  (*p2) (q, 0);
  BOOST_CHECK(q.head<3>().isZero());
  BOOST_CHECK(q.tail<3>().isZero());

  (*p2) (q, p1->length());
  BOOST_CHECK(q.head<3>().isApprox( Configuration_t::Ones(3)));
  BOOST_CHECK(q.tail<3>().isApprox(-Configuration_t::Ones(3)));

  (*p2) (q, p1->length() * 0.5);
  BOOST_CHECK(q.head<3>().isApprox( Configuration_t::Ones(3) * 0.5));
  BOOST_CHECK(q.tail<3>().isApprox(-Configuration_t::Ones(3) * 0.5));
}
