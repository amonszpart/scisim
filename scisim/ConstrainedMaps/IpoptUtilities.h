// IpoptUtilities.h
//
// Breannan Smith
// Last updated: 09/03/2015

#ifndef IPOPT_UTILITIES_H
#define IPOPT_UTILITIES_H

#ifdef IPOPT_FOUND
#include "IpIpoptApplication.hpp"
#endif

#include <iosfwd>
#include <vector>

namespace IpoptUtilities
{

  // TODO: Replace string parameter to Ipopt with enum class of valid options, thus obviating the need for this function.
  bool linearSolverSupported( const std::string& linear_solver_name );

  bool containsDuplicates( const std::vector<std::string>& linear_solvers );

  #ifdef IPOPT_FOUND
  std::string ipoptReturnStatusToString( const Ipopt::SolverReturn& status );
  #endif

}

#endif
