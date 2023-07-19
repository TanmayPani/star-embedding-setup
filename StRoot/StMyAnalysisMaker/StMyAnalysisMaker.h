#ifndef StMyAnalysisMaker_h
#define StMyAnalysisMaker_h

#include "StMaker.h"

//C++ includes
#include <map>
#include <set>
#include <vector>

//ROOT includes
#include "TVector3.h"

class StRefMultCorr;
class StEmcPosition2;
class StPicoDstMaker;
class StPicoDst;
class StPicoEvent;
class StPicoTrack;
class StPicoMcTrack;
class StPicoBTowHit;
class TStarArrays;
class TStarEvent;
class TStarJet;
class StMyJetMaker;
class StPythiaEvent;
class StPythiaEventMaker;
//class TTree;
class TFile;
class TH1F;
class TH2F;

class StMyAnalysisMaker : public StMaker {
public:
    StMyAnalysisMaker(string name, string output, bool dodebug = false);
    virtual ~StMyAnalysisMaker();

    enum HadronicCorrectionType{kNone = 0, kHighestMatchedTrackE = 1, kFull = 2};
    enum RunFlags{kRun12 = 12, kRun14 = 14};

    // class required functions
    virtual Int_t Init();
    virtual Int_t Make();
    //virtual Int_t Clear();
    virtual Int_t Finish();

    void setDoEventDebug(bool b){doEventDebug = b;}
    void setDoTriggerDebug(bool b){doTriggerDebug = b;}
    void setDoTrackDebug(bool b){doTrackDebug = b;}
    void setDoTowerDebug(bool b){doTowerDebug = b;}
    void setDoJetDebug(bool b){doJetDebug = b;}
    void setDoGenDebug(bool b){doGenDebug = b;}

    void setRunFlag(RunFlags run){runFlag = run;}
    void setDoppAnalysis(bool b){doppAnalysis = b;}
    void setDoRunbyRun(bool b){doRunbyRun = b;}
    void setDoJetAnalysis(bool b){doJetAnalysis = b;}
    void setDoEmbedding(bool b){doEmbedding = b;}

    void setZVtxRange(double min, double max){zVtx_Min = min; zVtx_Max = max;}
    void setAbsZVtxMax(double z){absZVtx_Max = z;}
    void setCentralityRange(int min, int max){centralityMin = min; centralityMax = max; doCentSelection = true;}
    void setExcludeNoJetEvents(bool b){excludeNoJetEvents = b;}
    void setSelectHTEventsOnly(bool b){selectHTEventsOnly = b;}

    void setTrackPtMin(double m)        {trackPtMin = m;}
    void setTrackPtMax(double m)        {trackPtMax = m;}
    void setTrackEtaMin(double m)       {trackEtaMin = m;}
    void setTrackEtaMax(double m)       {trackEtaMax = m;}
    void setTrackDCAMax(double m)       {trackDCAMax = m;}
    void setTrackNHitsFitMin(double m)  {trackNHitsFitMin = m;}
    void setTrackNHitsRatioMin(double m){trackNHitsRatioMin = m;}

    void setTowerEnergyMin(double m){towerEnergyMin = m;}
    void setTowerEtaMin(double m)   {towerEtaMin = m;}
    void setTowerEtaMax(double m)   {towerEtaMax = m;}
    void setTowerHadronicCorrType(HadronicCorrectionType t){hadronicCorrType = t;}
    void setJetConstituentMinPt(double pt){jetConstituentMinPt = pt;}

    //Output Methods...
    TStarEvent* getEvent(){return tsEvent;}

private:
    double pi0mass = 0.13957;
    // bad and dead tower list arrays
    std::set<int>        badTowers;
    std::set<int>        deadTowers;
    // bad run list 
    std::set<int>        badRuns;

    //Various functions to initialize stuff in Init()...
    void setUpBadRuns();
    void setUpBadTowers();
    void setUpDeadTowers();
    void declareHistograms();
    void writeHistograms();

    //To check event for triggers...
    double getTrackingEfficiency(double pt, double eta, int centbin, double zdcx, TFile *infile);

    //To set centrality related info...
    int runStRefMultCorr();

    //To get event related info...
    int getMakers();
    int setUpTriggers();
    int makeDetectorLevel();
    int makeGenLevel();

    void runOverTracks();
    void runOverTowers();
    void runOverGenTracks();

    int  runOverJets();
    int  runOverGenJets();

    void bookTree();

    bool isTrackGood(StPicoTrack *track);
    bool isGenTrackGood(StPicoMcTrack *track);
    bool isTowerGood(unsigned int itow, StPicoBTowHit *tower);

    RunFlags runFlag = RunFlags::kRun14;
    HadronicCorrectionType hadronicCorrType = HadronicCorrectionType::kFull;

    //TFile Object that will contain the .root file containing efficiency histograms
    TFile *efficiencyFile = nullptr;

    //Need these to Set StMaker name, and get required objects from *.PicoDst.root files...
    std::string anaName = "";
    std::string outputFileName = "";
    std::string histoFileName = "";
    StPicoDstMaker *picoDstMaker = nullptr;
    StPicoDst *picoDst = nullptr;
    StPicoEvent *picoEvent = nullptr;

    StMyJetMaker *jetMaker = nullptr;
    StMyJetMaker *genJetMaker = nullptr;

    StPythiaEventMaker *pythiaEventMaker = nullptr;
    StPythiaEvent *pythiaEvent = nullptr;

    TStarArrays *tsArrays = nullptr;
    TStarEvent *tsEvent = nullptr;

    TStarJet *leadingJet = nullptr;
    TStarJet *subleadingJet = nullptr;

    //StRoot/StRefMultCorr objects for centrality calculation...
    StRefMultCorr *grefmultCorr = nullptr;
    StRefMultCorr *grefmultCorrUtil = nullptr;

    //Points to modified StEmcPosition class instance to 
    //get position vectors of towers given a primary vertex...
    StEmcPosition2 *emcPosition = nullptr;

    //Boolean flags to toggle functionalities
    bool doDebug = false;
    bool doEventDebug = false;
    bool doTriggerDebug = false;
    bool doTrackDebug = false;
    bool doGenDebug = false;
    bool doTowerDebug = false;
    bool doJetDebug = false;
    bool doParticleDebug = false;

    bool doppAnalysis = false;
    bool doRunbyRun = false;
    bool selectHTEventsOnly = false;
    //bool doSelectForMBEvents = false;
    bool doCentSelection = false;
    bool excludeNoJetEvents = false;
    bool doJetAnalysis = false;
    bool doFullJets = false;
    bool doEmbedding = false;
    bool doPythiaEvent = false;

    //Event quality cuts
    double zVtx_Min = -40.0;
    double zVtx_Max = 40.0;
    double absZVtx_Max = 40.0;

    //Event analysis cuts
    double centralityMin = 0;
    double centralityMax = 10; 

    //Track quality cuts
    double trackPtMin = 0.2;
    double trackPtMax = 30.0;
    double trackEtaMin = -1.0;
    double trackEtaMax = 1.0;
    double trackDCAMax = 3.0;
    double trackNHitsFitMin = 15;
    double trackNHitsRatioMin = 0.52;

    //Tower quality cuts
    double towerEtaMin = -1.0;
    double towerEtaMax = 1.0;
    double towerEnergyMin = 0.2;

    double highTowerThreshold = 4.0;

    //2-D vector containing all tracks matched to tower
    std::vector<double>        towerHadCorrSumTrE ;  
    std::vector<double>        towerHadCorrMaxTrE ;
    std::vector<unsigned int>  towerNTracksMatched;  

    double maxTrackPt = 0;
    double maxGenTrackPt = 0;
    double maxTowerEt = 0;
    double maxParticlePt = 0;

    //Jet Analysis cuts
    double jetConstituentMinPt = 2.0;

    TFile *treeOut = nullptr;
    TFile *histOut = nullptr;

    TTree *tree = nullptr; 

    unsigned int runID = 0;
    unsigned int eventID = 0;

    TVector3 pVtx;

    int centbin9 = -99;
    int centbin16 = -99;
    int centscaled = -99;
    int ref9 = -99;
    int ref16 = -99;
    double Wt = 1.0;

    std::vector<unsigned int> eventTriggers;

    std::map<unsigned int, unsigned int> genTrackIndexMap;

    std::map<std::string, TH1F*> histos1D;
    std::map<std::string, TH2F*> histos2D;

    ClassDef(StMyAnalysisMaker, 1)
};

#endif