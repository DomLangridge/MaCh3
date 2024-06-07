#include "mcmc.h"

// *************************
// Initialise the manager and make it an object of mcmc class
// Now we can dump manager settings to the output file
mcmc::mcmc(manager *man) : FitterBase(man) {
// *************************

  // Beginning step number
  stepStart = 0;

  // Starting parameters should be thrown
  reject = false;
  chainLength = fitMan->raw()["General"]["MCMC"]["NSteps"].as<double>();

  AnnealTemp = GetFromManager<double>(fitMan->raw()["General"]["MCMC"]["AnnealTemp"], -999);
  if(AnnealTemp < 0) anneal = false;
  else
  {
    MACH3LOG_INFO("Enabling simulated annealing with T = {}", AnnealTemp);
    anneal = true;
  }
}

// *************************
// Destructor: close the logger and output file
mcmc::~mcmc() {
// *************************

}

// *************************
// Load starting positions from the end of a previous chain
void mcmc::ReadParsFromFile(std::string file) {
// *************************
  MACH3LOG_INFO("MCMC getting starting position from {}", file);

  TFile *infile = new TFile(file.c_str(), "READ");
  TTree *posts = (TTree*)infile->Get("posteriors");
  TObjArray* brlis = (TObjArray*)posts->GetListOfBranches();
  int nbr = brlis->GetEntries();
  TString* branch_names = new TString[nbr];
  double* branch_vals = new double[nbr];

  for (int i = 0; i < nbr; ++i) {
    TBranch *br = (TBranch*)brlis->At(i);
    TString bname = br->GetName();
    branch_names[i] = bname;
    std::cout << " * Loading " << bname << std::endl;
    posts->SetBranchAddress(branch_names[i], &branch_vals[i]);
  }

  posts->GetEntry(posts->GetEntries()-1);

  delete[] branch_names;
  delete[] branch_vals;
  infile->Close();
  delete infile;
}

// **********************
// Do we accept the proposed step for all the parameters?
void mcmc::CheckStep() {
// **********************

  accept = false;

  // Set the acceptance probability to zero
  accProb = 0.0;

  // Calculate acceptance probability
  if (anneal) accProb = TMath::Min(1.,TMath::Exp( -(logLProp-logLCurr) / (TMath::Exp(-step/AnnealTemp)))); 
  else accProb = TMath::Min(1., TMath::Exp(logLCurr-logLProp));

  // Get the random number
  double fRandom = random->Rndm();

  // Do the accept/reject
  if (fRandom <= accProb) {
    accept = true;
    ++accCount;
  } else {
    accept = false;
  }

  #ifdef DEBUG
  if (debug) debugFile << " logLProp: " << logLProp << " logLCurr: " << logLCurr << " accProb: " << accProb << " fRandom: " << fRandom << std::endl;
  #endif
}


void mcmc::acceptStep(){
  // Update all the handlers to accept the step
  if (accept && !reject) {
    logLCurr = logLProp;

    if (osc) {
      osc->acceptStep();
    }

    // Loop over systematics and accept
    for (size_t s = 0; s < systematics.size(); ++s) {
      systematics[s]->acceptStep();
    }
  }

  stepClock->Stop();
  stepTime = stepClock->RealTime();

  // Write step to output tree
  outTree->Fill();

}

void mcmc::initialiseChain(){
  // Some common methods
  // Save the settings into the output file
  SaveSettings();

  // Prepare the output branches
  PrepareOutput();

  // Reconfigure the samples, systematics and oscillation for first weight
  // ProposeStep sets logLProp
  ProposeStep();
  // Set the current logL to the proposed logL for the 0th step
  // Accept the first step to set logLCurr: this shouldn't affect the MCMC because we ignore the first N steps in burn-in
  logLCurr = logLProp;


}


// *******************
// Run the Markov chain with all the systematic objects added
void mcmc::runMCMC() {
  // *******************
  initialiseChain();
  // Begin MCMC
  for (step = stepStart; step < stepStart+chainLength; ++step)
  {
    stepClock->Start();
    // Set the initial rejection to false
    reject = false;

    // Print 10 steps in total
    if ( (step-stepStart) % (chainLength/10) == 0) {
      PrintProgress();
    }

    // Propose current step variation and save the systematic likelihood that results in this step being taken
    // Updates logLProp
    ProposeStep();

    // Does the MCMC accept this step?
    CheckStep();
    // Now Separated out
    acceptStep();
    // Auto save the output
    if (step % auto_save == 0) outTree->AutoSave();
  }

  // Save all the MCMC output
  SaveOutput();

  // Process MCMC
  ProcessMCMC();
}

// *******************
// Process the MCMC output to get postfit etc
void mcmc::ProcessMCMC() {
// *******************

  if (fitMan == NULL) return;

  // Process the MCMC
  if (fitMan->raw()["General"]["ProcessMCMC"].as<bool>()) {

    // Make the processor
    MCMCProcessor Processor(std::string(outputFile->GetName()), false);
    
    Processor.Initialise();
    // Make the TVectorD pointers which hold the processed output
    TVectorD *Central = NULL;
    TVectorD *Errors = NULL;
    TVectorD *Central_Gauss = NULL;
    TVectorD *Errors_Gauss = NULL;
    TVectorD *Peaks = NULL;
    
    // Make the postfit
    Processor.GetPostfit(Central, Errors, Central_Gauss, Errors_Gauss, Peaks);
    Processor.DrawPostfit();

    // Make the TMatrix pointers which hold the processed output
    TMatrixDSym *Covariance = NULL;
    TMatrixDSym *Correlation = NULL;

    // Make the covariance matrix
    Processor.GetCovariance(Covariance, Correlation);
    Processor.DrawCovariance();

    std::vector<TString> BranchNames = Processor.GetBranchNames();

    // Re-open the TFile
    if (!outputFile->IsOpen()) {
      MACH3LOG_INFO("Opening output again to update with means..");
      outputFile = new TFile(fitMan->raw()["General"]["Output"]["Filename"].as<std::string>().c_str(), "UPDATE");
    }
    
    Central->Write("PDF_Means");
    Errors->Write("PDF_Errors");
    Central_Gauss->Write("Gauss_Means");
    Errors_Gauss->Write("Errors_Gauss");
    Covariance->Write("Covariance");
    Correlation->Write("Correlation");
  }
}

// *******************
// Do the initial reconfigure of the MCMC
void mcmc::ProposeStep() {
// *******************

  // Initial likelihood
  double llh = 0.0;

  // Initiate to false
  reject = false;

  // Propose steps for the oscillation handlers
  if (osc) {
    osc->proposeStep();

    // Now get the likelihoods for the oscillation
    osc_llh = osc->GetLikelihood();
    
    // Add the oscillation likelihoods to the reconfigure likelihoods
    llh += osc_llh;

    #ifdef DEBUG
    if (debug) debugFile << "LLH for oscillation handler: " << llh << std::endl;
    #endif
  }

  int stdIt = 0;
  // Loop over the systematics and propose the initial step
  for (std::vector<covarianceBase*>::iterator it = systematics.begin(); it != systematics.end(); ++it, ++stdIt) {

    // Could throw the initial value here to do MCMC stability studies
    // Propose the steps for the systematics
    (*it)->proposeStep();

    // Get the likelihood from the systematics
    syst_llh[stdIt] = (*it)->GetLikelihood();
    llh += syst_llh[stdIt];

    #ifdef DEBUG
    if (debug) debugFile << "LLH after " << systematics[stdIt]->getName() << " " << llh << std::endl;
    #endif
  }

  // Check if we've hit a boundary in the systematics
  // In this case we can save time by not having to reconfigure the simulation
  if (llh >= __LARGE_LOGL__) {
    reject = true;
    #ifdef DEBUG
    if (debug) debugFile << "Rejecting based on boundary" << std::endl;
    #endif
  }

  // Only reweight when we have a good parameter configuration
  // This speeds things up considerably because for every bad parameter configuration we don't have to reweight the MC
  if (!reject)
  {
    // Could multi-thread this
    // But since sample reweight is multi-threaded it's probably better to do that
    for (size_t i = 0; i < samples.size(); ++i)
    {
      // If we're running with different oscillation parameters for neutrino and anti-neutrino
      if (osc ){ 
        samples[i]->reweight(osc->getPropPars());
        // If we aren't using any oscillation
      } else {
        double* fake = NULL;
        samples[i]->reweight(fake);
      }
    }

    //DB for atmospheric event by event sample migration, need to fully reweight all samples to allow event passing prior to likelihood evaluation
    for (size_t i = 0; i < samples.size(); ++i) {
      // Get the sample likelihoods and add them
      sample_llh[i] = samples[i]->GetLikelihood();
      llh += sample_llh[i];
      #ifdef DEBUG
      if (debug) debugFile << "LLH after sample " << i << " " << llh << std::endl;
      #endif
    }

  // For when we don't have to reweight, set sample to madness
  } else {
    for (size_t i = 0; i < samples.size(); ++i) {
      // Set the sample_llh[i] to be madly high also to signify a step out of bounds
      sample_llh[i] = __LARGE_LOGL__;
      #ifdef DEBUG
      if (debug) debugFile << "LLH after REJECT sample " << i << " " << llh << std::endl;
      #endif
    }
  }

  // Save the proposed likelihood (class member)
  logLProp = llh;
}

// *******************
// Print the fit output progress
void mcmc::PrintProgress() {
// *******************

  MACH3LOG_INFO("Step:\t{}/{}, current: {:.2f}, proposed: {:.2f}", step - stepStart, chainLength, logLCurr, logLProp);
  MACH3LOG_INFO("Accepted/Total steps: {}/{} = {:.2f}", accCount, step - stepStart, static_cast<double>(accCount) / static_cast<double>(step - stepStart));

  for (std::vector<covarianceBase*>::iterator it = systematics.begin(); it != systematics.end(); ++it) {
    if (std::string((*it)->getName()) == "xsec_cov") {
      MACH3LOG_INFO("Cross-section parameters: ");
      (*it)->printNominalCurrProp();
    }
  }
  #ifdef DEBUF
  if (debug) {
    debugFile << "\n-------------------------------------------------------" << std::endl;
    debugFile << "Step:\t" << step + 1 << "/" << chainLength << "  |  current: " << logLCurr << " proposed: " << logLProp << std::endl;
  }
  #endif
}
