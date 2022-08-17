#ifndef _covarianceOsc_h_
#define _covarianceOsc_h_

#include <TMatrixT.h>
#include <TMatrixDSym.h>
#include <TVectorT.h>
#include <TVectorD.h>
#include <TCanvas.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TTree.h>
#include <TF1.h>
#include <TFile.h>
#include <TAxis.h>
#include <TRandom3.h>
#include <vector>
#include "covarianceBase.h"

class covarianceOsc : public covarianceBase
{
 public:
      
  covarianceOsc(const char* name, const char *file, TH2D *hist_dcpNH=NULL, TH2D *hist_dcpIH=NULL, TH2D *hist_23=NULL);//TMatrixDSym *cov);
      virtual ~covarianceOsc();
  
      void throwNominal(bool nomValues=true);
      double getLikelihood();
      bool checkBounds();
      double *getPropPars();//double *retrn);
      void proposeStep();
      std::vector<double> defaultPars(bool doubled);
      void setFlipDeltaM23(bool flip){flipdelM=flip;}
      void setFlipDeltaM23(double dm23NH, double dm23IH, double th23NH, double th23IH);
      void setFlipBeta(bool flip){flipBeta=flip;}
      void useReactorPrior(bool reactor){reactorPrior = reactor;};
      void setExtraBranches(TTree &tree);

 protected:
      double L, density;
      TVectorD* osc_prior;
      bool flipdelM;
      bool reactorPrior;
      bool flipBeta;
      TH2D *h_dcpth13NH;
      TH2D *h_dcpth13IH;
      TH2D *h_23;
      double fixdm23NH, fixdm23IH;
      double fixth23NH, fixth23IH;
      
      double *oscpars1;
};

#endif