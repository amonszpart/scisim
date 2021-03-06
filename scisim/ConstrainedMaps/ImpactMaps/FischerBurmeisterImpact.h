// FischerBurmeisterImpact.h
//
// Breannan Smith
// Last updated: 09/21/2015

#ifndef FISCHER_BURMEISTER_IMPACT_H
#define FISCHER_BURMEISTER_IMPACT_H

#include "scisim/ConstrainedMaps/QPTerminationOperator.h"
#include "scisim/Math/MathDefines.h"

class FischerBurmeisterImpact final : public QPTerminationOperator
{

public:

  explicit FischerBurmeisterImpact( const scalar& tol );
  virtual ~FischerBurmeisterImpact() override;

  virtual scalar operator()( const VectorXs& alpha, const VectorXs& grad_objective ) const override;

  virtual scalar tol() const override;

private:

  const scalar m_tol;

};

#endif
