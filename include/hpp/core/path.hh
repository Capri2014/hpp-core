//
// Copyright (c) 2014 CNRS
// Authors: Florent Lamiraux
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

#ifndef HPP_CORE_PATH_HH
# define HPP_CORE_PATH_HH

# include <boost/concept_check.hpp>
# include <hpp/core/fwd.hh>
# include <hpp/core/config.hh>
# include <hpp/core/constraint-set.hh>

namespace hpp {
  namespace core {
    /// Abstraction of paths: mapping from time to configuration space
    ///
    class HPP_CORE_DLLAPI Path
    {
    public:
      /// \name Construction, destruction, copy
      /// \{

      /// Destructor
      virtual ~Path () throw () {}

      /// Return a shared pointer to a copy of this
      virtual PathPtr_t copy () const = 0;

      /// Static cast into a derived type
      template <class T> boost::shared_ptr<T> as (void)
      {
	BOOST_CONCEPT_ASSERT ((boost::Convertible <T*, Path*>));
	assert (HPP_DYNAMIC_PTR_CAST (T, weak_.lock ()));
	return HPP_STATIC_PTR_CAST (T, weak_.lock ());
      }

      /// Static cast into a derived type
      template <class T> boost::shared_ptr<const T> as (void) const
      {
	BOOST_CONCEPT_ASSERT ((boost::Convertible <T*, Path*>));
	assert (HPP_DYNAMIC_PTR_CAST (const T, weak_.lock ()));
	return HPP_STATIC_PTR_CAST (const T, weak_.lock ());
      }
	
      /// Extraction/Reversion of a sub-path
      /// \param subInterval interval of definition of the extract path
      /// If upper bound of subInterval is smaller than lower bound,
      /// result is reversed.
      virtual PathPtr_t extract (const interval_t& subInterval) const;

      /// Reversion of a path
      /// \return a new path that is this one reversed.
      virtual PathPtr_t reverse () const;

      /// \}

      Configuration_t operator () (const value_type& t) const throw ()
      {
	Configuration_t result (outputSize ());
	impl_compute (result, t);
	return result;
      }

      Configuration_t operator () (const value_type& t, bool& success) const throw ()
      {
	Configuration_t result (outputSize ());
	success = impl_compute (result, t);
	return result;
      }

      bool operator () (ConfigurationOut_t result, const value_type& t)
	const throw ()
      {
	return impl_compute (result, t);
      }

      /// \name Constraints
      /// \{

      /// Get constraints the path is subject to
      const ConstraintSetPtr_t& constraints () const
      {
	return constraints_;
      }
      /// Set constraints the path is subject to
      void constraints (const ConstraintSetPtr_t& constraints)
      {
	constraints_ = constraints;
      }
      /// \}

      /// Get size of configuration space
      size_type outputSize () const
      {
	return outputSize_;
      }

      /// Get size of velocity
      size_type outputDerivativeSize () const
      {
	return outputDerivativeSize_;
      }

      /// Get interval of definition
      const interval_t& timeRange () const
      {
	return timeRange_;
      }

      /// Get length of definition interval
      value_type length () const
      {
	return timeRange_.second - timeRange_.first;
      }

      /// Get the initial configuration
      virtual Configuration_t initial () const = 0;

      /// Get the final configuration
      virtual Configuration_t end () const = 0;

    protected:
      /// Print path in a stream
      virtual std::ostream& print (std::ostream &os) const = 0;

      /// Constructor
      /// \param interval interval of definition of the path,
      /// \param outputSize size of the output configuration,
      /// \param outputDerivativeSize number of degrees of freedom of  the
      ///        underlying robot
      Path (const interval_t& interval, size_type outputSize,
	    size_type outputDerivativeSize) :
	timeRange_ (interval), outputSize_ (outputSize),
	outputDerivativeSize_ (outputDerivativeSize), constraints_ ()
	{
	}

      /// Constructor
      /// \param interval interval of definition of the path,
      /// \param outputSize size of the output configuration,
      /// \param outputDerivativeSize number of degrees of freedom of  the
      ///        underlying robot
      /// \param constraints constraints the set is subject to,
      ///        constraints are solved at each evaluation of the output
      ///        configuration.
      Path (const interval_t& interval, size_type outputSize,
	    size_type outputDerivativeSize,
	    const ConstraintSetPtr_t& constraints) :
	timeRange_ (interval), outputSize_ (outputSize),
	outputDerivativeSize_ (outputDerivativeSize), constraints_ (constraints)
	{
	}

      /// Copy constructor
      Path (const Path& path) :
	timeRange_ (path.timeRange_), outputSize_ (path.outputSize_),
	constraints_ (path.constraints_)
	  {
	  }

      /// Store weak pointer to itself
      ///
      /// should be called at construction of derived class instances
      void init (const PathPtr_t& self)
      {
	weak_ = self;
      }
      /// \brief Function evaluation.
      ///
      /// \return true if everything went good.
      virtual bool impl_compute (ConfigurationOut_t configuration,
				 value_type t) const = 0;

      /// Interval of definition
      interval_t timeRange_;
    private:
      /// Size of the configuration space
      size_type outputSize_;
      /// Number of degrees of freedom of the robot
      size_type outputDerivativeSize_;
      /// Constraints that apply to the robot
      ConstraintSetPtr_t constraints_;
      /// Weak pointer to itself
      PathWkPtr_t weak_;
      friend std::ostream& operator<< (std::ostream& os, const Path& path);
    }; // class Path
    inline std::ostream& operator<< (std::ostream& os, const Path& path)
    {
      return path.print (os);
    }

  } //   namespace core
} // namespace hpp
#endif // HPP_CORE_PATH_HH
