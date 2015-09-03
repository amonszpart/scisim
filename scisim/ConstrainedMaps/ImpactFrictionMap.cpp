// ImpactFrictionMap.cpp
//
// Breannan Smith
// Last updated: 09/03/2015

#include "ImpactFrictionMap.h"

#include "SCISim/UnconstrainedMaps/FlowableSystem.h"
#include "SCISim/Constraints/Constraint.h"

#include "SCISim/HDF5File.h"

#include <iostream>

ImpactFrictionMap::~ImpactFrictionMap()
{}

bool ImpactFrictionMap::impactOperatorSupported( const std::string& impact_operator_name )
{
  // Array doesn't seem to work correctly on CRF's version of stdlib
  // const std::array<std::string,3> valid_impact_operators{ { "lcp_ipopt", "lcp_ql", "lcp_apgd" } };
  const std::vector<std::string> valid_impact_operators{ "lcp_ipopt", "lcp_ql", "lcp_apgd", "lcp_gpminres" };
  return std::find( valid_impact_operators.begin(), valid_impact_operators.end(), impact_operator_name ) != valid_impact_operators.end();
}

bool ImpactFrictionMap::frictionOperatorSupported( const std::string& friction_operator_name )
{
  const std::vector<std::string> valid_friction_operators{ "bound_constrained_mdp_ql", "bound_constrained_mdp_operator_ipopt", "restricted_sample_linear_mdp_ipopt", "linear_mdp_ql", "linear_mdp_ipopt", "smooth_mdp_ipopt", "bound_constrained_mdp_apgd", "smooth_mdp_apgd" };
  return std::find( valid_friction_operators.begin(), valid_friction_operators.end(), friction_operator_name ) != valid_friction_operators.end();
}

// TODO: Implement in a cleaner way -- create a function in fsys that checks if kinematic constraints are respected
bool ImpactFrictionMap::noImpulsesToKinematicGeometry( const FlowableSystem& fsys, const SparseMatrixsc& N, const VectorXs& alpha, const SparseMatrixsc& D, const VectorXs& beta, const VectorXs& v0 )
{
  const VectorXs dv{ fsys.Minv() * ( N * alpha + D * beta ) };
  assert( unsigned( v0.size() ) % fsys.numVelDoFsPerBody() == 0 );
  const unsigned nbodies = unsigned( v0.size() ) / fsys.numVelDoFsPerBody();
  for( unsigned i = 0; i < nbodies; ++i )
  {
    if( fsys.isKinematicallyScripted( i ) )
    {
      if( fsys.numVelDoFsPerBody() == 6 )
      {
        // 6-DoF rigid body in 3D
        if( ( dv.segment<3>( 3 * i ).array() != 0.0 ).any() )
        {
          return false;
        }
        if( ( dv.segment<3>( 3 * nbodies + 3 * i ).array() != 0.0 ).any() )
        {
          return false;
        }
      }
      else if( fsys.numVelDoFsPerBody() == 2 )
      {
        // 2-DoF ball in 2D
        if( ( dv.segment<2>( 2 * i ).array() != 0.0 ).any() )
        {
          return false;
        }
      }
      else
      {
        std::cerr << "Unhandled code path in ImpactFrictionMap::noImpulsesToKinematicGeometry. This is a bug." << std::endl;
        std::exit( EXIT_FAILURE );
      }
    }
  }
  return true;
}

static void getCollisionIndices( const Constraint& con, std::pair<int,int>& indices )
{
  con.getBodyIndices( indices );
  if( indices.second == -1 )
  {
    const unsigned static_object_index{ con.getStaticObjectIndex() };
    indices.second = - int( static_object_index ) - 2;
  }
}

void ImpactFrictionMap::exportConstraintForcesToBinaryFile( const VectorXs& q, const std::vector<std::unique_ptr<Constraint>>& constraints, const MatrixXXsc& contact_bases, const VectorXs& alpha, const VectorXs& beta, const scalar& dt, HDF5File& output_file )
{
  const unsigned ncons = constraints.size();
  assert( ncons == alpha.size() );
  assert( std::vector<std::unique_ptr<Constraint>>::size_type( ncons ) == constraints.size() );
  assert( alpha.size() == ncons );

  const unsigned ambient_space_dims{ static_cast<unsigned>( contact_bases.rows() ) };
  assert( ambient_space_dims == 2 || ambient_space_dims == 3 );
  assert( beta.size() == ncons * ( ambient_space_dims - 1 ) );

  // Write out the number of collisions
  output_file.writeScalar( "", "collision_count", ncons );

  // NB: Prior to version 1.8.7, HDF5 did not support zero sized dimensions.
  //     Some versions of Ubuntu still have old versions of HDF5, so workaround.
  // TODO: Remove once our servers are updated.
  if( ncons == 0 )
  {
    return;
  }

  // Write out the indices of all bodies involved
  {
    // Place all indices into a single matrix for output
    Matrix2Xic collision_indices{ 2, ncons };
    for( unsigned con = 0; con < ncons; ++con )
    {
      std::pair<int,int> indices;
      assert( constraints[con] != nullptr );
      getCollisionIndices( *constraints[con], indices );
      collision_indices.col( con ) << indices.first, indices.second;
    }
    // Output the indices
    output_file.writeMatrix( "", "collision_indices", collision_indices );
  }

  // Write out the world space contact points
  {
    // Place all contact points into a single matrix for output
    MatrixXXsc collision_points{ ambient_space_dims, ncons };
    for( unsigned con = 0; con < ncons; ++con )
    {
      VectorXs contact_point;
      assert( constraints[con] != nullptr );
      constraints[con]->getWorldSpaceContactPoint( q, contact_point );
      assert( contact_point.size() == ambient_space_dims );
      collision_points.col( con ) = contact_point;
    }
    // Output the points
    output_file.writeMatrix( "", "collision_points", collision_points );
  }

  // Write out the world space contact normals
  {
    // Place all contact normals into a single matrix for output
    MatrixXXsc collision_normals{ ambient_space_dims, ncons };
    for( unsigned con = 0; con < ncons; ++con )
    {
      collision_normals.col( con ) = contact_bases.col( ambient_space_dims * con );
    }
    #ifndef NDEBUG
    for( unsigned con = 0; con < ncons; ++con )
    {
      assert( fabs( collision_normals.col( con ).norm() - 1.0 ) <= 1.0e-6 );
    }
    #endif
    // Output the normals
    output_file.writeMatrix( "", "collision_normals", collision_normals );
  }

  // Write out the collision forces
  {
    // Place all contact forces into a single matrix for output
    MatrixXXsc collision_forces{ ambient_space_dims, ncons };
    for( unsigned con = 0; con < ncons; ++con )
    {
      // Contribution from normal
      collision_forces.col( con ) = alpha( con ) * contact_bases.col( ambient_space_dims * con );
      // Contribution from friction
      for( unsigned friction_sample = 0; friction_sample < ambient_space_dims - 1; ++friction_sample )
      {
        assert( ( ambient_space_dims - 1 ) * con + friction_sample < beta.size() );
        const scalar impulse{ beta( ( ambient_space_dims - 1 ) * con + friction_sample ) };

        const unsigned column_number{ ambient_space_dims * con + friction_sample + 1 };
        assert( column_number < contact_bases.cols() );
        assert( fabs( contact_bases.col( ambient_space_dims * con ).dot( contact_bases.col( column_number ) ) ) <= 1.0e-6 );

        collision_forces.col( con ) += impulse * contact_bases.col( column_number );
      }
    }
    // Output the forces
    output_file.writeMatrix( "", "collision_forces", collision_forces );
  }
}

bool ImpactFrictionMap::constraintSetShouldConserveMomentum( const std::vector<std::unique_ptr<Constraint>>& cons )
{
  return std::all_of( std::begin(cons), std::end(cons), [](const std::unique_ptr<Constraint>& con){ return con->conservesTranslationalMomentum(); } );
}

bool ImpactFrictionMap::constraintSetShouldConserveAngularMomentum( const std::vector<std::unique_ptr<Constraint>>& cons )
{
  return std::all_of( std::begin(cons), std::end(cons), [](const std::unique_ptr<Constraint>& con){ return con->conservesAngularMomentumUnderImpactAndFriction(); } );
}
