#ifndef STATIC_CYLINDER_RENDERER_H
#define STATIC_CYLINDER_RENDERER_H

#include <QtOpenGL>

#include "scisim/Math/MathDefines.h"

class StaticCylinder;

class StaticCylinderRenderer final
{

public:

  StaticCylinderRenderer( const int idx, const scalar& length );

  void draw( const StaticCylinder& cylinder ) const;

  inline int idx() const
  {
    return m_idx;
  }

private:

  int m_idx;
  GLfloat m_L;

  Eigen::Matrix<GLfloat,Eigen::Dynamic,1> m_crds;

};

#endif
