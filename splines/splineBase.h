#ifndef _splineBase_h_
#define _splineBase_h_

#include "splineInterface.h"
#include <cstdlib>

class splineBase : public splineInterface
{
 public:
  splineBase(int nutype);
  virtual ~splineBase();
  virtual void setupSplines(){};
      
 protected:
  int nutype; // 2 = numu/signue | -2 = numub | 1 = nue | -1 = nueb
  TFile *splinefile;
  //  setupSpline(char *spline);
  
};

#endif
