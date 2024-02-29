#pragma once

// C++ includes
#include <math.h>
#include <map>

// ROOT includes
#include "TList.h"

// MaCh3 includes
#include "covariance/covarianceBase.h"
#include "samplePDF/Structs.h"

#include "yaml-cpp/yaml.h"

class covarianceXsec : public covarianceBase {

  public:
  covarianceXsec(const char *name, const char *file, double threshold=-1,int FirstPCAdpar=-999,int LastPCAdpar=-999);
  covarianceXsec(const char *YAMLFile);
    ~covarianceXsec();

    // Print information about the whole object once it is set
    void Print();

    // General Getter functions not split by detector
	// ETA - a lot of these can go... they're just duplications from the base
	// class.
    double GetParamPrior(const int i)      {return xsec_param_prior_a[i];}
    //const int  GetXSecParamID(const int i, const int j) const {return xsec_param_id_a[i][j];}
	//ETA - just return the int of the DetID, this can be removed to do a string comp
	//at some point.
    int  GetXsecParamDetID(const int i) const {return _fDetID[i];}
	//ETA - just return a string of "spline", "norm" or "functional"
    const char*  GetXsecParamType(const int i) const {return _fParamType[i].c_str();}

	//ETA - trying out the yaml parsing
	void ParseYAML(const char* FileName);

    bool IsParFlux(const int i){
      return isFlux[i];
    }

	//ETA - these can be removed as Near params will be given by DetID so this is defunct.
    // Get functions for Near normalisation parameters
    const std::vector<XsecNorms4> GetNearNormPars() const{return NearNormParams;}
    int                     GetNumNearNormParams() const  {return nNearNormParams;}

    // Get functions for Far normalisation parameters
    const std::vector<XsecNorms4> GetFarNormPars() const{return FarNormParams;}

    // Get functions for Near spline parameters
    int                       GetNumNearSplineParams() const  {return nNearSplineParams;}
    const std::vector<std::string>& GetNearSplineParsNames() const  {return NearsplineParsNames;}
    const std::vector<std::string>& GetNearSplineFileParsNames() const  {return _fNDSplineNames;}
    const std::vector<int>&         GetNearSplineParsIndex() const  {return NearsplineParsIndex;}
    const std::vector<SplineInterpolation> GetSplineInterpolation() const{return _fSplineInterpolationType;}
    SplineInterpolation GetParSplineInterpolation(int i) {return _fSplineInterpolationType[i];}

    //DB Get spline parameters depending on given DetID
    const std::vector<std::string> GetSplineParsNamesFromDetID(int DetID);
    const std::vector<std::string> GetSplineFileParsNamesFromDetID(int DetID);
    const std::vector<std::string> GetFDSplineFileParsNamesFromDetID(int DetID);
    const std::vector<std::string> GetNDSplineFileParsNamesFromDetID(int DetID);
    const std::vector< std::vector<int> > GetSplineModeVecFromDetID(int DetID);
    const std::vector<int> GetSplineParsIndexFromDetID(int DetID);
    int GetNumSplineParamsFromDetID(int DetID);

    //DB Get norm/func parameters depending on given DetID
    const std::vector<XsecNorms4> GetNormParsFromDetID(int DetID);
    int GetNumFuncParamsFromDetID(int DetID);
    const std::vector<std::string> GetFuncParsNamesFromDetID(int DetID);
    const std::vector<int> GetFuncParsIndexFromDetID(int DetID);

    // Get functions for Far spline parameters
    int                       GetNumFarSplineParams() const  {return nFarSplineParams;}
    const std::vector<std::string>& GetFarSplineParsNames() const  {return FarSplineParsNames;}
    const std::vector<std::string>& GetFarSplineFileParsNames() const  {return FarSplineFileParsNames;}
    //TVectorT<double> *GetFarSplineParsModes_TVec() {return FarSplineParsModes;}
    //const double GetFarSplineParsModes(int i) {return xsec_fd_spline_mode_a[i];}
    const std::vector<int>&         GetFarSplineParsIndex() const  {return FarSplineParsIndex;}
    const std::vector<int>&         GetFarSplineModeVec(int i) const {return FarSplineModes[i];}
	
    // Get functions for uniq spline parameters //!!decide what to do about these
    int                       GetNumSplineParamsUniq() const  {return nSplineParamsUniq;}
    const std::vector<std::string>& GetSplineParsUniqNames() const  {return splineParsUniqNames;}
    const std::vector<int>&         GetSplineParsUniqIndex() const  {return splineParsUniqIndex;}

    // Get functions for shared spline parameters //!!decide what to do about these
    int                       GetNumSplineParamsShare() const  {return nSplineParamsShare;}
    const std::vector<std::string>& GetSplineParsShareNames() const  {return splineParsShareNames;}
    const std::vector<int>&         GetSplineParsShareIndex() const  {return splineParsShareIndex;}
    const std::vector<int>&         GetSplineParsShareToUniq() const {return splineParsShareToUniq;}
    
    // Get functions for Near functional parameter (e.g. BeRPA)
    int                       GetNumNearFuncParams() const     {return nNearFuncParams;}
    const std::vector<std::string>& GetNearFuncParsNames() const  {return NearfuncParsNames;}
    const std::vector<int>&         GetNearFuncParsIndex() const  {return NearfuncParsIndex;}

    // Get functions for Far functional parameter (e.g. BeRPA)
    int                       GetNumFarFuncParams() const     {return nFarFuncParams;}
    const std::vector<std::string>& GetFarFuncParsNames() const  {return FarFuncParsNames;}
    const std::vector<int>&         GetFarFuncParsIndex() const  {return FarFuncParsIndex;}

    //KS: For most covariances nominal and fparInit (prior) are the same, however for Xsec those can be differrent
    // For example Sigma Var are done around nominal in ND280, no idea why though...
    std::vector<double> getNominalArray()
    {
      std::vector<double> nominal;
      for (int i = 0; i < size; i++)
      {
        nominal.push_back(_fPreFitValue.at(i));
      }
      return nominal;
    }

    const double getNominal(const int i)
    {
      return _fPreFitValue.at(i);
    };

    // Get nominal and prior for saving to the output file
    //TVectorT<double> *GetNominal_TVec() {return xsec_param_nom;}
    //TVectorT<double> *GetPrior_TVec()   {return xsec_param_prior;}
    
    //KS Function to set to nominal either flux or xsec parmeters
    void setXsecOnlyParameters();
    void setFluxOnlyParameters();
    
    // What parameter Gets reweighted by what amount according to MCMC
    inline double calcReWeight(const int bin){
	  if (bin >= 0 && bin < _fNumPar) {
		return _fPropVal[bin];
	  } else {
		std::cerr << "Specified bin is <= 0 OR bin > npar!" << std::endl;
		std::cerr << "bin = " << bin << ", npar = " << _fNumPar << std::endl;
		std::cerr << "This won't ruin much that this step in the MCMC, but does indicate something wrong in memory!" << std::endl;
		return 1.0;
	  }

	  return 1.0;  
	}; 

  protected:
    // Helper functions to decide on what setup we're running
    void ScanParameters();

    void initParams(double fScale);
    void setXsecParNames();

    //DB StepScaleReading
    TVectorT<double> *xsec_stepscale;
    std::vector<double> xsec_stepscale_vec;

    // Here are some array equivalents (actually used in MCMC)
    // nominal values in MC
    double *xsec_param_nom_a;
    // lower bound
    double *xsec_param_lb_a;
    // upper bound
    double *xsec_param_ub_a;
    // priors from external data fit
    double *xsec_param_prior_a;
    
    std::vector<bool> isFlux;

    int nTotalNormParams;
    int nFaronlyNormParams;//Needed for consistency check
	//ETA - we can maybe remove these for now
    int nLowEnergyAtmOnlyNormParams;//DB Needed for consistency check
    int nHighEnergyAtmOnlyNormParams;//DB Needed for consistency check

    std::vector<XsecNorms4> FarNormParams;
    int nFarNormParams;

    std::vector<XsecNorms4> NearNormParams;
    int nNearNormParams;

    // Number of Near spline parameters
    int nNearSplineParams;
    std::vector<std::string> NearsplineParsNames;
    std::vector<std::string> NearSplineFileParsNames;
    std::vector<int> NearsplineParsIndex;
    std::vector<SplineInterpolation> _fSplineInterpolationType;

    int nFarSplineParams;
    std::vector<std::string> FarSplineParsNames;
    std::vector<int> FarSplineParsIndex;
    //ETA - new object for storing spline name in File
    std::vector<std::string> FarSplineFileParsNames;
    //Mode which spline applies to
    //mode=12 (which is kMaCh3_nModes for NIWG2020 model) means it applies to all modes
    //TVectorT<double> *FarSplineParsModes;
    std::vector<std::vector<int> > FarSplineModes;

    // Number of spline parameters that aren't repeated
    int nSplineParamsUniq;
    std::vector<std::string> splineParsUniqNames;
    std::vector<int> splineParsUniqIndex;

    // Number of spline parameters that are shared
    int nSplineParamsShare;
    std::vector<std::string> splineParsShareNames;
    std::vector<int> splineParsShareIndex;
    std::vector<int> splineParsShareToUniq;

	// ETA - again, these can be removed soon as we get this info from a check
	// against DetID
    // Number of functional parameter
    int nNearFuncParams;
    std::vector<std::string> NearfuncParsNames;
    std::vector<int> NearfuncParsIndex;

    int nFarFuncParams;
    std::vector<std::string> FarFuncParsNames;
    std::vector<int> FarFuncParsIndex;

  private:
	//ETA - do we need these now?
	// it would be nice if we could get rid of these
	std::vector<std::string> _fNDSplineNames;
	std::vector<std::string> _fFDSplineNames;
	std::vector<std::vector<int>> _fFDSplineModes;
	//A vector to store the type of interpolation used for each systematic

	int _nNormPars;
};

