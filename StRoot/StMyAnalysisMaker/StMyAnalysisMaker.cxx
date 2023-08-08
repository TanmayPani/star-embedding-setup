#define StMyAnalysisMaker_cxx

#include "StMyAnalysisMaker.h"
//#include <unordered_set>
//ROOT includes
#include "TH1.h"
#include "TH2.h"
#include "TTree.h"
#include "TFile.h"
// centrality includes
#include "StRoot/StRefMultCorr/CentralityMaker.h"
#include "StRoot/StRefMultCorr/StRefMultCorr.h"
// STAR includes
#include "StPicoEvent/StPicoDst.h"
#include "StPicoDstMaker/StPicoDstMaker.h"
#include "StPicoEvent/StPicoEvent.h"
#include "StPicoEvent/StPicoTrack.h"
#include "StPicoEvent/StPicoMcTrack.h"
//#include "StRoot/StPicoEvent/StPicoEmcTrigger.h"
#include "StPicoEvent/StPicoBTowHit.h"
#include "StPicoEvent/StPicoBEmcPidTraits.h"
//MyAnalysisMaker includes
#include "StEmcPosition2.h"
//#include "StMyAnalysisUtils.h"
#include "StRoot/TStarEventClass/TStarEvent.h"
#include "StRoot/TStarEventClass/TStarTrack.h"
#include "StRoot/TStarEventClass/TStarGenTrack.h"
#include "StRoot/TStarEventClass/TStarTower.h"
#include "StRoot/TStarEventClass/TStarJet.h"
#include "StRoot/TStarEventClass/TStarArrays.h"
#include "StRoot/StMyJetMaker/StMyJetMaker.h"
#include "StRoot/StPythiaEventMaker/StPythiaEvent.h"
#include "StRoot/StPythiaEventMaker/StPythiaEventMaker.h"

ClassImp(StMyAnalysisMaker);

using namespace std;

StMyAnalysisMaker::StMyAnalysisMaker(string name, string output, bool dodebug):
StMaker(name.c_str()){
    doDebug = dodebug;

    if(doDebug){
        cout<<"StMyAnalysisMaker::StMyAnalysisMaker()"<<endl;
        doEventDebug = true;
        doTrackDebug = true;
        doGenDebug = true;
        doTowerDebug = true;
        doTriggerDebug = true;
        doJetDebug = true;
    }

    anaName = name; 
    outputFileName = output;
    outputFileName.insert(outputFileName.find(".root"), ".tree");
    histoFileName = output;
    histoFileName.insert(histoFileName.find(".root"), ".hist");

    if(doDebug){
        cout<<"Name of the StMaker instance : "<<anaName<<endl;
        cout<<"Name of the file that will store the output TTree : "<<outputFileName<<endl;
        cout<<"Name of the file that will store the Histograms : "<<histoFileName<<endl;
    }

    towerHadCorrSumTrE.resize(4800);
    towerHadCorrMaxTrE.resize(4800);
    towerNTracksMatched.resize(4800);

    if(doDebug)cout<<"StMyAnalysisMaker::StMyAnalysisMaker() completed"<<endl;
}

StMyAnalysisMaker::~StMyAnalysisMaker(){
    if(doDebug)cout<<"********StMyAnalysisMaker::~StMyAnalysisMaker()*********"<<endl;
    if(emcPosition) delete emcPosition;
    if(efficiencyFile) delete efficiencyFile; 
    if(tsArrays) delete tsArrays;
    for(const auto& hist1D : histos1D){
        if(doDebug)cout<<"Deleting "<<hist1D.first<<endl;
        if(hist1D.second) delete hist1D.second;
    }
    for(const auto& hist2D : histos2D){
        if(doDebug)cout<<"Deleting "<<hist2D.first<<endl;
        if(hist2D.second) delete hist2D.second;
    }
    histos1D.clear();
    histos2D.clear();
}

Int_t StMyAnalysisMaker::Init(){
    if(doDebug)cout<<"***********StMyAnalysisMaker::Init()**************"<<endl;

    int returnCode = getMakers();
    if(returnCode >= 0)return returnCode;

    if(!doppAnalysis){
        grefmultCorr = CentralityMaker::instance()->getgRefMultCorr_P18ih_VpdMB30_AllLumi();
        if(doDebug){
            cout<<"Set up grefmultCorr..."<<endl;
            grefmultCorr->print();
        }

        //grefmultCorrUtil = new StRefMultCorr("grefmult_P18ih_VpdMB30_AllLumi_MB5sc");
        //grefmultCorrUtil->setVzForWeight(16, -16.0, 16.0);
        //grefmultCorrUtil->readScaleForWeight("StRoot/StRefMultCorr/macros/weight_grefmult_vpd30_vpd5_Run14_P18ih_set1.txt");
    }

    if(!doRunbyRun)setUpBadRuns(); //Probably deprecated, might add this into StRefMultCorr

    setUpBadTowers();//There may be a better way
    setUpDeadTowers();//There may be a better way

    string efficiencyFileName;

    if(doppAnalysis)efficiencyFileName = "./StRoot/StMyAnalysisMaker/Run12_efficiency_New.root";
    else efficiencyFileName = "./StRoot/StMyAnalysisMaker/Run14_efficiencySmaller2D.root";

    efficiencyFile = new TFile(efficiencyFileName.c_str(), "READ");
    
    if(!efficiencyFile->IsOpen()){
        cout<<"Could not open efficiency file!"<<endl;
        return kStWarn;
    }

    if(doDebug)cout<<"Opened efficiency file: "<<efficiencyFileName<<endl;

    if(doDebug)cout<<"Setting up StEmcPosition2..."<<endl;
    emcPosition = new StEmcPosition2();

    TStarEvent::setRunFlag(runFlag);

    if(doDebug)cout<<"***********END OF StMyAnalysisMaker::Init()**************"<<endl;

    for(auto& hist : histos1D){
        hist.second->Sumw2();
    }

    for(auto& hist : histos2D){
        hist.second->Sumw2();
    }

    return kStOK;
}

void StMyAnalysisMaker::Clear(Option_t *option){
    if(doDebug)cout<<"***********StMyAnalysisMaker::Clear()**************"<<endl;
    //TStarArrays::clearArrays();
    //cout<<"clear 1 !"<<endl;
    if(doDebug)cout<<"***********END OF StMyAnalysisMaker::Clear()**************"<<endl;
}

void StMyAnalysisMaker::writeHistograms(){
    if(histoFileName != ""){
        histOut = new TFile(histoFileName.c_str(), "UPDATE");
        histOut->cd();
        histOut->mkdir(GetName());
        histOut->cd(GetName());
        for(const auto& hist : histos1D){
            hist.second->Write();
            if(doDebug)cout<<"Wrote "<<hist.first<<endl;
        }
        for(const auto& hist : histos2D){
            hist.second->Write();
            if(doDebug)cout<<"Wrote "<<hist.first<<endl;
        }
        histOut->cd();
        histOut->Write();
        histOut->Close();
        if(doDebug)cout<<"Wrote histograms to "<<histoFileName<<endl;
    }
}

Int_t StMyAnalysisMaker::Finish(){
    cout<< "StMyAnalysisMaker::Finish()"<<endl;

    writeHistograms(); 

    efficiencyFile->Close();

    return kStOk;
}

Int_t StMyAnalysisMaker::Make(){
    if(doEventDebug)cout<<"***********StMyAnalysisMaker::Make()**************"<<endl;

    TStarArrays::clearArrays();

    int makerReturnInt = makeEvent();
    if(makerReturnInt >= 0)return makerReturnInt;

    makerReturnInt = makeDetectorLevel();
    if(doEmbedding){makerReturnInt = makeGenLevel();}
    if(makerReturnInt >= 0) return makerReturnInt;

    if(doEventDebug){
        cout<<"TStarEvent summary: "<<endl;
        tsEvent->Print();
        cout<<"Filling tree"<<endl;
    }

    if(doEventDebug)cout<<"**************Finished StMyAnalysisMaker::Make()********************"<<endl;

    return kStOk;
}

int StMyAnalysisMaker::getMakers(){
    picoDstMaker = static_cast<StPicoDstMaker*>(GetMaker("picoDst"));
    if(!picoDstMaker){
        cout<<"You havent added a StPicoDstMaker!"<<endl;
        return kStFatal;
    }if(doEventDebug)cout<<"Got StPicoDstMaker!"<<endl;

    if(doJetAnalysis){
        if(doJetDebug)cout<<"Getting jet maker..."<<endl;
        jetMaker = static_cast<StMyJetMaker*>(GetMaker("jetMaker"));
        if(!jetMaker){
            cout<<" No jet maker found!"<<endl;
            return kStFatal;
        }if(doJetDebug)cout<<"Got StMyJetMaker!"<<endl;
    }

    if(doEmbedding){
        if(doPythiaEvent){
            pythiaEventMaker = static_cast<StPythiaEventMaker*>(GetMaker("pythiaEventMaker"));
            if(!pythiaEventMaker){
                cout<<" No Pythia event maker found!"<<endl;
                return kStFatal;
            }if(doEventDebug)cout<<"Got Pythia event maker!"<<endl;
        }
        if(doJetAnalysis){
            if(doJetDebug)cout<<"Getting gen-level jet maker..."<<endl;
            genJetMaker = static_cast<StMyJetMaker*>(GetMaker("genJetMaker"));
            if(!genJetMaker){
                cout<<" No Gen-level jet maker found!"<<endl;
                return kStFatal;
            }if(doJetDebug)cout<<"Got Gen level StJetMaker!"<<endl;
        }
    }        
    return -1;
}

int StMyAnalysisMaker::makeEvent(){
    picoEvent = static_cast<StPicoEvent*>(StPicoDst::event());
    if(!picoEvent){
        cout<<" No PicoEvent! Skip! " << endl;
        return kStWarn;
    }if(doEventDebug){cout<<"Got StPicoEvent!"<<endl;}fillHist1D("hEventStats", 0);

    if(pythiaEventMaker){
        if(doEventDebug)cout<<"Getting Pythia event..."<<endl;
        pythiaEvent = static_cast<StPythiaEvent*>(pythiaEventMaker->pythiaEvent());
        if(!pythiaEvent){
            cout<<" No Pythia event found! SKIP!"<<endl;
            return kStWarn;
        }if(doEventDebug)cout<<"Got Pythia event!"<<endl;

        if((pythiaEvent->runId() != picoEvent->runId()) || (pythiaEvent->eventId() != picoEvent->eventId())){
            cout<<"Pythia event and Pico event out of sync! SKIP!"<<endl;
            return kStWarn;
        }if(doEventDebug)cout<<"Pythia event and Pico event in sync!"<<endl;
    }

    //Reject bad runs here..., if doing run by run jobs, reject bad runs while submitting jobs
    runID = picoEvent->runId();
    if(!doRunbyRun && badRuns.count(runID)>0){
        if(doEventDebug)cout<<"Bad run: "<<runID<<endl;
        return kStOK;
    }fillHist1D("hEventStats", 1);

    pVtx = picoEvent->primaryVertex();
    //primary Z vertex cut...
    if(abs(pVtx.z()) > absZVtx_Max){
        if(doEventDebug)cout<<"Bad Z vertex: "<<pVtx.z()<<endl;
        return kStOK;
    }fillHist1D("hEventStats", 2);

    tsEvent = TStarArrays::addEvent();
    tsEvent->setIdNumbers(runID, picoEvent->eventId());
    tsEvent->setPrimaryVertex(pVtx);
    tsEvent->setRefMults(picoEvent->grefMult(), picoEvent->refMult()); 
    tsEvent->setZDCCoincidence(picoEvent->ZDCx());
    tsEvent->setBBCCoincidence(picoEvent->BBCx());
    tsEvent->setVPDVz(picoEvent->vzVpd());

    if(!doppAnalysis){
        int makerReturnInt = runStRefMultCorr();
        if(makerReturnInt >= 0) return makerReturnInt;
    }

    if(!doEmbedding){
        int makerReturnInt = setUpTriggers();
        if(makerReturnInt >= 0) return makerReturnInt;
    }else if(doEventDebug)cout<<"Embedding mode, skipping trigger selection here..."<<endl;

    return -1;

}

int StMyAnalysisMaker::makeDetectorLevel(){
    if(doEventDebug)cout<<"************StMyAnalysisMaker::makeDetectorLevel()**************"<<endl;
    
    runOverTracks(); //Runs over all tracks

    runOverTowers(); //Runs over all towers

    if(doEmbedding && selectHTEventsOnly){
       if(maxTowerEt < highTowerThreshold){
           if(doEventDebug)cout<<"No high tower in event with max tower et: "<<maxTowerEt<<endl;
           return kStOK;
       }fillHist1D("hEventStats", 3);
    }

    if(doEventDebug)cout<<"Max track pt: "<<maxTrackPt<<" Max tower Et: "<<maxTowerEt<<endl;

    fillHist2D("h2MaxTrkPtvTowEt", maxTrackPt, maxTowerEt, Wt);

    return -1;
}

int StMyAnalysisMaker::makeGenLevel(){
    if(doEventDebug)cout<<"************StMyAnalysisMaker::makeGenLevel()**************"<<endl;

    runOverGenTracks(); //Runs over all gen tracks

    if(doPythiaEvent){
        StPythiaEvent *pyEvt = TStarArrays::addPythiaEvent();
        pyEvt->set(*pythiaEvent);
    }

    return -1;
}

int StMyAnalysisMaker::runStRefMultCorr(){
    if(doEventDebug)cout<<"**********StMyAnalysisMaker::runStRefMultCorr()************"<<endl;

    centscaled = -1;

    if(doppAnalysis){
        if(doEventDebug)cout<<"Doing pp analysis, skipping centrality determination"<<endl;
        return -1;
    }
    if(!grefmultCorr){
        cout<<"ERROR: Doing heavy-ion analysis without StRefMultCorr!"<<endl;
        return -1;
    }
    grefmultCorr->init(runID);
    grefmultCorr->initEvent(tsEvent->gRefMult(), tsEvent->Vz(), tsEvent->ZDC_Coincidence());
    if(doEventDebug)cout<<"grefmultCorr->initEvent() done"<<endl;
  
/*  centrality bins:0: 75-80% 70-80% / 1: 70-75% 60-70% 
                    2: 65-70% 50-60% / 3: 60-65% 40-50% 
                    4: 55-60% 30-40% / 5: 50-55% 20-30% 
                    6: 45-50% 10-20% / 7: 40-45% 5-10% 
                    8: 35-40% 0- 5%  / 9: 30-35% 10 25-30% 
                    11:20-25% / 12: 15-20% / 13: 10-15% 
                    14:5-10% / 15: 0- 5% */
    //centbin9 = grefmultCorr->getCentralityBin9();
    //ref9 = 8-centbin9;
    centbin16 = grefmultCorr->getCentralityBin16();
    if(doEventDebug)cout<<"Got centrality bin..."<<centbin16<<endl;

    if(centbin16 < 0){
        if(doEventDebug)cout<<"Bad centrality bin: "<<centbin16<<endl;
        return kStOK;
    }fillHist1D("hEventStats", 4);

    ref16 = 15-centbin16; 
    centscaled = 5.0*ref16 + 2.5;
    if(doEventDebug)cout<<"Got centrality scaled: "<<centscaled<<"%"<<endl;

    tsEvent->setCentrality(centscaled);
    tsEvent->setCorrectedRefmult(grefmultCorr->getRefMultCorr(tsEvent->gRefMult(), tsEvent->Vz(), tsEvent->ZDC_Coincidence(), 2));
    Wt = Wt0*grefmultCorr->getWeight();
    tsEvent->setWeight(Wt);

    if(doEventDebug)cout<<"Got corrected refmult: "<<tsEvent->refMultCorr()<<" Event weight: "<<Wt<<endl;
    
    fillHist1D("hgRefMult", picoEvent->grefMult(), Wt);
    fillHist1D("hRefMult", tsEvent->refMultCorr(), Wt);
    fillHist1D("hCentrality", centscaled, Wt);
    fillHist2D("h2CentvWeight", centscaled, Wt);

    if(doCentSelection){
        if((centscaled < centralityMin) || (centscaled > centralityMax)){
            if(doEventDebug)cout<<"Centrality selection failed: "<<centscaled<<endl;
            return kStOK;
        }fillHist1D("hEventStats", 5); 
    }
    //grefmultCorrUtil->init(runID);
    //grefmultCorrUtil->initEvent(tsEvent->gRefMult(), tsEvent->Vz(), tsEvent->ZDC_Coincidence()); 
    //tsEvent->SetMB5toMB30Reweight((tsEvent->IsMB5() && !tsEvent->IsMB30()) ? grefmultCorrUtil->getWeight() : 1.0);
    if(doEventDebug)cout<<"********** END StMyAnalysisMaker::runStRefMultCorr() done************"<<endl;
    return -1;
}

int StMyAnalysisMaker::setUpTriggers(){
    eventTriggers = picoEvent->triggerIds();
    if(doTriggerDebug){
        cout<<"Reading event Triggers: "<<endl;
        for(const auto& trig : eventTriggers){
            cout<<trig<<endl;
        }
        cout<<"***************************"<<endl;
    }
    tsEvent->setTriggers(eventTriggers);
    fillHist1D("hTriggerStats", 0); 
    bool hasTrigger = false;
    if(tsEvent->isMBmon()){
        fillHist1D("hTriggerStats", 1);
        hasTrigger = true;
        if(doTriggerDebug)cout<<"MBmon event!"<<endl;
    }
    if(tsEvent->isMB5()){
        fillHist1D("hTriggerStats", 2);
        hasTrigger = true;
        if(doTriggerDebug)cout<<"MB5 event!"<<endl;
    }
    if(tsEvent->isMB30()){
        fillHist1D("hTriggerStats", 3);
        hasTrigger = true;
        if(doTriggerDebug)cout<<"MB30 event!"<<endl;
    } 
    if(tsEvent->isHT1()){
        fillHist1D("hTriggerStats", 4);
        hasTrigger = true;
        if(doTriggerDebug)cout<<"HT1 event!"<<endl;
    }  
    if(tsEvent->isHT2()){
        fillHist1D("hTriggerStats", 5);
        hasTrigger = true;
        if(doTriggerDebug)cout<<"HT2 event!"<<endl;
    }  
    if(tsEvent->isHT3()){
        fillHist1D("hTriggerStats", 6);
        hasTrigger = true;
        if(doTriggerDebug)cout<<"HT3 event!"<<endl;
    }
    if(!hasTrigger){
        if(doTriggerDebug)cout<<"No trigger found!"<<endl;
        return kStOK;
    }

    if(selectHTEventsOnly){
        if(!tsEvent->isHT()){
            if(doEventDebug || doTriggerDebug)cout<<"Not a HT event!"<<endl;
            return kStOK;
        }fillHist1D("hEventStats", 3);
    }

    if(!doppAnalysis){
        if(tsEvent->isMB5()) fillHist1D("hCentralityMB05", centscaled, Wt);
        if(tsEvent->isMB30())fillHist1D("hCentralityMB30", centscaled, Wt);
        if(tsEvent->isHT2()) fillHist1D("hCentralityHT2" , centscaled, Wt);
        if(tsEvent->isHT3()) fillHist1D("hCentralityHT3" , centscaled, Wt);
    }

    return -1;
}

void StMyAnalysisMaker::runOverTracks(){
    if(doTrackDebug)cout<<"**********StMyAnalysisMaker::runOverTracks()************"<<endl;

    if(doTrackDebug)cout<<"Resetting hadronic correction vectors... "<<endl;
    towerHadCorrSumTrE.assign(4800, 0);
    towerHadCorrMaxTrE.assign(4800, 0);
    towerNTracksMatched.assign(4800, 0);

    if(maxTrackPt > 0){
        if(doTrackDebug)cout<<"Max track pt was at "<<maxTrackPt<<", now set to 0 ..."<<endl;
        maxTrackPt = 0;
    }

    if(TStarArrays::numberOfTracks() > 0){
        cout<<"Track array not cleared from previous event!"<<endl;
    }

    unsigned int nTracks = StPicoDst::numberOfTracks();

    if(doTrackDebug){
        cout<<"*************** Tracks Summary: ***************"<<endl;
        cout<<"Number of tracks: "<<nTracks<<endl;
    }

    for(unsigned int itrk = 0; itrk < nTracks; itrk++){ //begin Track Loop...
        StPicoTrack *trk = static_cast<StPicoTrack*>(StPicoDst::track(itrk));

        if(!isTrackGood(trk)) continue;

        TVector3 trkMom = trk->pMom();
        double trkPt = trkMom.Perp();
        double trkEta = trkMom.Eta();
        double trkPhi = trkMom.Phi();
        short trkChrg = trk->charge();

        int towerMatched = trk->bemcTowerIndex();
        if(towerMatched >= 0){
            double E = sqrt(trkMom.Mag2() + pi0mass*pi0mass);
            towerNTracksMatched[towerMatched]++;   
            towerHadCorrSumTrE[towerMatched] += E;
            towerHadCorrMaxTrE[towerMatched] = max(E, towerHadCorrMaxTrE[towerMatched]);
        }else{fillHist1D("hTrackStats", 7);}

        maxTrackPt = max(trkPt, maxTrackPt);

        TStarTrack* tsTrk = TStarArrays::addTrack();
        tsTrk->setIndex(TStarArrays::numberOfTracks()-1);
        tsTrk->setCharge(trkChrg);
        tsTrk->setVector(trkMom, sqrt(trkMom.Mag2() + pi0mass*pi0mass));
       // double trackingEff = getTrackingEfficiency(trkPt, trkEta, ref16, tsEvent->ZDC_Coincidence(), efficiencyFile);
        //tsTrk->setTrackingEfficiency(trackingEff);
        tsTrk->setMatchedTower(towerMatched);
        tsTrk->setNSigmas(trk->nSigmaPion(), trk->nSigmaKaon(), trk->nSigmaProton(), trk->nSigmaElectron());

        if(doTrackDebug){
            cout<<"_______Done with track: "<<itrk<<"________"<<endl;
            cout<<"Made StPicoTrack into TStarTrack! comparing"<<endl;
            tsTrk->Print();
            cout<<"Adding to track array..."<<endl;
        }

        if(doJetAnalysis){
            jetMaker->addConstituentVector(*tsTrk);
            if((trkPt > jetConstituentMinPt) && doJetDebug){
                cout<<"StPicoTrack: "<<itrk<<" pt: "<<trkPt<<" eta: "<<trkEta<<" phi: "<<trkPhi<<" charge: "<<trkChrg<<endl;
                cout<<"TStarTrack: "<<tsTrk->index()<<" pt: "<<tsTrk->pt()<<" eta: "<<tsTrk->eta()<<" phi: "<<tsTrk->phi()<<" charge: "<<tsTrk->charge()<<endl;
            }
        }
        fillHist1D("hTrackPt", trkPt, Wt);
        fillHist1D("hTrackPtxCh", trkPt*trkChrg, Wt);
        fillHist1D("hTrackEta", trkEta, Wt); 
        fillHist1D("hTrackPhi", tsTrk->phi(), Wt);

       // fillHist2D("h2TrackPtvEff", trkPt, trackingEff, Wt);
        fillHist2D("h2TrackEtavPhi", tsTrk->phi(), trkEta, trkPt*Wt); 
    } //end Track Loop...
    fillHist2D("h2CentvMaxTrackPt", centscaled, maxTrackPt, Wt);
    if(doTrackDebug)cout<<"Final Max track pt: "<<maxTrackPt<<endl;
    tsEvent->setMaxTrackPt(maxTrackPt);
    if(doTrackDebug)cout<<"********** END StMyAnalysisMaker::runOverTracks() done************"<<endl;
}

void StMyAnalysisMaker::runOverTowers(){    
    if(doTowerDebug)cout<<"**********StMyAnalysisMaker::runOverTowers()************"<<endl;
    if(maxTowerEt > 0){
        if(doTowerDebug)cout<<"Max tower Et was at "<<maxTowerEt<<", now set to 0..."<<endl;
        maxTowerEt = 0;
    }
    if(TStarArrays::numberOfTowers() > 0){
        cout<<"Tower array not cleared from previous event!"<<endl;
    }
    unsigned int nTowers = StPicoDst::numberOfBTowHits();
    if(doTowerDebug){
        cout<<"*************** Towers Summary: ***************"<<endl;
        cout<<"Number of towers: "<<nTowers<<endl;
    }
    for(unsigned int itow = 0; itow < nTowers; itow++){
        StPicoBTowHit *tower = static_cast<StPicoBTowHit*>(StPicoDst::btowHit(itow));

        if(!isTowerGood(itow, tower)) continue;
        double towERaw = tower->energy();

        //Get tower's position...
        TVector3 towPos = emcPosition->getPosFromVertex(pVtx, itow+1);
        if(doTowerDebug){
            cout<<"Tower: "<<itow<<" E: "<<towERaw<<" eta: "<<towPos.Eta()<<" phi: "<<towPos.Phi()<<endl;
        }
        double towEta = towPos.Eta();
        if((towEta < towerEtaMin) || (towEta > towerEtaMax)){
            if(doTowerDebug)
                cout<<"Tower eta: "<<towEta<<" outside of range: "<<towerEtaMin<<" to "<<towerEtaMax<<endl;
            continue;
        }fillHist1D("hTowerStats", 4);
        //Start hardonic correction of tower...
        double towE = towERaw;
        if(towerNTracksMatched[itow] != 0){
            if(doTowerDebug){
                cout<<"Tower: "<<itow<<" has "<<towerNTracksMatched[itow]<<" tracks matched ";
                cout<<"sum of energies"<<towerHadCorrSumTrE[itow]<<" max energy: "<<towerHadCorrMaxTrE[itow]<<endl;
            }
            if(hadronicCorrType == HadronicCorrectionType::kFull){towE -= towerHadCorrSumTrE[itow];}
            else if(hadronicCorrType == HadronicCorrectionType::kHighestMatchedTrackE){towE -= towerHadCorrMaxTrE[itow];}
            if(doTowerDebug)cout<<"Tower energy after hadronic correction: "<<towE<<endl;
        }else{fillHist1D("hTowerStats", 7);
            if(doTowerDebug)cout<<"Tower: "<<itow<<" has no matched tracks"<<endl;
        }
        double towEt = towE/cosh(towPos.Eta());
        if(towEt < towerEnergyMin){
            if(doTowerDebug) cout<<"Tower Et: "<<towEt<<" less than minimum: "<<towerEnergyMin<<endl;
            continue;
        }else if(doTowerDebug){
            if(towEt > maxTowerEt)cout<<"Max tower Et changed from: "<<maxTowerEt<<" to "<<towEt<<endl;
        }fillHist1D("hTowerStats", 5);

        maxTowerEt = max(towEt, maxTowerEt);

        fillHist1D("hTowerStats", 6);

        TStarTower* tsTow = TStarArrays::addTower();
        tsTow->setIndex(TStarArrays::numberOfTowers()-1);
        tsTow->setADC(tower->adc());
        tsTow->setRawE(towERaw);
        tsTow->setTowerVector(towPos, towE, pi0mass);
        tsTow->setNMatchedTracks(towerNTracksMatched[itow]);

        if(doTowerDebug){
            cout<<"Made StPicoBTowHit into TStarTower! comparing"<<endl;
            tsTow->Print();
            cout<<"Adding to tower array..."<<endl;
        }
        if(doJetAnalysis){
            jetMaker->addConstituentVector(*tsTow);
            if((towEt > jetConstituentMinPt) && doJetDebug){
                cout<<"StPicoTower: "<<itow<<" Et: "<<towEt<<" eta: "<<towEta<<" phi: "<<towPos.Phi()<<endl;
                cout<<"TStarTower:"<<tsTow->index()<<" Pt: "<<tsTow->pt()<<" eta: "<<tsTow->eta()<<" phi: "<<tsTow->phi()<<endl;
            }
        }
        fillHist1D("hTowerERaw", towERaw, Wt);
        fillHist1D("hTowerE", towE, Wt);
        fillHist1D("hTowerEt", towEt, Wt);
        fillHist1D("hTowerEta", towEta, Wt);
        fillHist1D("hTowerPhi", tsTow->phi(), Wt);
        fillHist2D("h2TowerdECorr", towerHadCorrSumTrE[itow], towerHadCorrMaxTrE[itow], Wt);
        fillHist2D("h2TowerEtavPhi", tsTow->phi(), towEta, towEt*Wt);
    }//end tower loop...
    fillHist2D("h2CentvMaxTowerEt", centscaled, maxTowerEt, Wt);
    if(doTowerDebug)cout<<"Final Max tower Et: "<<maxTowerEt<<endl;
    tsEvent->setMaxTowerEt(maxTowerEt);
    if(doTowerDebug)cout<<"********** END StMyAnalysisMaker::runOverTowers() done************"<<endl;
}

void StMyAnalysisMaker::runOverGenTracks(){
    if(doGenDebug)cout<<"**********StMyAnalysisMaker::runOverGenTracks()************"<<endl;

    if(maxGenTrackPt > 0){
        if(doGenDebug)cout<<"Max gen track pt was at "<<maxGenTrackPt<<", now set to 0 ..."<<endl;
        maxGenTrackPt = 0;
    }
    if(TStarArrays::numberOfGenTracks() > 0){
        cout<<"Gen Track array not cleared from previous event!"<<endl;
    }
    unsigned int nGenTracks = StPicoDst::numberOfMcTracks();
    if(doGenDebug){
        cout<<"*************** Gen level Summary: ***************"<<endl;
        cout<<"Number of gen tracks: "<<nGenTracks<<endl;
    }

    for(unsigned int igen = 0; igen < nGenTracks; igen++){
        StPicoMcTrack *genTrk = static_cast<StPicoMcTrack*>(StPicoDst::mcTrack(igen));

        if(!isGenTrackGood(genTrk)) continue;

        TLorentzVector gen4Mom = genTrk->fourMomentum();

        double genPt = gen4Mom.Pt();
        double genEta = gen4Mom.Eta();
        double genPhi = gen4Mom.Phi();
        int genCharge = genTrk->charge();

        maxGenTrackPt = max(maxGenTrackPt, genPt);  

        TStarGenTrack *tsGenTrk = TStarArrays::addGenTrack();
        tsGenTrk->setVector(gen4Mom);
        tsGenTrk->setIndex(TStarArrays::numberOfGenTracks()-1);
        tsGenTrk->setCharge(genCharge);
        tsGenTrk->setGeantId(genTrk->geantId());
        tsGenTrk->setPdgId(genTrk->pdgId());
        tsGenTrk->setIdVtxStart(genTrk->idVtxStart());
        tsGenTrk->setIdVtxEnd(genTrk->idVtxStop());

        genTrackIndexMap[igen] = TStarArrays::numberOfGenTracks() - 1;

        if(doGenDebug){
            cout<<"Made StPicoMcTrack into TStarGenTrack! comparing"<<endl;
            cout<<"StPicoMcTrack # "<<igen<<" mapped to TStarGenTrack # "<<genTrackIndexMap[igen]<<endl;
            tsGenTrk->Print();
            cout<<"Adding to gen track array..."<<endl;
        }

        if(doJetAnalysis){
            genJetMaker->addConstituentVector(*tsGenTrk);
            if((genPt > jetConstituentMinPt) && doJetDebug){
                cout<<"StPicoMcTrack: "<<igen<<" Pt: "<<genPt<<" eta: "<<genEta<<" phi: "<<genPhi<<endl;
                cout<<"TStarGenTrack:"<<tsGenTrk->index()<<" Pt: "<<tsGenTrk->pt()<<" eta: "<<tsGenTrk->eta()<<" phi: "<<tsGenTrk->phi()<<endl;
            }
        }

        fillHist1D("hGenTrackPt", genPt, Wt);
        fillHist1D("hGenTrackEta", genEta, Wt);
        fillHist1D("hGenTrackPhi", tsGenTrk->phi(), Wt);
        fillHist2D("h2GenTrackEtavPhi", tsGenTrk->phi(), genEta, genPt*Wt);

    }//end gen track loop...

    fillHist2D("h2CentvMaxGenTrackPt", centscaled, maxGenTrackPt, Wt);
    if(doGenDebug)cout<<"Final Max gen track Pt: "<<maxGenTrackPt<<endl;
    tsEvent->setMaxGenTrackPt(maxGenTrackPt);
    if(doGenDebug)cout<<"********** END StMyAnalysisMaker::runOverGenTracks() done************"<<endl;
}

bool StMyAnalysisMaker::isTowerGood(unsigned int itow, StPicoBTowHit* tower){
    if(doTowerDebug)cout<<"**********StMyAnalysisMaker::isTowerGood()************"<<endl;

    if(!tower){
        //if(doDebug)cout<<"Tower pointer is null!"<<endl;
        return false;
    }fillHist1D("hTowerStats", 0);

    if(tower->isBad()){
        //if(doDebug)cout<<"Tower is bad ! 1"<<endl;
        return false;
    }
    if(badTowers.count(itow+1)>0){
        //if(doDebug)cout<<"Tower is bad ! 2"<<endl;
        return false;
    }fillHist1D("hTowerStats", 1);

    if(deadTowers.count(itow+1)>0){
        //if(doDebug)cout<<"Tower is dead!"<<endl;
        return false;
    }fillHist1D("hTowerStats", 2);

    if(tower->energy() < towerEnergyMin){
        if(doTowerDebug)
            cout<<"Tower failed energy cut: "<<tower->energy()<<endl;
        return false;
    }fillHist1D("hTowerStats", 3);

    if(doTowerDebug)cout<<"********** END StMyAnalysisMaker::isTowerGood() done************"<<endl;

    return true;
}


bool StMyAnalysisMaker::isTrackGood(StPicoTrack* trk){
    if(doTrackDebug)cout<<"**********StMyAnalysisMaker::isTrackGood()************"<<endl;

    if(!trk){
        if(doTrackDebug)cout<<"Track pointer is null!"<<endl;
        return false;
    }
    //if(doDebug){cout<<"Reading StPicoTrack..."<<endl;
        //trk->Print();
    //}       
//Check if track is primary...
    if(!(trk->isPrimary())){
        //if(doDebug)cout<<"Track is not primary!"<<endl;
        return false;
    }fillHist1D("hTrackStats", 0);
//Track quality cuts...
//DCA:
    if(trk->gDCA(pVtx).Mag() > trackDCAMax){
        if(doTrackDebug)
            cout<<"Track failed DCA cut: "<<trk->gDCA(pVtx).Mag()<<endl;
        return false;
    }fillHist1D("hTrackStats", 1);
//nHitsFit:
    if(trk->nHitsFit() < trackNHitsFitMin){
        if(doTrackDebug)
            cout<<"Track failed nHitsFit cut: "<<trk->nHitsFit()<<endl;
        return false;
    }fillHist1D("hTrackStats", 2); 
//nHitsRatio:
    if((trk->nHitsFit()/(double)trk->nHitsMax()) < trackNHitsRatioMin){
        if(doTrackDebug)
            cout<<"Track failed nHitsFit/nHitsMax cut: "<<trk->nHitsFit()/(double)trk->nHitsMax()<<endl;
        return false;
    }fillHist1D("hTrackStats", 3); 

    TVector3 trkMom = trk->pMom();
    double trkPt = trkMom.Pt();
//Track Pt:
    if(trkPt < trackPtMin){
        if(doTrackDebug)
            cout<<"Track failed pt cut: "<<trkPt<<endl;
        return false;
    }fillHist1D("hTrackStats", 4); 
//Track Eta:
    double trkEta = trkMom.Eta();
    if((trkEta < trackEtaMin) || (trkEta > trackEtaMax)){
        if(doTrackDebug)
            cout<<"Track failed eta cut: "<<trkEta<<endl;
        return false;
    }fillHist1D("hTrackStats", 5); 

    fillHist1D("hTrackStats", 6); 

    if(doTrackDebug){
        cout<<"********** END StMyAnalysisMaker::isTrackGood() done************"<<endl;
        cout<<"Track accepted: "<<" pt: "<<trkPt<<" eta: "<<trkEta<<" phi: "<<trkMom.Phi()<<" charge: "<<trk->charge()<<endl;
        if(trkPt > maxTrackPt) cout<<"Max track pt changed from: "<<maxTrackPt<<" to "<<trkPt<<endl;
        cout<<"Tower matched: "<<trk->bemcTowerIndex()<<endl;
        //cout<<"Tracking efficiency: "<<getTrackingEfficiency(trkPt, trkEta, ref16, tsEvent->ZDC_Coincidence(), efficiencyFile)<<endl;
        cout <<"NSigma Pion: "<< trk->nSigmaPion();
        cout <<" Kaon: "      << trk->nSigmaKaon();
        cout <<" Proton: "    << trk->nSigmaProton();
        cout <<" Electron: "  << trk->nSigmaElectron()<<endl;
    }

    return true;
}

bool StMyAnalysisMaker::isGenTrackGood(StPicoMcTrack* genTrk){
    if(doGenDebug)cout<<"**********StMyAnalysisMaker::isGenTrackGood()************"<<endl;

    if(!genTrk){
        if(doGenDebug)cout<<"StPicoMcTrack pointer is null!"<<endl;
        return false;
    }fillHist1D("hGenTrackStats", 0);

    unsigned int idVtxStop = genTrk->idVtxStop();
    if(idVtxStop > 0){
        if(doGenDebug)
            cout<<"Gen Track is not final state!"<<endl;
        return false;
    }fillHist1D("hGenTrackStats", 1);

    TVector3 genMom = genTrk->p();
    double genPt = genMom.Pt();
    double genEta = genMom.Eta();
    double genPhi = genMom.Phi();
//Track Pt:
    if(genPt < trackPtMin){
        if(doGenDebug)
            cout<<"GenTrack failed pt cut: "<<genPt<<endl;
        return false;
    }fillHist1D("hGenTrackStats", 2);
//Track Eta:
    if((genEta < trackEtaMin) || (genEta > trackEtaMax)){
        if(doGenDebug)
            cout<<"GenTrack failed eta cut: "<<genEta<<endl;
        return false;
    }fillHist1D("hGenTrackStats", 3);

    fillHist1D("hGenTrackStats", 4);
    if(doGenDebug){
        cout<<"********** END StMyAnalysisMaker::isGenTrackGood() done************"<<endl;
        cout<<"Gen track accepted: "<<" Pt: "<<genPt<<" eta: "<<genEta<<" phi: "<<genPhi<<endl;
        if(genPt > maxGenTrackPt)
            cout<<"Max gen track Pt changed from: "<<maxGenTrackPt<<" to "<<genPt<<endl;  
    }
    return true;
}

void StMyAnalysisMaker::addHist1D(const std::string& key, const std::string& title, const int& nBinsX, const double& xMin, const double& xMax){
    histos1D[key] = new TH1F(key.c_str(), title.c_str(), nBinsX, xMin, xMax);
}

void StMyAnalysisMaker::addHist1D(const std::string& key, const std::string& title, const int& nBinsX, double* xBins){
    histos1D[key] = new TH1F(key.c_str(), title.c_str(), nBinsX, xBins);
}

void StMyAnalysisMaker::addHist2D(const std::string& key, const std::string& title, const int& nBinsX, const double& xMin, const double& xMax, const int& nBinsY, const double& yMin, const double& yMax){
    histos2D[key] = new TH2F(key.c_str(), title.c_str(), nBinsX, xMin, xMax, nBinsY, yMin, yMax);
}

void StMyAnalysisMaker::addHist2D(const std::string& key, const std::string& title, const int& nBinsX, double* xBins, const int& nBinsY, const double& yMin, const double& yMax){
    histos2D[key] = new TH2F(key.c_str(), title.c_str(), nBinsX, xBins, nBinsY, yMin, yMax);
}

void StMyAnalysisMaker::addHist2D(const std::string& key, const std::string& title, const int& nBinsX, const double& xMin, const double& xMax, const int& nBinsY, double* yBins){
    histos2D[key] = new TH2F(key.c_str(), title.c_str(), nBinsX, xMin, xMax, nBinsY, yBins);
}

void StMyAnalysisMaker::addHist2D(const std::string& key, const std::string& title, const int& nBinsX, double* xBins, const int& nBinsY, double* yBins){
    histos2D[key] = new TH2F(key.c_str(), title.c_str(), nBinsX, xBins, nBinsY, yBins);
}

void StMyAnalysisMaker::fillHist1D(const std::string& key, const double& x, const double& weight){
    auto search = histos1D.find(key);
    if(search != histos1D.end())  search->second->Fill(x, weight);
}

void StMyAnalysisMaker::fillHist2D(const std::string& key, const double& x, const double& y, const double& weight){
    auto search = histos2D.find(key);
    if(search != histos2D.end()) search->second->Fill(x, y, weight);
}

double StMyAnalysisMaker::getTrackingEfficiency(double x, double y, int cbin, double zdcx, TFile *infile){
    double effBinContent = -99; // value to be extracted from histogram
    const char *species =  "pion"; // FIXME
    int lumiBin = floor(zdcx/10000);

    char* histName; 
    int effBin = -99;

    if(!doppAnalysis){
        if(x > 4.5) x = 4.5;  // for pt > 4.5 use value at 4.5
        // get 2D pTEta efficiency histo
        histName = Form("hTrack_%s_Efficiency_pTEta_final_centbin%d_lumibin%d", species, cbin, lumiBin);
        TH2F *h = static_cast<TH2F*>(infile->Get(Form("%s", histName)));
        if(!h) cout<<"don't have requested histogram! "<<Form("%s", histName)<<endl;
        h->SetName(Form("%s", histName));
        // get efficiency 
        effBin        = h->FindBin(x, y); // pt, eta
        effBinContent = h->GetBinContent(effBin);
        // delete histo and close input file
        delete h;
    }else{
       if(x > 1.8) x = 1.8;  // for pt > 1.8 use value at 1.8

       histName = Form("hppRun12_PtEtaEfficiency_data_aacuts");
       // changed from double to double
       TH2F *h = static_cast<TH2F*>(infile->Get(Form("%s", histName)));
       if(!h) cout<<"don't have requested histogram! "<<Form("%s", histName)<<endl;
       h->SetName(Form("%s", histName));
       // get efficiency 
       effBin        = h->FindBin(x, y); // pt, eta
       effBinContent = h->GetBinContent(effBin);
       double effBinContentErr = h->GetBinError(effBin);

       // delete histo and close input file
       delete h;
    }

    return effBinContent;
}

void StMyAnalysisMaker::setUpBadRuns(){
    string line;
    string inFileName;
    if(doppAnalysis){
        inFileName = "StRoot/StMyAnalysisMaker/runLists/Y2012_BadRuns_P12id_w_missing_HT.txt";
    }else{
        inFileName = "StRoot/StMyAnalysisMaker/runLists/Y2014_BadRuns_P18ih_w_missing_HT.txt";
    }
    ifstream inFile(inFileName.c_str());

    if( !inFile.good() ) {
        cout << "Can't open " << inFileName <<endl;;
    }else cout<<"Opening Bad Runs file: "<<inFileName<<endl;

    while(getline (inFile, line) ){
        if( line.size()==0 ) continue; // skip empty lines
        if( line[0] == '#' ) continue; // skip comments
        istringstream ss( line );
        while( ss ){
            string entry;
            getline( ss, entry, ',' );
            int ientry = atoi(entry.c_str());
            if(ientry) {
                badRuns.insert( ientry );
                //cout << "Added bad run # "<< ientry<< endl;
            }
        }
    }
}

void StMyAnalysisMaker::setUpBadTowers(){
    string inFileName;
    if(runFlag == RunFlags::kRun12){
        if(jetConstituentMinPt == 0.0)
            inFileName = "StRoot/StMyAnalysisMaker/towerLists/Y2012_BadTowers_P12id.txt";
        else if(jetConstituentMinPt == 0.2)
            inFileName = "StRoot/StMyAnalysisMaker/towerLists/Y2012_BadTowers_P12id_200MeV.txt";
        else if(jetConstituentMinPt == 1.0)
            inFileName = "StRoot/StMyAnalysisMaker/towerLists/Y2012_BadTowers_P12id_1000MeV.txt";
        else if(jetConstituentMinPt == 2.0)
            inFileName = "StRoot/StMyAnalysisMaker/towerLists/Y2012_BadTowers_P12id_2000MeV.txt";
    }else if(runFlag == RunFlags::kRun14){
        if(jetConstituentMinPt == 0.0)
            inFileName = "StRoot/StMyAnalysisMaker/towerLists/Y2014_BadTowers_P18ih.txt";
        else if(jetConstituentMinPt == 0.2)
            inFileName = "StRoot/StMyAnalysisMaker/towerLists/Y2014_BadTowers_P18ih_200MeV.txt";
        else if(jetConstituentMinPt == 1.0)
            inFileName = "StRoot/StMyAnalysisMaker/towerLists/Y2014_BadTowers_P18ih_1000MeV.txt";
        else if(jetConstituentMinPt == 2.0)
            inFileName = "StRoot/StMyAnalysisMaker/towerLists/Y2014_BadTowers_P18ih_2000MeV.txt";
    }

    string line;
    ifstream inFile (inFileName.c_str());
    if( !inFile.good() ) {
        cout << "Can't open " << inFileName <<endl;;
    }else cout<<"Opening Bad Tower file: "<<inFileName<<endl;

    while(getline (inFile, line) ){
        if( line.size()==0 ) continue; // skip empty lines
        if( line[0] == '#' ) continue; // skip comments
        istringstream ss( line );
        while( ss ){
            string entry;
            getline( ss, entry, ',' );
            int ientry = atoi(entry.c_str());
            if(ientry) {
                badTowers.insert( ientry );
                //cout << "Added bad tower # "<< ientry<< endl;
            }
        }
    }
}

void StMyAnalysisMaker::setUpDeadTowers(){
    string inFileName;
    if(runFlag == RunFlags::kRun12) inFileName = "StRoot/StMyAnalysisMaker/towerLists/Y2012_DeadTowers.txt";
    else if(runFlag == RunFlags::kRun14) inFileName = "StRoot/StMyAnalysisMaker/towerLists/Y2014_DeadTowers_P18ih.txt";

     // open infile
    string line;
    ifstream inFile ( inFileName.c_str() );
    if( !inFile.good() ) {
        cout << "Can't open " << inFileName <<endl;;
    }else cout<<"Opening Dead Tower file: "<<inFileName<<endl;

    while(getline (inFile, line) ){
        if( line.size()==0 ) continue; // skip empty lines
        if( line[0] == '#' ) continue; // skip comments
        istringstream ss( line );
        while( ss ){
            string entry;
            getline( ss, entry, ',' );
            int ientry = atoi(entry.c_str());
            if(ientry) {
                deadTowers.insert( ientry );
                //cout << "Added dead tower # "<< ientry<< endl;
            }
        }
    }
}