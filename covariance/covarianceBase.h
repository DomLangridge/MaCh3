#ifndef _covarianceBase_h_
#define _covarianceBase_h_

// ROOT includes
#include "TMatrixT.h"
#include "TMatrixDSym.h"
#include "TVectorT.h"
#include "TVectorD.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TTree.h"
#include "TFile.h"
#include "TAxis.h"
#include "TRandom3.h"
#include "TMath.h"
#include "math.h"
#include "TDecompChol.h"
#include "TStopwatch.h"
#include "TMatrixDSymEigen.h"
#include "TMatrixDEigen.h"
#include "TDecompSVD.h"

// MaCh3 includes
#include "samplePDF/Structs.h"
#include "throwParms/ThrowParms.h"

// Don't forget yaml!
#include "yaml-cpp/yaml.h"

#ifdef MULTITHREAD
#include "omp.h"
#endif

#ifndef __LARGE_LOGL__
#define __LARGE_LOGL__ 1234567890
#endif

//#define DEBUG_PCA
#ifdef DEBUG_PCA
//KS: When debuging we produce some fancy plots, but we don't need it during normal work flow
#include "TROOT.h"
#include "TStyle.h"
#include "TColor.h"
#include "TLine.h"
#include "TText.h"
#include "TLegend.h"

#if DEBUG_PCA == 2
#include "Eigen/Eigenvalues"
#endif

#endif


class covarianceBase {
 public:
  // The constructors
  covarianceBase(){};
  //ETA - construcotr for a YAML file
  covarianceBase(const char *YAMLFile);
  //"Usual" constructors from root file
  covarianceBase(const char *name, const char *file);
  covarianceBase(const char *name, const char *file, int seed);
  // For Eigen Value decomp
  covarianceBase(const char *name, const char *file, int seed, double threshold,int FirstPCAdpar, int LastPCAdpar);
  virtual ~covarianceBase();
  
  // Setters
  // need virtual for eRPA parameter over-ride
  void setCovMatrix(TMatrixDSym *cov);
  void setName(const char *name) { matrixName = name; }
  virtual void setParName(int i, char *name) { fParNames[i] = name; }
  void setSingleParameter(const int parNo, const double parVal);
  void setPar(const int i, const double val);
  void setParCurrProp(int i, double val);
  void setParProp(int i, double val) {
    std::cout << "Setting " << getParName(i) << " to " << val << std::endl; 
    fParProp[i] = val;
    if (pca) TransferToPCA();
  };
  void setParameters(std::vector<double> pars = std::vector<double>());    
  virtual void setEvalLikelihood(int i, bool eL);
  
  // set branches for output file
  void setBranches(TTree &tree);
  void setStepScale(double scale);

  //DB Function to set fIndivStepScale from a vector (Can be used from execs and inside covariance constructors)
  void setIndivStepScale(std::vector<double> stepscale);
  //KS: In case someone really want to change this
  void setPrintLength(unsigned int PriLen) { PrintLength = PriLen; };

  // set a custom proposal function
  //DEPRECATED
  void setPropFunct(int i, TF1 *func);

  // Throwers
  void throwParProp(const double mag = 1.);
  void throwParCurr(const double mag = 1.);
  void throwParameters();
  void throwNominal(bool nomValues=false, int seed = 0);
  // Randomly throw the parameters in their 1 sigma range
  void RandomConfiguration();
  
  // Getters
  TMatrixDSym *getCovMatrix() { return covMatrix; };
  TMatrixDSym *getInvCovMatrix() { return invCovMatrix; };
  bool getEvalLikelihood(const int i) { return fParEvalLikelihood[i]; };

  virtual double getLikelihood();
  virtual int CheckBounds();
  double calcLikelihood();

  const char *getName() { return matrixName; };
  virtual const char* getParName(const int i) const {
    return fParNames[i];
  };
  std::string const getInputFile() const { return inputFile; };

  // Get diagonal error for ith parameter
  double getDiagonalError(const int i) { 
    return sqrt((*covMatrix)(i,i));
  }


  // Adaptive Step Tuning Stuff
  void resetIndivStepScale();
  void useSeparateThrowMatrix(TString throwMatrixName, TString throwMatrixFile, TString parameterMeansName="");
  void useSeparateThrowMatrix();
  void saveAdaptiveToFile(TString outFileName, TString systematicName);

  void setThrowMatrix(TMatrixDSym *cov);
  void updateThrowMatrix(TMatrixDSym *cov);
  void setNumberOfSteps(const int nsteps){
    total_steps=nsteps;
    if(total_steps>=lower_adapt)resetIndivStepScale();
  }
  // Set thresholds for MCMC steps
  void setAdaptiveThresholds(int low_threshold=10000, int up_threshold=1000000){lower_adapt=low_threshold; upper_adapt=up_threshold;}

  TMatrixDSym *getThrowMatrix(){return throwMatrix;}
  TMatrixD *getThrowMatrix_CholDecomp(){return throwMatrix_CholDecomp;}
  std::vector<double> getParameterMeans(){return par_means;}


  //========
  //DB Pointer return
  //========
  const double* retPointer(int iParam) {return &fParProp[iParam];}

  virtual std::vector<double> getNominalArray();
  const std::vector<double> getProposed() const;
  double getParProp(const int i) {
    return fParProp[i]; 
  };
  double getParCurr(const int i) {
    return fParCurr[i];
  };
  double getParInit(const int i) {
    return fParInit[i];
  };
  virtual double getNominal(const int i) {
    return getParInit(i);
  };
  double getParProp_PCA(const int i) {
    if (!pca) {
      std::cerr << "Am not running in PCA mode" << std::endl;
      throw;
    }
    return fParProp_PCA(i);
  };
  double getParCurr_PCA(const int i) {
    if (!pca) {
      std::cerr << "Am not running in PCA mode" << std::endl;
      throw;
    }
    return fParCurr_PCA(i);
  };

  const TMatrixD getTransferMatrix() {
    if (!pca) {
      std::cerr << "Am not running in PCA mode" << std::endl;
      throw;
    }
    return TransferMat;
  }

  const TMatrixD getEigenVectors() {
    if (!pca) {
      std::cerr << "Am not running in PCA mode" << std::endl;
      throw;
    }
    return eigen_vectors;
  }

  const TVectorD getEigenValues() {
    if (!pca) {
      std::cerr << "Am not running in PCA mode" << std::endl;
      throw;
    }
    return eigen_values;
  }

  void setParProp_PCA(const int i, const double value) {
    if (!pca) {
      std::cerr << "Am not running in PCA mode" << std::endl;
      throw;
    }
    fParProp_PCA(i) = value;
    // And then transfer back to the parameter basis
    TransferToParam();
  }

  void setParCurr_PCA(const int i, const double value) {
    if (!pca) {
      std::cerr << "Am not running in PCA mode" << std::endl;
      throw;
    }
    fParCurr_PCA(i) = value;
    // And then transfer back to the parameter basis
    TransferToParam();
  }

  int getSize() { return size; };
  int getNpars() { 
    if (pca) return npars;
    else return size;
  }

  // Printers
  void printNominal();
  void printNominalCurrProp();
  void printPars();
  void printIndivStepScale();

  // Steppers
  virtual void proposeStep(); // generate a new proposed state
  void acceptStep(); // accepted this step

  // fix parameters at nominal values
  void toggleFixAllParameters();
  virtual void toggleFixParameter(const int i);
  bool isParameterFixed(const int i) {
    if (fParSigma[i] < 0) {
      return true;
    } else {
      return false;
    }
  }
  void ConstructPCA();
#ifdef DEBUG_PCA
  void DebugPCA(const double sum, TMatrixD temp, TMatrixDSym submat);
#endif
  // is PCA, can use to query e.g. LLH scans
  bool IsPCA() { return pca; };

  double* MatrixMult(double*, double*, int);
  double** MatrixMult(double**, double**, int);
  TMatrixD MatrixMult(TMatrixD, TMatrixD);
  inline void MatrixVectorMulti(double* VecMulti, double** matrix, const double* vector, const int n);
  inline double MatrixVectorMultiSingle(double** matrix, const double* vector, const int Length, const int i);

  //Turn on/off true adaptive MCMC
  //Also set thresholds for use (having a lower threshold gives us some data to adapt from!)
  void enableAdaptiveMCMC(bool enable=true){
    use_adaptive=enable;
    total_steps=0; //Set these to default values
    lower_adapt=10000;
    upper_adapt=10000000;
  }
  void updateAdaptiveCovariance();

 protected:
  void init(const char *name, const char *file);
  //YAML init
  void init(const char *YAMLFile);
  void init(TMatrixDSym* covMat);

  void MakePosDef(TMatrixDSym *cov = NULL);
  void makeClosestPosDef(TMatrixDSym *cov);
  void TransferToPCA();
  void TransferToParam();

  //Handy function to return 1 for any systs
  const double* ReturnUnity(){return &Unity;}

  // The input root file we read in
  const std::string inputFile;

  int size;
  // Name of cov matrix
  const char *matrixName;
  // The covariance matrix
  TMatrixDSym *covMatrix;
  // The inverse covariance matrix
  TMatrixDSym *invCovMatrix;
  //KS: Same as above but much faster as TMatrixDSym cache miss
  double **InvertCovMatrix;
    
  //KS: set Random numbers for each thread so each thread has differnt seed
  TRandom3 **random_number;

  // For Cholesky decomposed parameter throw
  double* randParams;
  //  TMatrixD *chel;
  double fStepScale;

  //KS: This is used when printing parameters, sometimes we have super long parmaeters name, we want to flexibly adjust couts
  unsigned int PrintLength;

  // state info (arrays of each parameter)
  Char_t  **fParNames;
  Double_t *fParInit;
  Double_t *fParCurr;
  Double_t *fParProp;
  Double_t *fParSigma;
  Double_t *fParLoLimit;
  Double_t *fParHiLimit;
  Double_t *fIndivStepScale;
  bool     *fParEvalLikelihood;
  Double_t currLogL;
  Double_t propLogL;
  Double_t *corr_throw;

  //ETA - duplication of some of these
  //ideally these should all be private and we have setters be protected 
  //setters and public getters
  std::vector<std::string> _fNames;
  int _fNumPar;
  YAML::Node _fYAMLDoc;

  std::vector<double> _fPreFitValue;
  std::vector<double> _fGenerated;
  std::vector<double> _fError;
  std::vector<double> _fLB;
  std::vector<double> _fUB;
  std::vector<int> _fDetID;
  std::vector<std::string> _fDetString;
  std::vector<std::string> _fParamType;
  std::vector<double> _fStepScale;
  std::vector<bool> _fFlatPrior;
  TMatrixDSym *_fCovMatrix;
  //TMatrixT<double> *_fCovMatrix;

  std::vector<int> _fNormModes;
  std::vector<std::string> _fNDSplineNames;
  std::vector<std::string> _fFDSplineNames;
  std::vector<std::vector<int>> _fFDSplineModes;
//  std::vector<std::string> _fKinematicPars;
//  std::vector<std::vector<int>> _fKinematicBounds;

  std::vector<std::vector<std::string>> _fKinematicPars;
  std::vector<std::vector<std::vector<double>>> _fKinematicBounds;

  //Unity for null systs to point back to
  const double Unity = 1.0;

  // PCA
  bool pca;
  double eigen_threshold;
  int npars;
  int FirstPCAdpar;
  int LastPCAdpar;
  int nKeptPCApars;
  TVectorD eigen_values;
  TMatrixD eigen_vectors;
  std::vector<double> eigen_values_master;
  TMatrixD TransferMat;
  TMatrixD TransferMatT;
  // Also save current and proposed parameter in PCA basis
  TVectorD fParProp_PCA;
  TVectorD fParCurr_PCA;
  std::vector<double> fParSigma_PCA;
  std::vector<int> isDecomposed_PCA;

  TMatrixDSym* throwMatrix;
  TMatrixD* throwMatrix_CholDecomp;
  //Same as above but much faster as TMatrixDSym cache miss
  double **throwMatrixCholDecomp;

  void randomize();
  void CorrelateSteps();

  // Truely Adaptive Stuff
  void initialiseNewAdaptiveChain();
  // TMatrixD* getMatrixSqrt(TMatrixDSym* inputMatrix);
  // double calculateSubmodality(TMatrixD* sqrtVectorCov, TMatrixDSym* throwCov);
  bool use_adaptive;
  int total_steps;
  int lower_adapt, upper_adapt; //Thresholds for when to turn on/off adaptive MCMC
  // TMatrixD* covSqrt;
  std::vector<double> par_means;
  std::vector<double> par_means_prev;
  TMatrixDSym* adaptiveCovariance;
};

TH2D* TMatrixIntoTH2D(const TMatrix &Matrix, std::string title);
#endif
