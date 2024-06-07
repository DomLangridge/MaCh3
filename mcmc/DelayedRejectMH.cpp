#include "DelayedRejectMH.h"

/**
 * TODO :
 *   -> Work out how to make N iterations arbitrary
 *   -> Cache alpha and q(whatever)
 *   -> Interface nicely with covarianceBase
 *   -> Sort out the spaghetti inheritance from mcmc
 */


// *********************
DelayedRejectionMCMC::DelayedRejectionMCMC(manger man) : mcmc(man){
// *********************
    // For now this is empty!
    // TODO: Add methods for setting scale etc.

    MACH3LOG_INFO("Using Delayed Rejection Metropolis Hastings!")


}


// *********************
DelayedRejectionMCMC::~DelayedRejectionMCMC{
// *********************
// TODO: Nothing necessary rn
}

void DelayedRejectionMCMC::setSystStepScale(double scale){
// Lets us rescale our proposal matrices
  if(osc) osc->setStepScale(scale);
  for(auto syst : systematics) { syst->setStepScale(scale);}
}

// *********************
void runMCMC(){
// HW: TODO: Rewrite MCMC so they can use inherited structures for this!
// ********************
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
    CheckStep();

    // HW : TODO Generalise this..
    if(!accept){
      setSystStepScale(0.01); // Shrink
      ProposeDelayedStep();
      CheckDelayedStep();
      setSystStepScale(1.0); // Grow
    }

    acceptStep();

    // Auto save the output
    if (step % auto_save == 0) outTree->AutoSave();
  }

  // Save all the MCMC output
  SaveOutput();

  // Process MCMC
  ProcessMCMC();
}

void DelayedRejectionMCMC::ProposeDelayedStep(){
  // HW: Now for the tricky bit. REALLY we should
  // let this work for N iterations, as proof of concept we'll try 1

  // For a single stage process the formula is
  //     a = [q(y2, y1)(1-a(y1,y2))]/[q(x,y1)[(1-a(y1, x))]]
  // Where y1 and x are the step we just rejected and the current step(resp.)
  // and y2 is the step we're about to propose. q is the proposal function and p the prior

  // So this will be hacky BUT should work. Firstly let's get the stuff we already have

  // Let's do a proposal function about the rejected point
  // This is also probably slow
  if(osc) {
    current_step[systematics.size()] = osc->getCurrVec();
    //rejected_step[systematics.size()] = osc->getPropVec();
    osc->setParCurrVec(rejected_step[systematics.size()]);
  }

  for(unsigned int i=0; i<systematics.size(); i++){
    current_step[i] = systematics[i].size()->getCurrVec();
  }

  // Now we propose a new step based on our previously rejected one
  logLReject = logLProp;
  ProposeStep();
}

void DelayedRejectionMCMC::CheckDelayedStep(){
  // Note we explicitly ignore annealing
  // Also since we're nicely gaussian etc. we don't need to reverse scaling!

  // Let's get the difference from proposal functions first

  // This is the term generated by our previous rejected step + our new step


  double q_y1_y2 = 0.;
  double q_y1_x  = 0.;

  for(unsigned int i=0; i<systematics.size; i++){
    auto syst = systematics[i];
     q_y1_y2 += calcGaussianDifference(syst->getParCurrVec(), syst->getParPropVec(),
                                            syst->getThrowMatrixInv(), false);
    // Next we need to proposal probability for our rejected +accepted step
     q_y1_x += calcGaussianDifference(syst->getParCurrVec(), current_step[i],
                                           syst->getThrowMatrixInv(), false);
  }

  if(osc){
     q_y1_y2 += calcGaussianDifference(osc->getParCurrVec(), osc->getParPropVec(),
                                            osc->getThrowMatrixInv(), false);
    // Next we need to proposal probability for our rejected +accepted step
     q_y1_x += calcGaussianDifference(osc->getParCurrVec(), current_step[systematics.size()],
                                           osc->getThrowMatrixInv(), false);

  }

  // Now all we need is L(y2) and we have all the information we need
  double denom_alpha_x_y1
  denom_alpha_x_y1 = 1-accProb;

  // NOW we can calculate "alpha" for our new step
  double accProb_reject = TMath::Min(1., TMath::Exp(logLProp-logLReject));

  double accProb_full = TMath::Min(1, TMath::Exp(q_y1_y2-q_y1_x)*((1-accProb_reject)/denom_alpha_x_y1));


  double fRandom = random->Rndm();

  if (fRandom <= accProb) {
    accept = true;
    ++accCount;
  } else {
    // We need to re-update our systematics

    if(osc){
      osc->setParCurrVec(current_step[systematics.size()]);
    }
    for(unsigned int i=0; i<systematics.size(); i++){
      systematics[i]->setParCurrVec(current_step[i]);
    }


    accept = false;
  }

}
