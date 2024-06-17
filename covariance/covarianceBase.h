#pragma once

// MaCh3 includes
#include "samplePDF/Structs.h"
#include "covariance/CovarianceUtils.h"
#include "covariance/ThrowParms.h"
#include "manager/manager.h"

// Don't forget yaml!
#include "yaml-cpp/yaml.h"
#include "manager/MaCh3Logger.h"
#include <vector>

#ifndef _LARGE_LOGL_
#define _LARGE_LOGL_ 1234567890.0
/// Large Likelihood is used it parameter go out of physical boundary, this indicates in MCMC that such step should eb removed
#endif

//#define DEBUG_PCA 1
#ifdef DEBUG_PCA
//KS: When debugging we produce some fancy plots, but we don't need it during normal work flow
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

/// @brief Base class responsible for handling of systematic error parameters. Capable of using PCA or using adaptive throw matrix
class covarianceBase {
 public:
  /// @brief ETA - constructor for a YAML file
  covarianceBase(std::vector<std::string> YAMLFile, const char *name, double threshold = -1, int FirstPCAdpar = -999, int LastPCAdpar = -999);
  /// @brief "Usual" constructors from root file
  covarianceBase(const char *name, const char *file);
  /// @brief "Usual" constructors from root file with seed
  covarianceBase(const char *name, const char *file, int seed);
  /// @brief Constructor For Eigen Value decomp
  covarianceBase(const char *name, const char *file, int seed, double threshold, int FirstPCAdpar, int LastPCAdpar);
  /// @brief Destructor
  virtual ~covarianceBase();
  
  // Setters
  // ETA - maybe need to add checks to index on the setters? i.e. if( i > _fPropVal.size()){throw;}
  void setCovMatrix(TMatrixDSym *cov);
  void setName(const char *name) { matrixName = name; }
  void setParName(int i, char *name) { _fNames.at(i) = std::string(name); }
  void setSingleParameter(const int parNo, const double parVal);
  /// @brief Set all the covariance matrix parameters to a user-defined value
  void setPar(const int i, const double val);
  /// @brief Set current parameter value
  void setParCurrProp(const int i, const double val);
  /// @brief Set proposed parameter value
  void setParProp(const int i, const double val) {
    _fPropVal[i] = val;
    if (pca) TransferToPCA();
  };

  // TODO : Make the error part of the logger
  void setParPropVec(std::vector<double> new_prop){
    if(new_prop.size() != (unsigned)getNpars()){
      std::cerr<<"ERROR::Cannot set vector with size "<<new_prop.size()<<std::endl;
      throw;
    }
    _fPropVal=new_prop;
  }

  void setParCurrVec(std::vector<double> new_curr){
    if(new_curr.size() != (unsigned)getNpars()){
      std::cerr<<"ERROR::Cannot set vector with size "<<new_curr.size()<<std::endl;
      throw;
    }
    _fCurrVal=new_curr;
 }

  void setParameters(std::vector<double> pars = std::vector<double>());
  void setEvalLikelihood(int i, bool eL);
  
  /// @brief set branches for output file
  void SetBranches(TTree &tree, bool SaveProposal = false);
  /// @brief Set global step scale for covariance object
  void setStepScale(double scale);
  /// @brief DB Function to set fIndivStepScale from a vector (Can be used from execs and inside covariance constructors)
  void setIndivStepScale(const int ParameterIndex, const double StepScale){ _fIndivStepScale.at(ParameterIndex) = StepScale; }
  /// @brief DB Function to set fIndivStepScale from a vector (Can be used from execs and inside covariance constructors)
  void setIndivStepScale(std::vector<double> stepscale);
  /// @brief KS: In case someone really want to change this
  inline void setPrintLength(const unsigned int PriLen) { PrintLength = PriLen; }

  /// @brief KS: After step scale, prefit etc. value were modified save this modified config.
  void SaveUpdatedMatrixConfig();

  /// @brief Throw the proposed parameter by mag sigma. Should really just have the user specify this throw by having argument double
  void throwParProp(const double mag = 1.);

  /// @brief Helper function to throw the current parameter by mag sigma. Can study bias in MCMC with this; put different starting parameters
  void throwParCurr(const double mag = 1.);
  /// @brief Throw the parameters according to the covariance matrix. This shouldn't be used in MCMC code ase it can break Detailed Balance;
  void throwParameters();
  /// @brief Throw nominal values
  void throwNominal(bool nomValues = false, int seed = 0);
  /// @brief Randomly throw the parameters in their 1 sigma range
  void RandomConfiguration();
  
  /// @brief Check if parameters were proposed outside physical boundary
  virtual int CheckBounds();
  // @brief Calculates LogL based on an abritrary gaussian
  double calcGaussianDifference(std::vector<double> proposed_value, std::vector<double> central_value,
                                  double **inv_cov_matrix, bool check_prior=true);
  /// @brief Calc penalty term based on inverted covariance matrix
  double CalcLikelihood();
  /// @brief Return CalcLikelihood if some params were thrown out of boundary return _LARGE_LOGL_
  virtual double GetLikelihood();

  // Getters
  TMatrixDSym *getCovMatrix() { return covMatrix; }
  TMatrixDSym *getInvCovMatrix() { return invCovMatrix; }
  /// @brief Get if param has flat prior or not
  inline bool getEvalLikelihood(const int i) { return _fFlatPrior[i]; }

  /// @brief Get name of covariance
  const char *getName() { return matrixName; }
  /// @brief Get name of covariance
  std::string GetParName(const int i) {return _fNames[i];}
  /// @brief Get name of the Parameter
  const char* GetParName(const int i) const { return _fNames[i].c_str(); }
  /// @brief Get fancy name of the Parameter
  std::string GetParFancyName(const int i) {return _fFancyNames[i];}
  /// @brief Get fancy name of the Parameter
  const char* GetParFancyName(const int i) const { return _fFancyNames[i].c_str(); }
  /// @brief Get name of input file
  std::string const getInputFile() const { return inputFile; }

  /// @brief Get diagonal error for ith parameter
  inline double getDiagonalError(const int i) { return std::sqrt((*covMatrix)(i,i)); }

  // Adaptive Step Tuning Stuff
  void resetIndivStepScale();

  void initialiseAdaption(manager* fitMan);
  void saveAdaptiveToFile(TString outFileName, TString systematicName);
  bool getDoAdaption(){return use_adaptive;}

  void setThrowMatrix(TMatrixDSym *cov);
  void updateThrowMatrix(TMatrixDSym *cov);
  inline void setNumberOfSteps(const int nsteps) {
    total_steps = nsteps;
    if(total_steps >= start_adaptive_throw) resetIndivStepScale();
  }

  inline TMatrixDSym *getThrowMatrix(){return throwMatrix;}
  inline TMatrixD *getThrowMatrix_CholDecomp(){return throwMatrix_CholDecomp;}
  inline double** getThrowMatrixInv(){return InvertThrowMatrix;}
  inline std::vector<double> getParameterMeans(){return par_means;}
  /// @brief KS: Convert covariance matrix to correlation matrix and return TH2D which can be used for fancy plotting
  TH2D* GetCorrelationMatrix();

  /// @brief What parameter Gets reweighted by what amount according to MCMC
  inline double calcReWeight(const int bin) {
    if (bin >= 0 && bin < _fNumPar) {
      return _fPropVal[bin];
    } else {
      MACH3LOG_WARN("Specified bin is <= 0 OR bin > npar!");
      MACH3LOG_WARN("bin = {}, npar = {}", bin, _fNumPar);
      MACH3LOG_WARN("This won't ruin much that this step in the MCMC, but does indicate something wrong in memory!");
      return 1.0;
    }
    return 1.0;
  }
  //========
  //ETA - This might be a bit squiffy? If the vector gots moved from say a
  //push_back then the pointer is no longer valid... maybe need a better 
  //way to deal with this? It was fine before when the return was to an 
  //element of a new array. There must be a clever C++ way to be careful
  //========
  /// @brief DB Pointer return to param position
  inline const double* retPointer(const int iParam) {return &(_fPropVal.data()[iParam]);}

  //Some Getters
  inline int    GetNumParams()               {return _fNumPar;}
  virtual std::vector<double> getNominalArray();
  const std::vector<double>& getPreFitValues(){return _fPreFitValue;}
  const std::vector<double>& getGeneratedValues(){return _fGenerated;}
  const std::vector<double> getProposed() const;
  inline double getParProp(const int i) { return _fPropVal[i]; }
  inline std::vector<double> getParPropVec() {return _fPropVal;}
  inline double getParCurr(const int i) { return _fCurrVal[i]; }
  inline std::vector<double> getParCurrVec() {return _fCurrVal;}
  inline double getParInit(const int i) { return _fPreFitValue[i]; }

  virtual double getNominal(const int i) { return getParInit(i); }
  inline double GetGenerated(const int i) { return _fGenerated[i];}
  inline double GetUpperBound(const int i){ return _fUpBound[i];}
  inline double GetLowerBound(const int i){ return _fLowBound[i]; }
  inline double GetIndivStepScale(int ParameterIndex){return _fIndivStepScale.at(ParameterIndex); }
  inline double getParProp_PCA(const int i) {
    if (!pca) { MACH3LOG_ERROR("Am not running in PCA mode"); throw; }
    return fParProp_PCA(i);
  }
  
  inline double getParCurr_PCA(const int i) {
    if (!pca) { MACH3LOG_ERROR("Am not running in PCA mode"); throw; }
    return fParCurr_PCA(i);
  }

  inline bool isParameterFixedPCA(const int i) {
    if (fParSigma_PCA[i] < 0) { return true;  }
    else                      { return false; }
  }

  inline const TMatrixD getTransferMatrix() {
    if (!pca) { MACH3LOG_ERROR("Am not running in PCA mode"); throw; }
    return TransferMat;
  }

  inline const TMatrixD getEigenVectors() {
    if (!pca) { MACH3LOG_ERROR("Am not running in PCA mode"); throw; }
    return eigen_vectors;
  }

  inline const TVectorD getEigenValues() {
    if (!pca) { MACH3LOG_ERROR("Am not running in PCA mode"); throw; }
    return eigen_values;
  }

  inline const std::vector<double> getEigenValuesMaster() {
    if (!pca) { MACH3LOG_ERROR("Am not running in PCA mode"); throw; }
    return eigen_values_master;
  }

  inline void setParProp_PCA(const int i, const double value) {
    if (!pca) { MACH3LOG_ERROR("Am not running in PCA mode"); throw; }
    fParProp_PCA(i) = value;
    // And then transfer back to the parameter basis
    TransferToParam();
  }

  inline void setParCurr_PCA(const int i, const double value) {
    if (!pca) { MACH3LOG_ERROR("Am not running in PCA mode"); throw; }
    fParCurr_PCA(i) = value;
    // And then transfer back to the parameter basis
    TransferToParam();
  }

  inline void setParameters_PCA(std::vector<double> pars) {
    if (!pca) { MACH3LOG_ERROR("Am not running in PCA mode"); throw; }
    if (pars.size() != size_t(_fNumParPCA)) {
      MACH3LOG_ERROR("Warning: parameter arrays of incompatible size! Not changing parameters! {} has size {} but was expecting {}", matrixName, pars.size(), _fNumPar);
      throw;
    }
    unsigned int parsSize = pars.size();
    for (unsigned int i = 0; i < parsSize; i++) {
      fParProp_PCA(i) = pars[i];
    }
    //KS: Transfer to normal base
    TransferToParam();
  }

  inline int getSize() { return _fNumPar; }
  /// @brief Get number of params which will be different depending if using Eigen decomposition or not
  inline int getNpars() {
    if (pca) return _fNumParPCA;
    else return _fNumPar;
  }

  // Printers
  void printNominal();
  void printNominalCurrProp();
  void printPars();
  /// @brief Print step scale for each parameter
  void printIndivStepScale();

  /// @brief Generate a new proposed state
  virtual void proposeStep();
  /// @brief Accepted this step
  void acceptStep();

  /// @brief fix parameters at nominal values
  void toggleFixAllParameters();
  /// @brief fix parameter at nominal values
  void toggleFixParameter(const int i);
  /// @brief Is parameter fixed or not
  bool isParameterFixed(const int i) {
    if (_fError[i] < 0) { return true; }
    else                { return false; }
  }
  /// @brief CW: Calculate eigen values, prepare transition matrices and remove param based on defined threshold
  void ConstructPCA();
  #ifdef DEBUG_PCA
  /// @brief KS: Let's dump all useful matrices to properly validate PCA
  void DebugPCA(const double sum, TMatrixD temp, TMatrixDSym submat);
  #endif

  /// @brief is PCA, can use to query e.g. LLH scans
  inline bool IsPCA() { return pca; }

  /// @brief KS: Custom function to perform multiplication of matrix and vector with multithreading
  inline void MatrixVectorMulti(double* _restrict_ VecMulti, double** _restrict_ matrix, const double* _restrict_ vector, const int n);
  /// @brief KS: Custom function to perform multiplication of matrix and single element which is thread safe
  inline double MatrixVectorMultiSingle(double** _restrict_ matrix, const double* _restrict_ vector, const int Length, const int i);

  protected:
  void init(const char *name, const char *file);
  /// @brief Initialisation of the class using config
  void init(std::vector<std::string> YAMLFile);
  void init(TMatrixDSym* covMat);
  /// @brief Initialise vectors with parameters information
  void ReserveMemory(const int size);


  /// @brief "Randomize" the parameters in the covariance class for the proposed step. Used the proposal kernel and the current parameter value to set proposed step
  void randomize();

  // HW HACK: Need to make sure DRAM works
  // TODO: Make PCA and regular steps use the same container
  // (...and stop using pointer arrays......)
  void CorrelateSteps(){
    //if(!pca){
      CorrelateSteps(_fCurrVal);
///    }
//    else{
//      CorrelateSteps(fParCurr_PCA);
//    }
  }

  void CorrelateSteps(std::vector<double> current_step);

  /// @brief Make matrix positive definite by adding small values to diagonal, necessary for inverting matrix
  void MakePosDef(TMatrixDSym *cov = NULL);

  /// @brief HW: Finds closest possible positive definite matrix in Frobenius Norm ||.||_frob Where ||X||_frob=sqrt[sum_ij(x_ij^2)] (basically just turns an n,n matrix into vector in n^2 space then does Euclidean norm)
  void makeClosestPosDef(TMatrixDSym *cov);
  /// @brief Transfer param values from normal base to PCA base
  void TransferToPCA();
  /// @brief Transfer param values from PCA base to normal base
  void TransferToParam();

  /// @brief Handy function to return 1 for any systs
  const double* ReturnUnity(){return &Unity;}

  /// The input root file we read in
  const std::string inputFile;

  int size;
  /// Name of cov matrix
  const char *matrixName;
  /// The covariance matrix
  TMatrixDSym *covMatrix;
  /// The inverse covariance matrix
  TMatrixDSym *invCovMatrix;
  /// KS: Same as above but much faster as TMatrixDSym cache miss
  double **InvertCovMatrix;
    
  /// KS: set Random numbers for each thread so each thread has different seed
  TRandom3 **random_number;

  /// Random number taken from gaussian around prior error used for corr_throw
  double* randParams;
  /// Result of multiplication of Cholesky matrix and randParams
  double* corr_throw;
  /// Global step scale applied ot all params in this class
  double _fGlobalStepScale;

  /// KS: This is used when printing parameters, sometimes we have super long parameters name, we want to flexibly adjust couts
  unsigned int PrintLength;

  /// ETA _fNames is set automatically in the covariance class to be something like xsec_i, this is currently to make things compatible with the Diagnostic tools
  std::vector<std::string> _fNames;
  /// Fancy name for example rather than xsec_0 it is MAQE, useful for human reading
  std::vector<std::string> _fFancyNames;
  /// Stores config describing systematics
  YAML::Node _fYAMLDoc;
  /// Number of systematic parameters
  int _fNumPar;
  /// Parameter value dictated by the prior model. Based on it penalty term is calculated
  std::vector<double> _fPreFitValue;
  /// Current value of the parameter
  std::vector<double> _fCurrVal;
  /// Proposed value of the parameter
  std::vector<double> _fPropVal;
  /// Generated value of the parameter
  std::vector<double> _fGenerated;
  /// Prior error on the parameter
  std::vector<double> _fError;
  /// Lowest physical bound, parameter will not be able to go beyond it
  std::vector<double> _fLowBound;
  /// Upper physical bound, parameter will not be able to go beyond it
  std::vector<double> _fUpBound;
  /// Individual step scale used by MCMC algorithm
  std::vector<double> _fIndivStepScale;
  /// Whether to apply flat prior or not
  std::vector<bool> _fFlatPrior;

  /// Unity for null systs to point back to
  const double Unity = 1.0;

  /// perform PCA or not
  bool pca;
  /// CW: Threshold based on which we remove parameters in eigen base
  double eigen_threshold;
  /// Number of parameters in PCA base
  int _fNumParPCA;
  /// Index of the first param that is being decomposed
  int FirstPCAdpar;
  /// Index of the last param that is being decomposed
  int LastPCAdpar;
  /// Total number that remained after applying PCA Threshold
  int nKeptPCApars;
  /// Eigen value only of particles which are being decomposed
  TVectorD eigen_values;
  /// Eigen vectors only of params which are being decomposed
  TMatrixD eigen_vectors;
  /// Eigen values which have dimension equal to _fNumParPCA, and can be used in CorrelateSteps
  std::vector<double> eigen_values_master;
  /// Prefit value for PCA params
  std::vector<double> _fPreFitValue_PCA;
  /// Matrix used to converting from PCA base to normal base
  TMatrixD TransferMat;
  /// Matrix used to converting from normal base to PCA base
  TMatrixD TransferMatT;
  /// CW: Current parameter value in PCA base
  TVectorD fParProp_PCA;
  /// CW: Proposed parameter value in PCA base
  TVectorD fParCurr_PCA;
  /// Tells if parameter is fixed in PCA base or not
  std::vector<double> fParSigma_PCA;
  /// If param is decomposed this will return -1, if not this will return enumerator to param in normal base. This way we can map stuff like step scale etc between normal base and undecomposed param in eigen base.
  std::vector<int> isDecomposed_PCA;

  // Adaptive MCMC
  TMatrixDSym* throwMatrix;
  TMatrixD* throwMatrix_CholDecomp;
  /// Throw matrix that is being used in the fit, much faster as TMatrixDSym cache miss
  double **throwMatrixCholDecomp;
  double **InvertThrowMatrix;

  // Adaptive Stuff

  void setAdaptionDefaults();
  void setAdaptiveBlocks(std::vector<std::vector<int>> block_indices);
  void setThrowMatrixFromFile(std::string matrix_file_name, std::string matrix_name, std::string means_name);
  void updateAdaptiveCovariance();
  void createNewAdaptiveCovariance();

  bool use_adaptive;
  int total_steps;
  int start_adaptive_throw; // When do we start throwing?

  int start_adaptive_update; //Thresholds for when to turn on/off updating adaptive MCMC
  int end_adaptive_update;
  int adaptive_update_step;

  std::vector<int> adapt_block_matrix_indices;
  std::vector<int> adapt_block_sizes;


  std::vector<double> par_means;
  TMatrixDSym* adaptiveCovariance;
};
