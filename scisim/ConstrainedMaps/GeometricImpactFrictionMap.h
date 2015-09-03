// GeometricImpactFrictionMap.h
//
// Breannan Smith
// Last updated: 09/03/2015

#ifndef GEOMETRIC_IMPACT_FRICTION_MAP_H
#define GEOMETRIC_IMPACT_FRICTION_MAP_H

#include "ImpactFrictionMap.h"

class Constraint;
class HDF5File;
class FrictionSolver;

class GeometricImpactFrictionMap final : public ImpactFrictionMap
{

public:

  GeometricImpactFrictionMap( const scalar& abs_tol, const unsigned max_iters, const bool external_warm_start_alpha, const bool external_warm_start_beta );
  GeometricImpactFrictionMap( std::istream& input_stream );

  virtual ~GeometricImpactFrictionMap() override = default;

  virtual void flow( ScriptingCallback& call_back, FlowableSystem& fsys, ConstrainedSystem& csys, UnconstrainedMap& umap, FrictionSolver& friction_solver, const unsigned iteration, const scalar& dt, const scalar& CoR_default, const scalar& mu_default, const VectorXs& q0, const VectorXs& v0, VectorXs& q1, VectorXs& v1 ) override;

  // Resets data used in warm starting to initial setting
  virtual void resetCachedData() override;

  virtual void serialize( std::ostream& output_stream ) const override;

  virtual std::string name() const override;

  virtual void exportForcesNextStep( HDF5File& output_file ) override;

private:

  void initializeImpulses( const std::vector<std::unique_ptr<Constraint>>& active_set, const VectorXs& q0, const SparseMatrixsc& D, ConstrainedSystem& csys, const int num_impulses_per_normal, const int ambient_space_dims, VectorXs& alpha, VectorXs& beta );
  void cacheImpulses( const std::vector<std::unique_ptr<Constraint>>& active_set, const VectorXs& q0, ConstrainedSystem& csys, const int num_impulses_per_normal, const int ambient_space_dims, const VectorXs& alpha, const VectorXs& beta );

  // For saving out constraint forces
  void exportConstraintForcesToBinary( const VectorXs& q, const std::vector<std::unique_ptr<Constraint>>& constraints, const MatrixXXsc& contact_bases, const VectorXs& alpha, const VectorXs& beta, const scalar& dt );

  // True -> staggered projections, False -> So-bogus
  bool m_use_staggered_projections;

  // Cached friction impulse from last solve
  VectorXs m_f;

  // SP solver controls
  scalar m_abs_tol;
  unsigned m_max_iters;
  // If true, initialize solve with alpha/beta from last time step, otherwise initialize alpha/beta to zero
  bool m_external_warm_start_alpha;
  bool m_external_warm_start_beta;

  // Temporary state for writing constraint forces
  bool m_write_constraint_forces;
  HDF5File* m_constraint_force_stream;

};

#endif
