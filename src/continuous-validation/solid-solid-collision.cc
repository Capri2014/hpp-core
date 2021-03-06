//
// Copyright (c) 2014,2015,2016, 2018 CNRS
// Authors: Florent Lamiraux, Joseph Mirabel, Diane Bury
//
// This file is part of hpp-core
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
// hpp-core  If not, see
// <http://www.gnu.org/licenses/>.

#include <hpp/core/continuous-validation/solid-solid-collision.hh>

#include <hpp/pinocchio/device.hh>

# include <hpp/fcl/collision_data.h>
# include <hpp/fcl/collision.h>
# include <hpp/pinocchio/body.hh>
# include <hpp/pinocchio/collision-object.hh>
# include <hpp/pinocchio/joint.hh>
# include <hpp/core/straight-path.hh>
# include <hpp/core/interpolated-path.hh>
# include <hpp/core/deprecated.hh>
# include <pinocchio/multibody/geometry.hpp>


namespace hpp {
  namespace core {
    namespace continuousValidation {

      SolidSolidCollisionPtr_t SolidSolidCollision::create(const JointPtr_t& joint_a,
					      const ConstObjectStdVector_t& objects_b,
					      value_type tolerance)
      {
        SolidSolidCollisionPtr_t shPtr(new SolidSolidCollision(joint_a, objects_b, tolerance));
        return shPtr;
      }

      SolidSolidCollisionPtr_t SolidSolidCollision::create(const JointPtr_t& joint_a,
					      const JointPtr_t& joint_b,
					      value_type tolerance)
      {
        SolidSolidCollisionPtr_t shPtr (new SolidSolidCollision(joint_a, joint_b, tolerance));
        return shPtr;
      }

      value_type SolidSolidCollision::computeMaximalVelocity(vector_t& Vb) const
      {
        value_type maximalVelocity = 0;
        for (CoefficientVelocities_t::const_iterator itCoef =
        coefficients_.begin (); itCoef != coefficients_.end (); ++itCoef) {
          const JointPtr_t& joint = itCoef->joint_;
          const value_type& value = itCoef->value_;
          maximalVelocity += value * Vb.segment(joint->rankInVelocity(), joint->numberDof()).norm();
        }
        return maximalVelocity;
      }

      bool SolidSolidCollision::removeObjectTo_b (const CollisionObjectConstPtr_t& object)
      {
        const std::size_t s = pairs_.size();
        for (CollisionPairs_t::iterator _pair = pairs_.begin ();
          _pair != pairs_.end ();) {
          if (object == _pair->second)
            _pair = pairs_.erase (_pair);
          else
            ++_pair;
        }
        return pairs_.size() < s;
      }

      void SolidSolidCollision::addCollisionPair (const CollisionObjectConstPtr_t& left,
          const CollisionObjectConstPtr_t& right)
      {
        // std::cout << "size = " << pairs_.size() << std::endl;
        // std::cout << "capacity = " << pairs_.capacity() << std::endl;
        pairs_.push_back (CollisionPair_t (left, right));
      }

      std::string SolidSolidCollision::name () const
      {
        std::ostringstream oss;
        oss << "(" << joint_a_->name () << ",";
        if (joint_b_) oss << joint_b_->name ();
        else oss << "obstacles";
        oss << ")";
        return oss.str ();
      }

      std::ostream& SolidSolidCollision::print (std::ostream& os) const
      {
        os << "SolidSolidCollision: " << joint_a_->name()
          << " - " << (joint_b_ ? joint_b_->name() : "World") << '\n';
        const se3::Model& model = joint_a_->robot ()->model();
        JointIndices_t joints = computeSequenceOfJoints ();
        for (std::size_t i = 0; i < joints.size (); ++i) {
          if (i > 0) os << model.names[i] << ',';
          else       os << "World"        << ',';
        }
        os << '\n';
        for (std::size_t i = 0; i < coefficients_.size(); ++i)
          os << coefficients_[i].value_ << ", ";
        return os;
      }

      SolidSolidCollision::JointIndices_t SolidSolidCollision::computeSequenceOfJoints () const
      {
        JointIndices_t joints;

        const se3::Model& model = joint_a_->robot ()->model();
        const JointIndex id_a = indexJointA(),
                         id_b = indexJointB();
        JointIndex ia = id_a, ib = id_b;

        std::vector<JointIndex> fromA, fromB;
        while (ia != ib)
        {
          if (ia > ib) {
            fromA.push_back(ia);
            ia = model.parents[ia];
          } else /* if (ia < ib) */ {
            fromB.push_back(ib);
            ib = model.parents[ib];
          }
        }
        assert (ia == ib);
        fromA.push_back(ia);

        // Check joint vectors
        if (fromB.empty()) assert (fromA.back() == id_b);
        else               assert (model.parents[fromB.back()] == ia);

        // Build sequence
        joints = fromA;
        joints.insert(joints.end(), fromB.rbegin(), fromB.rend());
        assert(joints.front() == id_a);
        assert(joints.back() == id_b);
        assert(joints.size() > 1);
        return joints;
      }

      void SolidSolidCollision::computeCoefficients (const JointIndices_t& joints)
      {
        const se3::Model& model = joint_a_->robot ()->model();

        JointPtr_t child;
        assert (joints.size () > 1);
        coefficients_.resize (joints.size () - 1);
        pinocchio::DevicePtr_t robot =joint_a_->robot ();
        // Store r0 + sum of T_{i/i+1} in a variable
        value_type cumulativeLength = joint_a_->linkedBody ()->radius ();
        value_type distance;
        std::size_t i = 0;
        while (i + 1 < joints.size()) {
          if (model.parents[joints[i]] == joints[i+1])
            child = Joint::create (robot, joints[i]);
          else if (model.parents[joints[i+1]] == joints[i])
            child = Joint::create (robot, joints[i+1]);
          else
            abort ();
          assert(child);
          coefficients_ [i].joint_ = child;
          // Go through all known types of joints
          //  TODO: REPLACE THESE FUNCTIONS WITH NEW API
          distance = child->maximalDistanceToParent ();
          coefficients_ [i].value_ =
            child->upperBoundLinearVelocity () +
            cumulativeLength * child->upperBoundAngularVelocity ();
          cumulativeLength += distance;

          ++i;
        }
      }

      SolidSolidCollision::SolidSolidCollision (const JointPtr_t& joint_a,
            const JointPtr_t& joint_b,
            value_type tolerance) :
        BodyPairCollision(tolerance), joint_a_ (joint_a), joint_b_ (joint_b),
        coefficients_ ()
      {
        assert (joint_a);
        if (joint_b && joint_b_->robot () != joint_a_->robot ()) {
          throw std::runtime_error
            ("Joints do not belong to the same device.");
        }
        if (indexJointA() == indexJointB()) {
          throw std::runtime_error ("Bodies should be different");
        }
        if (tolerance < 0) {
          throw std::runtime_error ("tolerance should be non-negative.");
        }

        if (joint_a_) { assert(joint_a_->linkedBody ()); }
        if (joint_b_) { assert(joint_b_->linkedBody ()); }
        // Find sequence of joints
        JointIndices_t joints (computeSequenceOfJoints ());
        computeCoefficients (joints);
      }

      SolidSolidCollision::SolidSolidCollision (const JointPtr_t& joint_a,
            const ConstObjectStdVector_t& objects_b,
            value_type tolerance) :
        BodyPairCollision(tolerance), joint_a_ (joint_a), joint_b_ (),
        coefficients_ ()
      {
        assert (joint_a);
        BodyPtr_t body_a = joint_a_->linkedBody ();
        assert (body_a);
        for (size_type i = 0; i < body_a->nbInnerObjects(); ++i) {
	      CollisionObjectConstPtr_t obj = body_a->innerObjectAt(i);
          for (ConstObjectStdVector_t::const_iterator it = objects_b.begin ();
              it != objects_b.end (); ++it) {
            assert (!(*it)->joint () ||
                (*it)->joint ()->robot () != joint_a_->robot ());
            pairs_.push_back (CollisionPair_t(obj, *it));
          }
        }
        // Find sequence of joints
        JointIndices_t joints (computeSequenceOfJoints ());
        computeCoefficients (joints);
      }

    } // namespace continuousValidation
  } // namespace core
} // namespace hpp
