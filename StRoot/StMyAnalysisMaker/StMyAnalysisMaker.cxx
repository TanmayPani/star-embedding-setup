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
    outputFileName = "EventTree_"+output;
    histoFileName = "Histograms_"+output;

    if(doDebug){
        cout<<"Name of the StMaker instance : "<<anaName<<endl;
        cout<<"Name of the file that will store the output TTree : "<<outputFileName<<endl;
        cout<<"Name of the file that will store the Histograms : "<<histoFileName<<endl;
    }

    towerHadCorrSumTrE.resize(4800);
    towerHadCorrMaxTrE.resize(4800);
    towerNTracksMatched.resize(4800);

    tsArrays = new TStarArrays();

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
    if(!doppAnalysis){
        grefmultCorr = CentralityMaker::instance()->getgRefMultCorr_P18ih_VpdMB30_AllLumi();
        if(doDebug){
            cout<<"Set up grefmultCorr..."<<endl;
            grefmultCorr->print();
        }

        grefmultCorrUtil = new StRefMultCorr("grefmult_P18ih_VpdMB30_AllLumi_MB5sc");
        grefmultCorrUtil->setVzForWeight(16, -16.0, 16.0);
        grefmultCorrUtil->readScaleForWeight("StRoot/StRefMultCorr/macros/weight_grefmult_vpd30_vpd5_Run14_P18ih_set1.txt");
    }

    if(!doRunbyRun)setUpBadRuns(); //Probably deprecated, might add this into StRefMultCorr

    setUpBadTowers();//There may be a better way
    setUpDeadTowers();//There may be a better way
    declareHistograms();

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

    TStarEvent::Class()->IgnoreTObjectStreamer();
    TStarVector::Class()->IgnoreTObjectStreamer();

    tsArrays->addArray("event");
    tsArrays->addArray("tracks");
    tsArrays->addArray("towers");
    if(doEmbedding)tsArrays->addArray("genTracks");
    if(doJetAnalysis)tsArrays->addArray("jets");
    if(doEmbedding && doJetAnalysis)tsArrays->addArray("genJets");

    bookTree();

    if(doDebug)cout<<"***********END OF StMyAnalysisMaker::Init()**************"<<endl;

   // if(treeOut->IsOpen())   treeOut->Close();

    return kStOK;
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

    if(outputFileName != ""){
        if(doDebug)cout<<"Writing tree to "<<outputFileName<<endl;
        //treeOut->cd();
        treeOut->Write();
        treeOut->Close();
    }

    writeHistograms(); 

    efficiencyFile->Close();

    return kStOk;
}

Int_t StMyAnalysisMaker::Make(){
    if(doEventDebug)cout<<"***********StMyAnalysisMaker::Make()**************"<<endl;

    tsArrays->clearArrays();

    int makerReturnInt = getMakers();
    if(makerReturnInt >= 0)return kStOK;
    
    //Reject bad runs here..., if doing run by run jobs, reject bad runs while submitting jobs
    runID = picoEvent->runId();
    if(!doRunbyRun && badRuns.count(runID)>0){
        if(doEventDebug)cout<<"Bad run: "<<runID<<endl;
        return kStOK;
    }histos1D["hEventStats"]->Fill(1);

    pVtx = picoEvent->primaryVertex();
    //primary Z vertex cut...
    if(abs(pVtx.z()) > absZVtx_Max){
        if(doEventDebug)cout<<"Bad Z vertex: "<<pVtx.z()<<endl;
        return kStOK;
    }histos1D["hEventStats"]->Fill(2);

    tsEvent = tsArrays->addEvent();
    tsEvent->setIdNumbers(runID, picoEvent->eventId());
    tsEvent->setPrimaryVertex(pVtx);
    tsEvent->setRefMults(picoEvent->grefMult(), picoEvent->refMult()); 
    tsEvent->setZDCCoincidence(picoEvent->ZDCx());
    tsEvent->setBBCCoincidence(picoEvent->BBCx());
    tsEvent->setVPDVz(picoEvent->vzVpd());

    if(!doppAnalysis){
        makerReturnInt = runStRefMultCorr();
        if(makerReturnInt >= 0) return makerReturnInt;
    }

    if(!doEmbedding){
        makerReturnInt = setUpTriggers();
        if(makerReturnInt >= 0) return makerReturnInt;
    }else{if(doEventDebug)cout<<"Embedding mode, skipping trigger selection here..."<<endl;}

    makerReturnInt = makeDetectorLevel();
    if(doEmbedding){makerReturnInt = makeGenLevel();}
    if(makerReturnInt >= 0) return makerReturnInt;

    if(doEventDebug){
        cout<<"TStarEvent summary: "<<endl;
        tsEvent->Print();
        cout<<"Filling tree"<<endl;
    }
    histos1D["hEventStats"]->Fill(10);

    tree->Fill();

    if(doEventDebug)cout<<"**************Finished StMyAnalysisMaker::Make()********************"<<endl;

    return kStOk;
}

int StMyAnalysisMaker::getMakers(){
    picoDstMaker = static_cast<StPicoDstMaker*>(GetMaker("picoDst"));
    if(!picoDstMaker){
        cout<<"You havent added a StPicoDstMaker!"<<endl;
        return kStFatal;
    }if(doEventDebug)cout<<"Got StPicoDstMaker!"<<endl;

    picoDst = static_cast<StPicoDst*>(picoDstMaker->picoDst());
    if(!picoDst){
        cout << " No PicoDst! Skip! " << endl;
        return kStWarn;
    }if(doEventDebug)cout<<"Got StPicoDst!"<<endl;

    picoEvent = static_cast<StPicoEvent*>(picoDst->event());
    if(!picoEvent){
        cout<<" No PicoEvent! Skip! " << endl;
        return kStWarn;
    }if(doEventDebug){cout<<"Got StPicoEvent!"<<endl;
        cout<<"************ StPicoDst::print() *************"<<endl;
        picoDst->print();
        //picoDst->printTracks();
        //picoDst->printBTowHits();
        cout<<"************ END StPicoDst::print() *************"<<endl;
    }histos1D["hEventStats"]->Fill(0);

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
            pythiaEventMaker = static_cast<StPythiaEventMaker*>(GetMaker("StPythiaEventMaker"));
            if(!pythiaEventMaker){
                cout<<" No Pythia event maker found!"<<endl;
                return kStFatal;
            }if(doEventDebug)cout<<"Got Pythia event maker!"<<endl;
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

int StMyAnalysisMaker::makeDetectorLevel(){
    if(doEventDebug)cout<<"************StMyAnalysisMaker::makeDetectorLevel()**************"<<endl;
    
    runOverTracks(); //Runs over all tracks

    runOverTowers(); //Runs over all towers

    if(doEventDebug)cout<<"Max track pt: "<<maxTrackPt<<" Max tower Et: "<<maxTowerEt<<endl;

    histos2D["h2MaxTrkPtvTowEt"]->Fill(maxTrackPt, maxTowerEt, Wt);

    if(excludeNoJetEvents){
        if((maxTrackPt < jetConstituentMinPt) && (maxTowerEt < jetConstituentMinPt)){
            if(doEventDebug)cout<<"Rejected low energy event: "<<maxTrackPt<<" "<<maxTowerEt<<endl;
            return kStOK;
        }histos1D["hEventStats"]->Fill(6);
    }

    if(doJetAnalysis){
        if(doJetDebug)cout<<"Running over jets..."<<endl;
        int returnCode = runOverJets();
        if(returnCode >= 0) return returnCode;
        histos1D["hEventStats"]->Fill(8);
    }

    if(doEmbedding && selectHTEventsOnly){
        if(maxTowerEt < highTowerThreshold){
            if(doEventDebug)cout<<"No high tower in event with max tower et: "<<maxTowerEt<<endl;
            return kStOK;
        }histos1D["hEventStats"]->Fill(3);
    }


    return -1;
}

int StMyAnalysisMaker::makeGenLevel(){
    if(doEventDebug)cout<<"************StMyAnalysisMaker::makeGenLevel()**************"<<endl;

    runOverGenTracks(); //Runs over all gen tracks

    if(excludeNoJetEvents){
        if(maxGenTrackPt < jetConstituentMinPt){
            if(doEventDebug)cout<<"Rejected low energy event: "<<maxGenTrackPt<<endl;
            return kStOK;
        }histos1D["hEventStats"]->Fill(6);
    }

    int returnCode = -1;

    if(doJetAnalysis){
        if(doJetDebug)cout<<"Running over gen jets..."<<endl;
        returnCode = runOverGenJets();
        if(returnCode >= 0) return returnCode;
    }histos1D["hEventStats"]->Fill(8);

    if(doPythiaEvent){
        StPythiaEvent *pyEvt = tsArrays->addPythiaEvent();
        *pyEvt = *pythiaEvent;
    }

    return -1;
}

void StMyAnalysisMaker::bookTree(){
    if(outputFileName == ""){
        cout<<"Trees are not being written to any file!"<<endl;
        return;
    }else{
        treeOut = new TFile(outputFileName.c_str(), "UPDATE");
        treeOut->cd();
        //treeOut->mkdir(GetName());
        //writedir = (TDirectory*)treeOut->Get(GetName());
        cout<<"Writing tree to: "<<outputFileName<<endl;
        tree = new TTree("Events", "Tree with event Info");
        tree->SetDirectory(gDirectory);
        tsArrays->setBranch(tree); 
        cout<<"Events tree directory set"<<endl;
    }
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
    }histos1D["hEventStats"]->Fill(4);

    ref16 = 15-centbin16; 
    centscaled = 5.0*ref16 + 2.5;
    if(doEventDebug)cout<<"Got centrality scaled: "<<centscaled<<"%"<<endl;

    tsEvent->setCentrality(centscaled);
    tsEvent->setCorrectedRefmult(grefmultCorr->getRefMultCorr(tsEvent->gRefMult(), tsEvent->Vz(), tsEvent->ZDC_Coincidence(), 2));
    Wt = grefmultCorr->getWeight();
    tsEvent->setWeight(Wt);

    if(doEventDebug)cout<<"Got corrected refmult: "<<tsEvent->refMultCorr()<<" Event weight: "<<Wt<<endl;
    
    histos1D["hgRefMult"]->Fill(picoEvent->grefMult(), Wt);
    histos1D["hRefMult"]->Fill(tsEvent->refMultCorr(), Wt);
    histos1D["hCentrality"]->Fill(centscaled, Wt);
    histos2D["h2CentvWeight"]->Fill(centscaled, Wt);

    if(doCentSelection){
        if((centscaled < centralityMin) || (centscaled > centralityMax)){
            if(doEventDebug)cout<<"Centrality selection failed: "<<centscaled<<endl;
            return kStOK;
        }histos1D["hEventStats"]->Fill(5); 
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
    histos1D["hTriggerStats"]->Fill(0); 
    bool hasTrigger = false;
    if(tsEvent->isMBmon()){
        histos1D["hTriggerStats"]->Fill(1);
        hasTrigger = true;
        if(doTriggerDebug)cout<<"MBmon event!"<<endl;
    }
    if(tsEvent->isMB5()){
        histos1D["hTriggerStats"]->Fill(2);
        hasTrigger = true;
        if(doTriggerDebug)cout<<"MB5 event!"<<endl;
    }
    if(tsEvent->isMB30()){
        histos1D["hTriggerStats"]->Fill(3);
        hasTrigger = true;
        if(doTriggerDebug)cout<<"MB30 event!"<<endl;
    } 
    if(tsEvent->isHT1()){
        histos1D["hTriggerStats"]->Fill(4);
        hasTrigger = true;
        if(doTriggerDebug)cout<<"HT1 event!"<<endl;
    }  
    if(tsEvent->isHT2()){
        histos1D["hTriggerStats"]->Fill(5);
        hasTrigger = true;
        if(doTriggerDebug)cout<<"HT2 event!"<<endl;
    }  
    if(tsEvent->isHT3()){
        histos1D["hTriggerStats"]->Fill(6);
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
        }histos1D["hEventStats"]->Fill(3);
    }

    if(!doppAnalysis){
        if(tsEvent->isMB5()) histos1D["hCentralityMB05"]->Fill(centscaled, Wt);
        if(tsEvent->isMB30())histos1D["hCentralityMB30"]->Fill(centscaled, Wt);
        if(tsEvent->isHT2()) histos1D["hCentralityHT2" ]->Fill(centscaled, Wt);
        if(tsEvent->isHT3()) histos1D["hCentralityHT3" ]->Fill(centscaled, Wt);
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

    if(tsArrays->numberOfTracks() > 0){
        cout<<"Track array not cleared from previous event!"<<endl;
    }

    unsigned int nTracks = picoDst->numberOfTracks();

    if(doTrackDebug){
        cout<<"*************** Tracks Summary: ***************"<<endl;
        cout<<"Number of tracks: "<<nTracks<<endl;
    }

    for(unsigned int itrk = 0; itrk < nTracks; itrk++){ //begin Track Loop...
        StPicoTrack *trk = static_cast<StPicoTrack*>(picoDst->track(itrk));

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
        }else{histos1D["hTrackStats"]->Fill(7);}

        maxTrackPt = max(trkPt, maxTrackPt);

        TStarTrack* tsTrk = tsArrays->addTrack();
        tsTrk->setIndex(tsArrays->numberOfTracks()-1);
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
        histos1D["hTrackPt"]->Fill(trkPt, Wt);
        histos1D["hTrackPtxCh"]->Fill(trkPt*trkChrg, Wt);
        histos1D["hTrackEta"]->Fill(trkEta, Wt); 
        histos1D["hTrackPhi"]->Fill(tsTrk->phi(), Wt);

       // histos2D["h2TrackPtvEff"]->Fill(trkPt, trackingEff, Wt);
        histos2D["h2TrackEtavPhi"]->Fill(tsTrk->phi(), trkEta, trkPt*Wt); 
    } //end Track Loop...
    histos2D["h2CentvMaxTrackPt"]->Fill(centscaled, maxTrackPt, Wt);
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
    if(tsArrays->numberOfTowers() > 0){
        cout<<"Tower array not cleared from previous event!"<<endl;
    }
    unsigned int nTowers = picoDst->numberOfBTowHits();
    if(doTowerDebug){
        cout<<"*************** Towers Summary: ***************"<<endl;
        cout<<"Number of towers: "<<nTowers<<endl;
    }
    for(unsigned int itow = 0; itow < nTowers; itow++){
        StPicoBTowHit *tower = static_cast<StPicoBTowHit*>(picoDst->btowHit(itow));

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
        }histos1D["hTowerStats"]->Fill(4);
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
        }else{histos1D["hTowerStats"]->Fill(7);
            if(doTowerDebug)cout<<"Tower: "<<itow<<" has no matched tracks"<<endl;
        }
        double towEt = towE/cosh(towPos.Eta());
        if(towEt < towerEnergyMin){
            if(doTowerDebug) cout<<"Tower Et: "<<towEt<<" less than minimum: "<<towerEnergyMin<<endl;
            continue;
        }else if(doTowerDebug){
            if(towEt > maxTowerEt)cout<<"Max tower Et changed from: "<<maxTowerEt<<" to "<<towEt<<endl;
        }histos1D["hTowerStats"]->Fill(5);

        maxTowerEt = max(towEt, maxTowerEt);

        histos1D["hTowerStats"]->Fill(6);

        TStarTower* tsTow = tsArrays->addTower();
        tsTow->setIndex(tsArrays->numberOfTowers()-1);
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
        histos1D["hTowerERaw"]->Fill(towERaw, Wt);
        histos1D["hTowerE"]->Fill(towE, Wt);
        histos1D["hTowerEt"]->Fill(towEt, Wt);
        histos1D["hTowerEta"]->Fill(towEta, Wt);
        histos1D["hTowerPhi"]->Fill(tsTow->phi(), Wt);
        histos2D["hTowerdESumvMax"]->Fill(towerHadCorrSumTrE[itow], towerHadCorrMaxTrE[itow], Wt);
        histos2D["h2TowerEtavPhi"]->Fill(tsTow->phi(), towEta, towEt*Wt);
    }//end tower loop...
    histos2D["h2CentvMaxTowerEt"]->Fill(centscaled, maxTowerEt, Wt);
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
    if(tsArrays->numberOfGenTracks() > 0){
        cout<<"Gen Track array not cleared from previous event!"<<endl;
    }
    unsigned int nGenTracks = picoDst->numberOfMcTracks();
    if(doGenDebug){
        cout<<"*************** Gen level Summary: ***************"<<endl;
        cout<<"Number of gen tracks: "<<nGenTracks<<endl;
    }

    for(unsigned int igen = 0; igen < nGenTracks; igen++){
        StPicoMcTrack *genTrk = static_cast<StPicoMcTrack*>(picoDst->mcTrack(igen));

        if(!isGenTrackGood(genTrk)) continue;

        TLorentzVector gen4Mom = genTrk->fourMomentum();

        double genPt = gen4Mom.Pt();
        double genEta = gen4Mom.Eta();
        double genPhi = gen4Mom.Phi();
        int genCharge = genTrk->charge();

        maxGenTrackPt = max(maxGenTrackPt, genPt);  

        TStarGenTrack *tsGenTrk = tsArrays->addGenTrack();
        tsGenTrk->setVector(gen4Mom);
        tsGenTrk->setIndex(tsArrays->numberOfGenTracks()-1);
        tsGenTrk->setCharge(genCharge);
        tsGenTrk->setGeantId(genTrk->geantId());
        tsGenTrk->setPdgId(genTrk->pdgId());
        tsGenTrk->setIdVtxStart(genTrk->idVtxStart());
        tsGenTrk->setIdVtxEnd(genTrk->idVtxStop());

        genTrackIndexMap[igen] = tsArrays->numberOfGenTracks() - 1;

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

        histos1D["hGenTrackPt"]->Fill(genPt, Wt);
        histos1D["hGenTrackEta"]->Fill(genEta, Wt);
        histos1D["hGenTrackPhi"]->Fill(tsGenTrk->phi(), Wt);
        histos2D["h2GenTrackEtavPhi"]->Fill(tsGenTrk->phi(), genEta, genPt*Wt);

    }//end gen track loop...

    histos2D["h2CentvMaxGenTrackPt"]->Fill(centscaled, maxGenTrackPt, Wt);
    if(doGenDebug)cout<<"Final Max gen track Pt: "<<maxGenTrackPt<<endl;
    tsEvent->setMaxGenTrackPt(maxGenTrackPt);
    if(doGenDebug)cout<<"********** END StMyAnalysisMaker::runOverGenTracks() done************"<<endl;
}

int StMyAnalysisMaker::runOverJets(){
    if(doJetDebug)cout<<"**********StMyAnalysisMaker::runOverJets()************"<<endl;

    if(tsArrays->numberOfJets() > 0){
        cout<<"Jet array not cleared from previous event!"<<endl;
    }

    unsigned int nJets = jetMaker->clusterJets();
    if(doJetDebug){
        cout<<"*************** Jet Summary: ***************"<<endl;
        cout<<"Number of jets: "<<nJets<<endl;
    }
    if(nJets == 0) return kStOK;

    for(unsigned int ijet = 0; ijet < nJets; ijet++){
        TStarJet *tsJet = tsArrays->addJet();
        tsJet->setJet(jetMaker->getJet(ijet));
        
        if(doJetDebug){
            cout<<"Jet # "<<ijet<<" pt: "<<tsJet->pt()<<" eta: "<<tsJet->eta()<<" phi: "<<tsJet->phi()<<endl;
            cout<<"Jet has "<<tsJet->numberOfConstituents()<<" constituents"<<endl;
        }
        unsigned int icon = 0;
        for(auto& con : tsJet->constituentIndices()){
            unsigned int index = con.first;
            short charge = con.second;
            //cout<<"Constituent # "<<icon<<" index: "<<index<<" charge: "<<charge<<endl;
            if(charge == 0){
                TStarTower *tow = tsArrays->getTower(index);
                tow->setJetIndex(ijet);
                if(doJetDebug){cout<<"___Constituent "<<icon++<<" is tower # ";
                            cout<<index<<" Et: "<<tow->et();
                            cout<<" eta: "<<tow->eta();
                            cout<<" phi: "<<tow->phi()<<endl;}
            }else{
                TStarTrack *trk = tsArrays->getTrack(index);
                trk->setJetIndex(ijet);
                if(doJetDebug)cout<<"___Constituent "<<icon++<<" is track # "<<index<<" pt: "<<trk->pt()<<" eta: "<<trk->eta()<<" phi: "<<trk->phi()<<endl;
            }
        }
    }
    
    if(doJetDebug){cout<<nJets<<" jets clustered, "<<tsArrays->numberOfJets()<<" jets stored."<<endl;
        cout<<"********** END StMyAnalysisMaker::runOverJets() done************"<<endl;}
    return -1;
}

int StMyAnalysisMaker::runOverGenJets(){
    if(doJetDebug)cout<<"**********StMyAnalysisMaker::runOverGenJets()************"<<endl;
    unsigned int nGenJets = genJetMaker->clusterJets();
    if(nGenJets == 0) return kStOK;

    if(doJetDebug){
        cout<<"*************** Gen Jet Summary: ***************"<<endl;
        cout<<"Number of gen jets: "<<nGenJets<<endl;
    }

    for(unsigned int ijet = 0; ijet < nGenJets; ijet++){
        TStarJet *tsGenJet = tsArrays->addGenJet();
        tsGenJet->setJet(genJetMaker->getJet(ijet));

        if(doJetDebug){
            cout<<"Gen Jet # "<<ijet<<" pt: "<<tsGenJet->pt()<<" eta: "<<tsGenJet->eta()<<" phi: "<<tsGenJet->phi()<<endl;
            cout<<"Gen Jet has "<<tsGenJet->numberOfConstituents()<<" constituents"<<endl;
        }
        unsigned int icon = 0;
        for(auto& con : tsGenJet->constituentIndices()){
            unsigned int index = con.first;
            TStarGenTrack *trk = tsArrays->getGenTrack(index);
            trk->setJetIndex(ijet);
            if(doJetDebug)cout<<"___Constituent "<<icon++<<" is gen track # "<<index<<" pt: "<<trk->pt()<<" eta: "<<trk->eta()<<" phi: "<<trk->phi()<<endl;
        }
    }
    if(doJetDebug){cout<<nGenJets<<" gen jets clustered, "<<tsArrays->numberOfGenJets()<<" gen jets stored."<<endl;
        cout<<"********** END StMyAnalysisMaker::runOverGenJets() done************"<<endl;}
    return -1;
}

bool StMyAnalysisMaker::isTowerGood(unsigned int itow, StPicoBTowHit* tower){
    if(doTowerDebug)cout<<"**********StMyAnalysisMaker::isTowerGood()************"<<endl;

    if(!tower){
        //if(doDebug)cout<<"Tower pointer is null!"<<endl;
        return false;
    }histos1D["hTowerStats"]->Fill(0);

    if(tower->isBad()){
        //if(doDebug)cout<<"Tower is bad ! 1"<<endl;
        return false;
    }
    if(badTowers.count(itow+1)>0){
        //if(doDebug)cout<<"Tower is bad ! 2"<<endl;
        return false;
    }histos1D["hTowerStats"]->Fill(1);

    if(deadTowers.count(itow+1)>0){
        //if(doDebug)cout<<"Tower is dead!"<<endl;
        return false;
    }histos1D["hTowerStats"]->Fill(2);

    if(tower->energy() < towerEnergyMin){
        if(doTowerDebug)
            cout<<"Tower failed energy cut: "<<tower->energy()<<endl;
        return false;
    }histos1D["hTowerStats"]->Fill(3);

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
    }histos1D["hTrackStats"]->Fill(0);
//Track quality cuts...
//DCA:
    if(trk->gDCA(pVtx).Mag() > trackDCAMax){
        if(doTrackDebug)
            cout<<"Track failed DCA cut: "<<trk->gDCA(pVtx).Mag()<<endl;
        return false;
    }histos1D["hTrackStats"]->Fill(1);
//nHitsFit:
    if(trk->nHitsFit() < trackNHitsFitMin){
        if(doTrackDebug)
            cout<<"Track failed nHitsFit cut: "<<trk->nHitsFit()<<endl;
        return false;
    }histos1D["hTrackStats"]->Fill(2); 
//nHitsRatio:
    if((trk->nHitsFit()/(double)trk->nHitsMax()) < trackNHitsRatioMin){
        if(doTrackDebug)
            cout<<"Track failed nHitsFit/nHitsMax cut: "<<trk->nHitsFit()/(double)trk->nHitsMax()<<endl;
        return false;
    }histos1D["hTrackStats"]->Fill(3); 

    TVector3 trkMom = trk->pMom();
    double trkPt = trkMom.Pt();
//Track Pt:
    if(trkPt < trackPtMin){
        if(doTrackDebug)
            cout<<"Track failed pt cut: "<<trkPt<<endl;
        return false;
    }histos1D["hTrackStats"]->Fill(4); 
//Track Eta:
    double trkEta = trkMom.Eta();
    if((trkEta < trackEtaMin) || (trkEta > trackEtaMax)){
        if(doTrackDebug)
            cout<<"Track failed eta cut: "<<trkEta<<endl;
        return false;
    }histos1D["hTrackStats"]->Fill(5); 

    histos1D["hTrackStats"]->Fill(6); 

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
    }histos1D["hGenTrackStats"]->Fill(0);

    unsigned int idVtxStop = genTrk->idVtxStop();
    if(idVtxStop > 0){
        if(doGenDebug)
            cout<<"Gen Track is not final state!"<<endl;
        return false;
    }histos1D["hGenTrackStats"]->Fill(1);

    TVector3 genMom = genTrk->p();
    double genPt = genMom.Pt();
    double genEta = genMom.Eta();
    double genPhi = genMom.Phi();
//Track Pt:
    if(genPt < trackPtMin){
        if(doGenDebug)
            cout<<"GenTrack failed pt cut: "<<genPt<<endl;
        return false;
    }histos1D["hGenTrackStats"]->Fill(2);
//Track Eta:
    if((genEta < trackEtaMin) || (genEta > trackEtaMax)){
        if(doGenDebug)
            cout<<"GenTrack failed eta cut: "<<genEta<<endl;
        return false;
    }histos1D["hGenTrackStats"]->Fill(3);

    histos1D["hGenTrackStats"]->Fill(4);
    if(doGenDebug){
        cout<<"********** END StMyAnalysisMaker::isGenTrackGood() done************"<<endl;
        cout<<"Gen track accepted: "<<" Pt: "<<genPt<<" eta: "<<genEta<<" phi: "<<genPhi<<endl;
        if(genPt > maxGenTrackPt)
            cout<<"Max gen track Pt changed from: "<<maxGenTrackPt<<" to "<<genPt<<endl;  
    }
    return true;
}

void StMyAnalysisMaker::declareHistograms(){
    if(doDebug)cout<<"**********StMyAnalysisMaker::declareHistograms()************"<<endl;

    histos1D["hEventStats"]     = new TH1F("hEventStats", "Event Statistics", 11, -0.5, 10.5);
    histos1D["hTriggerStats"]     = new TH1F("hTriggerStats", "Trigger Statistics", 10, -0.5, 9.5);
    histos1D["hRefMult"] = new TH1F("hRefMult", "Reference Multiplicity", 701, -0.5, 700.5);
    histos1D["hgRefMult"] = new TH1F("hgRefMult", "Global Reference Multiplicity", 701, -0.5, 700.5);
    histos1D["hCentrality"]     = new TH1F("hCentrality", "Event Centrality", 16, 0, 80);
    histos1D["hCentralityMB05"] = new TH1F("hCentralityMB05", "Event Centrality for MB5 events", 16, 0, 80); 
    histos1D["hCentralityMB30"] = new TH1F("hCentralityMB30", "Event Centrality for MB30 events", 16, 0, 80); 
    histos1D["hCentralityHT2"]  = new TH1F("hCentralityHT2", "Event Centrality for HT2 events", 16, 0, 80); 
    histos1D["hCentralityHT3"]  = new TH1F("hCentralityHT3", "Event Centrality for HT3 events", 16, 0, 80);

    histos2D["h2CentvWeight"] = new TH2F("h2CentvWeight", "Event Centrality vs RefMultCorr Weight", 16, 0, 80, 25, 0, 2.5);
    histos2D["h2CentvMaxTrackPt"] = new TH2F("h2CentvMaxTrackPt", "Event Centrality vs max(p_{T, track})", 16, 0, 80, 60, 0, 30);
    histos2D["h2CentvMaxGenTrackPt"] = new TH2F("h2CentvMaxGenTrackPt", "Event Centrality vs max(p_{T, gen track})", 16, 0, 80, 60, 0, 30);
    histos2D["h2CentvMaxTowerEt"] = new TH2F("h2CentvMaxTowerEt", "Event Centrality vs max(E_{T, tower})", 16, 0, 80, 80, 0, 40);
    histos2D["h2MaxTrkPtvTowEt"] = new TH2F("h2MaxTrkPtvTowEt", "Event max(p_{T, track}) vs max(E_{T, tower})", 60, 0, 30, 80, 0, 40);

    histos1D["hTrackStats"] = new TH1F("hTrackStats", "Track Statistics", 10, -0.5, 9.5);
    histos1D["hTrackPt"]    = new TH1F("hTrackPt", "p_{T, track}", 60, 0.0, 30);
    histos1D["hTrackPtxCh"]    = new TH1F("hTrackPtxCh", "p_{T, track}#times Charge", 120, -30.0, 30.0);
    histos1D["hTrackEta"]   = new TH1F("hTrackEta", "#eta_{track}", 40, -1.0, 1.0);
    histos1D["hTrackPhi"]   = new TH1F("hTrackPhi", "#phi_{track}", 126, 0.0, 2*TMath::Pi());

    histos2D["h2TrackPtvEff"] = new TH2F("h2TrackPtvEff", "p_{T, track} vs #epsilon_{track}", 60, 0.0, 30, 20, 0.0, 1.0);
    histos2D["h2TrackEtavPhi"] = new TH2F("h2TrackEtavPhi", "#phi_{track} vs #eta_{track}", 126, 0.0, 2*TMath::Pi(), 40, -1.0, 1.0);

    histos1D["hGenTrackStats"] = new TH1F("hGenTrackStats", "GenTrack Statistics", 10, -0.5, 9.5);
    histos1D["hGenTrackPt"]    = new TH1F("hGenTrackPt", "p_{T, track}", 60, 0.0, 30);
    histos1D["hGenTrackEta"]   = new TH1F("hGenTrackEta", "#eta_{track}", 40, -1.0, 1.0);
    histos1D["hGenTrackPhi"]   = new TH1F("hGenTrackPhi", "#phi_{track}", 126, 0.0, 2*TMath::Pi());

    histos2D["h2GenTrackEtavPhi"] = new TH2F("h2GenTrackEtavPhi", "#phi_{track} vs #eta_{track}", 126, 0.0, 2*TMath::Pi(), 40, -1.0, 1.0);

    histos1D["hTowerStats"]  = new TH1F("hTowerStats", "Tower Statistics", 10, -0.5, 9.5); 
    histos1D["hTowerERaw"]   = new TH1F("hTowerERaw", "E_{tower}, Uncorrected", 200, 0, 40);
    histos1D["hTowerE"]      = new TH1F("hTowerE", "E_{tower}, Corrected", 200, 0, 40);
    histos1D["hTowerEt"]     = new TH1F("hTowerEt", "E_{T, tower}", 200, 0, 40);
    histos1D["hTowerEta"]    = new TH1F("hTowerEta", "#eta_{tower}", 40, -1.0, 1.0);
    histos1D["hTowerPhi"]    = new TH1F("hTowerPhi", "#phi_{tower}", 126, 0.0, 2*TMath::Pi());

    histos2D["hTowerdESumvMax"] = new TH2F("hTowerdECorr", "#Delta E_{tower} Correction", 100, 0, 20, 100, 0, 20);
    histos2D["h2TowerEtavPhi"] = new TH2F("hTowerEtavPhi", "#phi_{tower} vs #eta_{tower}", 126, 0.0, 2*TMath::Pi(), 40, -1.0, 1.0);

    histos1D["hEventStats"]->GetXaxis()->SetBinLabel(1, "ALL");
    histos1D["hEventStats"]->GetXaxis()->SetBinLabel(2, "RUN GOOD");
    histos1D["hEventStats"]->GetXaxis()->SetBinLabel(3, "VZ PASS");
    histos1D["hEventStats"]->GetXaxis()->SetBinLabel(4, "HAS HT TRIGGER");
    histos1D["hEventStats"]->GetXaxis()->SetBinLabel(5, "CENTRALITY GOOD");
    histos1D["hEventStats"]->GetXaxis()->SetBinLabel(6, "CENTRALITY PASS");
    histos1D["hEventStats"]->GetXaxis()->SetBinLabel(7, "HAS JET CONSTIT");
    histos1D["hEventStats"]->GetXaxis()->SetBinLabel(8, "PT MAX PASS");
    histos1D["hEventStats"]->GetXaxis()->SetBinLabel(9, "HAS JET");
    histos1D["hEventStats"]->GetXaxis()->SetBinLabel(10,"HAS GEN JET");
    histos1D["hEventStats"]->GetXaxis()->SetBinLabel(11,"GOOD");

    histos1D["hTriggerStats"]->GetXaxis()->SetBinLabel(1, "ALL");
    histos1D["hTriggerStats"]->GetXaxis()->SetBinLabel(2, "MBmon");
    histos1D["hTriggerStats"]->GetXaxis()->SetBinLabel(3, "MB5");
    histos1D["hTriggerStats"]->GetXaxis()->SetBinLabel(4, "MB30");
    histos1D["hTriggerStats"]->GetXaxis()->SetBinLabel(5, "HT1xMB30");
    histos1D["hTriggerStats"]->GetXaxis()->SetBinLabel(6, "HT2xMB30");
    histos1D["hTriggerStats"]->GetXaxis()->SetBinLabel(7, "HT3");

    histos1D["hTrackStats"]->GetXaxis()->SetBinLabel(1, "ALL");
    histos1D["hTrackStats"]->GetXaxis()->SetBinLabel(2, "DCA PASS");
    histos1D["hTrackStats"]->GetXaxis()->SetBinLabel(3, "nHitsFit PASS");
    histos1D["hTrackStats"]->GetXaxis()->SetBinLabel(4, "nHitsRatio PASS");
    histos1D["hTrackStats"]->GetXaxis()->SetBinLabel(5, "PT PASS");
    histos1D["hTrackStats"]->GetXaxis()->SetBinLabel(6, "ETA PASS");
    histos1D["hTrackStats"]->GetXaxis()->SetBinLabel(7, "GOOD");
    histos1D["hTrackStats"]->GetXaxis()->SetBinLabel(8, "TOWER MATCHED");

    histos1D["hGenTrackStats"]->GetXaxis()->SetBinLabel(1, "ALL");
    histos1D["hGenTrackStats"]->GetXaxis()->SetBinLabel(2, "FINAL STATE");
    histos1D["hGenTrackStats"]->GetXaxis()->SetBinLabel(3, "PT PASS");
    histos1D["hGenTrackStats"]->GetXaxis()->SetBinLabel(4, "ETA PASS");
    histos1D["hGenTrackStats"]->GetXaxis()->SetBinLabel(5, "GOOD");
 
    histos1D["hTowerStats"]->GetXaxis()->SetBinLabel(1, "ALL");
    histos1D["hTowerStats"]->GetXaxis()->SetBinLabel(2, "GOOD");
    histos1D["hTowerStats"]->GetXaxis()->SetBinLabel(3, "ALIVE");
    histos1D["hTowerStats"]->GetXaxis()->SetBinLabel(4, "RawE PASS");
    histos1D["hTowerStats"]->GetXaxis()->SetBinLabel(5, "ETA PASS");
    histos1D["hTowerStats"]->GetXaxis()->SetBinLabel(6, "Et PASS");
    histos1D["hTowerStats"]->GetXaxis()->SetBinLabel(7, "ALL GOOD");
    histos1D["hTowerStats"]->GetXaxis()->SetBinLabel(8, "NO TRACKS MATCHED");

    for(const auto& hist : histos1D){
        hist.second->Sumw2();
    }

    for(const auto& hist : histos2D){
        hist.second->Sumw2();
    }

    if(doDebug)cout<<"StMyAnalysisMaker::initHistos() - DONE"<<endl;
        
}

//void StMyAnalysisMaker::setUpTriggers(){
//    if(runFlag == RunFlags::kRun14){
//        trigIds[Triggers::kDefault]     = {450005, 450008, 450009, 450010, 450014, 450015, 450018, 450020, 450024, 450025, 
//                                           450050, 450060, 450201, 450202, 450203, 450211, 450212, 450213 };               
//        trigIds[Triggers::kVPDMB5]      = {450005, 450008, 450009, 450014, 450015, 450018, 450024, 450025, 450050, 450060};
//        trigIds[Triggers::kVPDMB30]     = {450010, 450020};
//        trigIds[Triggers::kHT1xVPDMB30] = {450201, 450211};
//        trigIds[Triggers::kHT2xVPDMB30] = {450202, 450212};
//        trigIds[Triggers::kHT3]         = {450203, 450213};
//    }else if(runFlag == RunFlags::kRun12){
//        trigIds[Triggers::kDefault] = {370001, 370011, 370511, 370546, 390203, 370521, 370522, 370531, 370980, 380204, 
//                                       380205, 380205, 380208, 380206, 380216}; 
//        trigIds[Triggers::kVPDMB]   = {370001, 370011};
//        trigIds[Triggers::kHT1]     = {370511, 370546, 390203};
//        trigIds[Triggers::kHT2]     = {370521, 370522, 370531, 370980, 380204, 380205, 380205, 380208};
//        trigIds[Triggers::kHT3]     = {380206, 380216};
//    }
//
//}

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