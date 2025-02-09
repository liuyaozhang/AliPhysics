/*
Author: Vytautas Vislavicius
Extention of Generic Flow (https://arxiv.org/abs/1312.3572 by A. Bilandzic et al.)
Class steers the initialization and calculation of n-particle correlations. Uses recursive function, all terms are calculated only once.
Latest version includes the calculation of any number of gaps and any combination of harmonics (including eg symmetric cumulants, etc.)
If used, modified, or distributed, please aknowledge the author of this code.
*/

#include "AliGFW.h"
AliGFW::AliGFW():
  fInitialized(kFALSE)
{
};

AliGFW::~AliGFW() {
  for(auto pItr = fCumulants.begin(); pItr != fCumulants.end(); ++pItr)
    pItr->DestroyComplexVectorArray();
};

void AliGFW::AddRegion(TString refName, Int_t lNhar, Int_t lNpar, Double_t lEtaMin, Double_t lEtaMax, Int_t lNpT, Int_t BitMask) {
  if(lNpT < 1) {
    printf("Number of pT bins cannot be less than 1! Not adding anything.\n");
    return;
  };
  if(lEtaMin >= lEtaMax) {
    printf("Eta min. cannot be more than eta max! Not adding...\n");
    return;
  };
  if(refName.EqualTo("")) {
    printf("Region must have a name!\n");
    return;
  };
  Region lOneRegion;
  lOneRegion.Nhar = lNhar; //Number of harmonics
  lOneRegion.Npar = lNpar; //Number of powers
  lOneRegion.NparVec = vector<Int_t> {}; //if powers defined, then set this to empty vector
  lOneRegion.EtaMin = lEtaMin; //Min. eta
  lOneRegion.EtaMax = lEtaMax; //Max. eta
  lOneRegion.NpT = lNpT; //Number of pT bins
  lOneRegion.rName = refName; //Name of the region
  lOneRegion.BitMask = BitMask; //Bit mask
  AddRegion(lOneRegion);
};
void AliGFW::AddRegion(TString refName, Int_t lNhar, Int_t *lNparVec, Double_t lEtaMin, Double_t lEtaMax, Int_t lNpT, Int_t BitMask) {
  if(lNpT < 1) {
    printf("Number of pT bins cannot be less than 1! Not adding anything.\n");
    return;
  };
  if(lEtaMin >= lEtaMax) {
    printf("Eta min. cannot be more than eta max! Not adding...\n");
    return;
  };
  if(refName.EqualTo("")) {
    printf("Region must have a name!\n");
    return;
  };
  Region lOneRegion;
  lOneRegion.Nhar = lNhar; //Number of harmonics
  lOneRegion.Npar = 0; //If vector with powers defined, set this to zero
  lOneRegion.NparVec = vector<Int_t> {};//lNparVec; //vector with powers for each harmonic
  for(Int_t i=0;i<lNhar;i++) lOneRegion.NparVec.push_back(lNparVec[i]);
  lOneRegion.EtaMin = lEtaMin; //Min. eta
  lOneRegion.EtaMax = lEtaMax; //Max. eta
  lOneRegion.NpT = lNpT; //Number of pT bins
  lOneRegion.rName = refName; //Name of the region
  lOneRegion.BitMask = BitMask; //Bit mask
  AddRegion(lOneRegion);
};
void AliGFW::SplitRegions() {
  //Simple case. Will not look for overlaps, etc. Everything is left for end-used
};

Int_t AliGFW::CreateRegions() {
  if(fRegions.size()<1) {
    printf("No regions set. Skipping...\n");
    return 0;
  };
  SplitRegions();
  //for(auto pitr = fRegions.begin(); pitr!=fRegions.end(); pitr++) pitr->PrintStructure();
  Int_t nRegions=0;
  for(auto pItr=fRegions.begin(); pItr!=fRegions.end(); pItr++) {
    AliGFWCumulant *lCumulant = new AliGFWCumulant();
    if(pItr->NparVec.size()) {
      lCumulant->CreateComplexVectorArrayVarPower(pItr->Nhar, pItr->NparVec, pItr->NpT);
    } else {
      lCumulant->CreateComplexVectorArray(pItr->Nhar, pItr->Npar, pItr->NpT);
    };
    fCumulants.push_back(*lCumulant);
    ++nRegions;
  };
  if(nRegions) fInitialized=kTRUE;
  return nRegions;
};
void AliGFW::Fill(Double_t eta, Int_t ptin, Double_t phi, Double_t weight, Int_t mask, Double_t SecondWeight) {
  if(!fInitialized) CreateRegions();
  if(!fInitialized) return;
  for(Int_t i=0;i<(Int_t)fRegions.size();++i) {
    if(fRegions.at(i).EtaMin<eta && fRegions.at(i).EtaMax>eta && (fRegions.at(i).BitMask&mask))
      fCumulants.at(i).FillArray(eta,ptin,phi,weight,SecondWeight);
  };
};
TComplex AliGFW::TwoRec(Int_t n1, Int_t n2, Int_t p1, Int_t p2, Int_t ptbin, AliGFWCumulant *r1, AliGFWCumulant *r2, AliGFWCumulant *r3) {
  TComplex part1 = r1->Vec(n1,p1,ptbin);
  TComplex part2 = r2->Vec(n2,p2,ptbin);
  TComplex part3 = r3?r3->Vec(n1+n2,p1+p2,ptbin):TComplex(0,0);
  TComplex formula = part1*part2-part3;
  return formula;
};
TComplex AliGFW::RecursiveCorr(AliGFWCumulant *qpoi, AliGFWCumulant *qref, AliGFWCumulant *qol, Int_t ptbin, vector<Int_t> &hars) {
  vector<Int_t> pows;
  for(Int_t i=0; i<(Int_t)hars.size(); i++)
    pows.push_back(1);
  return RecursiveCorr(qpoi, qref, qol, ptbin, hars, pows);
};

TComplex AliGFW::RecursiveCorr(AliGFWCumulant *qpoi, AliGFWCumulant *qref, AliGFWCumulant *qol, Int_t ptbin, vector<Int_t> &hars, vector<Int_t> &pows) {
  if((pows.at(0)!=1) && qol) qpoi=qol; //if the power of POI is not unity, then always use overlap (if defined).
  //Only valid for 1 particle of interest though!
  if(hars.size()<2) return qpoi->Vec(hars.at(0),pows.at(0),ptbin);
  if(hars.size()<3) return TwoRec(hars.at(0), hars.at(1),pows.at(0),pows.at(1), ptbin, qpoi, qref, qol);
  Int_t harlast=hars.at(hars.size()-1);
  Int_t powlast=pows.at(pows.size()-1);
  hars.erase(hars.end()-1);
  pows.erase(pows.end()-1);
  TComplex formula = RecursiveCorr(qpoi, qref, qol, ptbin, hars, pows)*qref->Vec(harlast,powlast);
  Int_t lDegeneracy=1;
  Int_t harSize = (Int_t)hars.size();
  for(Int_t i=harSize-1;i>=0;i--) {
  //checking if current configuration is a permutation of the next one.
  //Need to have more than 2 harmonics though, otherwise it doesn't make sense.
    if(i>2) { //only makes sense when we have more than two harmonics remaining
      if(hars.at(i) == hars.at(i-1) && pows.at(i) == pows.at(i-1)) {//if it is a permutation, then increase degeneracy and continue;
        lDegeneracy++;
        continue;
      };
    }
    hars.at(i)+=harlast;
    pows.at(i)+=powlast;
    //The issue is here. In principle, if i=0 (dif), then the overlap is only qpoi (0, if no overlap);
    //Otherwise, if we are not working with the 1st entry (dif.), then overlap will always be from qref
    //One should thus (probably) make a check if i=0, then qovl=qpoi, otherwise qovl=qref. But need to think more
    //-- This is not aplicable anymore, since the overlap is explicitly specified
    TComplex subtractVal = RecursiveCorr(qpoi, qref, qol, ptbin, hars, pows);
    if(lDegeneracy>1) { subtractVal *= lDegeneracy; lDegeneracy=1; };
    formula-=subtractVal;
    hars.at(i)-=harlast;
    pows.at(i)-=powlast;

  };
  hars.push_back(harlast);
  pows.push_back(powlast);
  return formula;
};
void AliGFW::Clear() {
  for(auto ptr = fCumulants.begin(); ptr!=fCumulants.end(); ++ptr) ptr->ResetQs();
  fCalculatedNames.clear();
  fCalculatedQs.clear();
};
TComplex AliGFW::Calculate(TString config, Bool_t SetHarmsToZero) {
  if(config.EqualTo("")) {
    printf("Configuration empty!\n");
    return TComplex(0,0);
  };
  TString tmp;
  Ssiz_t sz1=0;
  TComplex ret(1,0);
  while(config.Tokenize(tmp,sz1,"}")) {
    if(SetHarmsToZero) SetHarmonicsToZero(tmp);
    TComplex val=CalculateSingle(tmp);
    ret*=val;
    fCalculatedQs.push_back(val);
    fCalculatedNames.push_back(tmp);
  };
  return ret;
};
TComplex AliGFW::CalculateSingle(TString config) {
  //First remove all ; and ,:
  config.ReplaceAll(","," ");
  config.ReplaceAll(";"," ");
  //Then make sure we don't have any double-spaces:
  while(config.Index("  ")>-1) config.ReplaceAll("  "," ");
  vector<Int_t> regs;
  vector<Int_t> hars;
  Int_t ptbin=0;
  Ssiz_t sz1=0;
  Ssiz_t szend=0;
  TString ts, ts2;
  //find the pT-bin:
  if(config.Tokenize(ts,sz1,"(")) {
    config.Tokenize(ts,sz1,")");
    ptbin=ts.Atoi();
  };
  //Fetch region descriptor
  if(sz1<0) sz1=0;
  if(!config.Tokenize(ts,szend,"{")) {
    printf("Could not find harmonics!\n");
    return TComplex(0,0);
  };
  //Fetch regions
  while(ts.Tokenize(ts2,sz1," ")) {
    if(sz1>=szend) break;
    Int_t ind=FindRegionByName(ts2);
    if(ts2.EqualTo(" ") || ts2.EqualTo("")) continue;
    if(ind<0) {
      printf("Could not find region named %s!\n",ts2.Data());
      break;
    };
    regs.push_back(ind);
  };
  //Fetch harmonics
  while(config.Tokenize(ts,szend," ")) hars.push_back(ts.Atoi());
  if(regs.size()==1) return Calculate(regs.at(0),hars);
  return Calculate(regs.at(0),regs.at(1),hars,ptbin);
};
AliGFW::CorrConfig AliGFW::GetCorrelatorConfig(TString config, TString head, Bool_t ptdif) {
  //First remove all ; and ,:
  config.ReplaceAll(","," ");
  config.ReplaceAll(";"," ");
  config.ReplaceAll("| ","|");
  //If pT-bin is provided, then look for & remove space before "(" (so that it's clean afterwards)
  while(config.Index(" (")>-1) config.ReplaceAll(" (","(");
  //Then make sure we don't have any double-spaces:
  while(config.Index("  ")>-1) config.ReplaceAll("  "," ");
  vector<Int_t> regs;
  vector<Int_t> hars;
  Ssiz_t sz1=0;
  Ssiz_t szend=0;
  TString ts, ts2;
  CorrConfig ReturnConfig;
  //Fetch region descriptor
  if(!config.Tokenize(ts,szend,"{")) {
    printf("Could not find harmonics!\n");
    return ReturnConfig;
  };
  szend=0;
  Int_t counter=0;
  while(config.Tokenize(ts,szend,"{")) {
    counter++;
    ReturnConfig.Regs.push_back(vector<Int_t> {});
    ReturnConfig.Hars.push_back(vector<Int_t> {});
    // ReturnConfig.ptInd.push_back(vector<Int_t> {});
    ReturnConfig.Overlap.push_back(-1); //initially, assume no overlap
    //Check if there's a particular pT bin I should be using here. If so, store it (otherwise, it's bin 0)
    Int_t ptbin=-1;
    Ssiz_t sz2=0;
    if(ts.Contains("(")) {
      if(!ts.Contains(")")) {printf("Missing \")\" in the configurator. Returning...\n"); return ReturnConfig; };
      sz2 = ts.Index("(");
      sz1=sz2+1;
      ts.Tokenize(ts2,sz1,")");
      ptbin=ts2.Atoi();
      ts.Remove(sz2,(sz1-sz2+1));
      szend-=(sz1-sz2); //szend also becomes shorter
      //also need to remove this from config now:
      sz2 = config.Index("(");
      sz1 = config.Index(")");
      config.Remove(sz2,sz1-sz2+1);
    };
    ReturnConfig.ptInd.push_back(ptbin);
    sz1=0;
    //Fetch regions
    while(ts.Tokenize(ts2,sz1," ")) {
      if(sz1>=szend) break;
      Bool_t isOverlap=ts2.Contains("|");
      if(isOverlap) ts2.Remove(0,1); //If overlap, remove the delimiter |
      Int_t ind=FindRegionByName(ts2);
      if(ts2.EqualTo(" ") || ts2.EqualTo("")) continue;
      if(ind<0) {
        printf("Could not find region named %s!\n",ts2.Data());
        break;
      };
      if(!isOverlap)
        ReturnConfig.Regs.at(counter-1).push_back(ind);
      else ReturnConfig.Overlap.at((int)ReturnConfig.Overlap.size()-1) = ind;
    };
    TString harstr;
    config.Tokenize(harstr,szend,"}");
    Ssiz_t dummys=0;
    //Fetch harmonics
    while(harstr.Tokenize(ts,dummys," ")) ReturnConfig.Hars.at(counter-1).push_back(ts.Atoi());
  };
  ReturnConfig.Head = head;
  ReturnConfig.pTDif = ptdif;
  // ReturnConfig.pTbin = ptbin;
  return ReturnConfig;
};

TComplex AliGFW::Calculate(Int_t poi, Int_t ref, vector<Int_t> hars, Int_t ptbin) {
  AliGFWCumulant *qref = &fCumulants.at(ref);
  AliGFWCumulant *qpoi = &fCumulants.at(poi);
  AliGFWCumulant *qovl = qpoi;
  return RecursiveCorr(qpoi, qref, qovl, ptbin, hars);
};
// TComplex AliGFW::Calculate(CorrConfig corconf, Int_t ptbin, Bool_t SetHarmsToZero, Bool_t DisableOverlap) {
//    vector<Int_t> ptbins;
//    for(Int_t i=0;i<(Int_t)corconf.size();i++) ptbins.push_back(ptbin);
//    return Calculate(corconf,ptbins,SetHarmsToZero,DisableOverlap);
// }
TComplex AliGFW::Calculate(CorrConfig corconf, Int_t ptbin, Bool_t SetHarmsToZero, Bool_t DisableOverlap) {
  if(corconf.Regs.size()==0) return TComplex(0,0); //Check if we have any regions at all
  // if(ptbins.size()!=corconf.Regs.size()) {printf("Number of pT-bins is not the same as number of subevents!\n"); return TComplex(0,0); };
  TComplex retval(1,0);
  Int_t ptInd;
  for(Int_t i=0;i<(Int_t)corconf.Regs.size();i++) { //looping over all regions
    if(corconf.Regs.at(i).size()==0)  return TComplex(0,0); //again, if no regions in the current subevent, then quit immediatelly
    ptInd = corconf.ptInd.at(i); //for i=0 (potentially, POI)
    if(ptInd<0) ptInd = ptbin;
    // Int_t ptbin = ptbins.at(i);
    //picking up the indecies of regions...
    Int_t poi = corconf.Regs.at(i).at(0);
    Int_t ref = (corconf.Regs.at(i).size()>1)?corconf.Regs.at(i).at(1):corconf.Regs.at(i).at(0);
    Int_t ovl = corconf.Overlap.at(i);
    //and regions themselves
    AliGFWCumulant *qref = &fCumulants.at(ref);
    AliGFWCumulant *qpoi = &fCumulants.at(poi);
    if(!qref->IsPtBinFilled(ptInd)) return TComplex(0,0); //if REF is not filled, don't even continue. Could be redundant, but should save little CPU time
    if(!qpoi->IsPtBinFilled(ptInd)) return TComplex(0,0);//if POI is not filled, don't even continue. Could be redundant, but should save little CPU time
    AliGFWCumulant *qovl=0;
    //Check if in the ref. region we have enough particles (no. of particles in the region >= no of harmonics for subevent)
    Int_t sz1 = corconf.Hars.at(i).size();
    if(poi!=ref) sz1--;
    if(qref->GetN() < sz1) return TComplex(0,0);
    //Then, figure the overlap
    if(ovl > -1) //if overlap is defined, then (unless it's explicitly disabled)
      qovl = DisableOverlap?0:&fCumulants.at(ovl);
    else if(ref==poi) qovl = qref; //If ref and poi are the same, then the same is for overlap. Only, when OL not explicitly defined
    if(SetHarmsToZero) for(Int_t j=0;j<(Int_t)corconf.Hars.at(i).size();j++) corconf.Hars.at(i).at(j) = 0;
    retval *= RecursiveCorr(qpoi, qref, qovl, ptInd, corconf.Hars.at(i));
  }
  return retval;
};

TComplex AliGFW::Calculate(Int_t poi, vector<Int_t> hars) {
  AliGFWCumulant *qpoi = &fCumulants.at(poi);
  return RecursiveCorr(qpoi, qpoi, qpoi, 0, hars);
};
Int_t AliGFW::FindRegionByName(TString refName) {
  for(Int_t i=0;i<(Int_t)fRegions.size();i++) if(fRegions.at(i).rName.EqualTo(refName)) return i;
  return -1;
};
Int_t AliGFW::FindCalculated(TString identifier) {
  if(fCalculatedNames.size()==0) return -1;
  for(Int_t i=0;i<(Int_t)fCalculatedNames.size();i++) {
    if(fCalculatedNames.at(i).EqualTo(identifier)) return i;
  };
  return -1;
};
Bool_t AliGFW::SetHarmonicsToZero(TString &instr) {
  TString tmp;
  Ssiz_t sz1=0, sz2;
  if(!instr.Tokenize(tmp,sz1,"{")) {
    printf("AliGFW::SetHarmonicsToZero: could not find \"{\" token in %s\n",instr.Data());
    return kFALSE;
  };
  sz2=sz1;
  Int_t indc=0;
  while(instr.Tokenize(tmp,sz1," ")) indc++;
  if(!indc) {
    printf("AliGFW::SetHarmonicsToZero: did not find any harmonics in %s, nothing to replace\n",instr.Data());
    return kFALSE;
  };
  instr.Remove(sz2);
  for(Int_t i=0;i<indc;i++) instr.Append("0 ");
  return kTRUE;
};
