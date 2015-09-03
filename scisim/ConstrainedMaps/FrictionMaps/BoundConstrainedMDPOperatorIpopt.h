// BoundConstrainedMDPOperatorIpopt.h
//
// Breannan Smith
// Last updated: 09/03/2015

#ifndef BOUND_CONSTRAINED_MDP_OPERATOR_IPOPT_H
#define BOUND_CONSTRAINED_MDP_OPERATOR_IPOPT_H

#include "FrictionOperator.h"

#ifdef IPOPT_FOUND
#include "IpIpoptApplication.hpp"
#endif

#include "SCISim/ConstrainedMaps/QPTerminationOperator.h"

// Solves the problem:
//   1/2 beta^T Q beta + beta^T ( D^T v0 + gdotD  )
//    s.t. - diag(mu) alpha <= beta <= diag(mu) alpha

class BoundConstrainedMDPOperatorIpopt final : public FrictionOperator
{

public:

  BoundConstrainedMDPOperatorIpopt( const std::vector<std::string>& linear_solvers, const scalar& tol );
  BoundConstrainedMDPOperatorIpopt( std::istream& input_stream );
  BoundConstrainedMDPOperatorIpopt( const BoundConstrainedMDPOperatorIpopt& other );
  virtual ~BoundConstrainedMDPOperatorIpopt() override = default;

  virtual void flow( const scalar& t, const SparseMatrixsc& Minv, const VectorXs& v0, const SparseMatrixsc& D, const SparseMatrixsc& Q, const VectorXs& gdotD, const VectorXs& mu, const VectorXs& alpha, VectorXs& beta, VectorXs& lambda ) override;

  virtual int numFrictionImpulsesPerNormal() const override;

  virtual void formGeneralizedFrictionBasis( const VectorXs& q, const VectorXs& v, const std::vector<std::unique_ptr<Constraint>>& K, SparseMatrixsc& D, VectorXs& drel ) override;

  virtual std::string name() const override;

  virtual std::unique_ptr<FrictionOperator> clone() const override;

  virtual void serialize( std::ostream& output_stream ) const override;

  virtual bool isLinearized() const override;

  #ifndef IPOPT_FOUND
  [[noreturn]]
  #endif
  void solveQP( const QPTerminationOperator& termination_operator, const SparseMatrixsc& Minv, const SparseMatrixsc& D, const VectorXs& b, const VectorXs& c, VectorXs& beta, VectorXs& lambda, scalar& achieved_tol ) const;

private:

  const std::vector<std::string> m_linear_solver_order;
  const scalar m_tol;

};

#ifdef IPOPT_FOUND
class BoundConstrainedMDPNLP final : public Ipopt::TNLP
{

public:

  BoundConstrainedMDPNLP( const SparseMatrixsc& Q, VectorXs& beta, const bool use_custom_termination = false, const QPTerminationOperator& termination_operator = QPTerminationOperator() );
  virtual ~BoundConstrainedMDPNLP() override;

  // Method to return some info about the nlp
  virtual bool get_nlp_info( Ipopt::Index& n, Ipopt::Index& m, Ipopt::Index& nnz_jac_g, Ipopt::Index& nnz_h_lag, TNLP::IndexStyleEnum& index_style ) override;

  // Method to return the bounds for the nlp
  virtual bool get_bounds_info( Ipopt::Index n, Ipopt::Number* x_l, Ipopt::Number* x_u, Ipopt::Index m, Ipopt::Number* g_l, Ipopt::Number* g_u ) override;

  // Method to return the starting point for the algorithm
  virtual bool get_starting_point( Ipopt::Index n, bool init_x, Ipopt::Number* x, bool init_z, Ipopt::Number* z_L, Ipopt::Number* z_U, Ipopt::Index m, bool init_lambda, Ipopt::Number* lambda ) override;

  // Method to return the objective value
  virtual bool eval_f( Ipopt::Index n, const Ipopt::Number* x, bool new_x, Ipopt::Number& obj_value ) override;

  // Method to return the gradient of the objective
  virtual bool eval_grad_f( Ipopt::Index n, const Ipopt::Number* x, bool new_x, Ipopt::Number* grad_f ) override;

  // Method to return the constraint residuals
  virtual bool eval_g( Ipopt::Index n, const Ipopt::Number* x, bool new_x, Ipopt::Index m, Ipopt::Number* g ) override;

  // Method to return the jacobian of the constraint residuals
  virtual bool eval_jac_g( Ipopt::Index n, const Ipopt::Number* x, bool new_x, Ipopt::Index m, Ipopt::Index nele_jac, Ipopt::Index* iRow, Ipopt::Index* jCol, Ipopt::Number* values ) override;

  virtual bool eval_h( Ipopt::Index n, const Ipopt::Number* x, bool new_x, Ipopt::Number obj_factor, Ipopt::Index m, const Ipopt::Number* lambda, bool new_lambda, Ipopt::Index nele_hess, Ipopt::Index* iRow, Ipopt::Index* jCol, Ipopt::Number* values ) override;

  // This method is called when the algorithm is complete so the TNLP can store/write the solution
  virtual void finalize_solution( Ipopt::SolverReturn status, Ipopt::Index n, const Ipopt::Number* x, const Ipopt::Number* z_L, const Ipopt::Number* z_U, Ipopt::Index m, const Ipopt::Number* g, const Ipopt::Number* lambda, Ipopt::Number obj_value, const Ipopt::IpoptData* ip_data, Ipopt::IpoptCalculatedQuantities* ip_cq ) override;

  virtual bool intermediate_callback( Ipopt::AlgorithmMode mode, Ipopt::Index iter, Ipopt::Number obj_value, Ipopt::Number inf_pr, Ipopt::Number inf_du, Ipopt::Number mu, Ipopt::Number d_norm, Ipopt::Number regularization_size, Ipopt::Number alpha_du, Ipopt::Number alpha_pr, Ipopt::Index ls_trials, const Ipopt::IpoptData* ip_data, Ipopt::IpoptCalculatedQuantities* ip_cq ) override;

  Ipopt::SolverReturn getReturnStatus() const;

  inline VectorXs& A()
  {
    return m_A;
  }

  inline VectorXs& C()
  {
    return m_C;
  }

  inline VectorXs& beta()
  {
    return m_beta;
  }

  inline const VectorXs& lambda() const
  {
    return m_lambda;
  }

  inline const scalar& achievedTolerance() const
  {
    return m_achieved_tolerance;
  }

private:

  // Quadratic term in objective. Q in: 1/2 beta^T Q beta
  const SparseMatrixsc& m_Q;
  // Linear termin in objective: A = D^T v0 + gdotD
  VectorXs m_A;
  // Constraint on bounds: - diag(mu) alpha
  VectorXs m_C;
  // Solution
  VectorXs& m_beta;
  // Constrain multipliers
  VectorXs m_lambda;

  // Return status from the last solve
  Ipopt::SolverReturn m_solve_return_status;

  const bool m_use_custom_termination;
  const QPTerminationOperator& m_termination_operator;
  scalar m_achieved_tolerance;

  BoundConstrainedMDPNLP( const BoundConstrainedMDPNLP& );
  BoundConstrainedMDPNLP& operator=( const BoundConstrainedMDPNLP& );

};
#endif

#endif
