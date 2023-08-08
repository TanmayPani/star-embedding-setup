const double pi = 1.0*TMath::Pi();

void LoadLibs(){
  // load fastjet libraries 3.x
  //gSystem->Load("libCGAL"); - not installed 
  gSystem->Load("$FASTJET/lib/libfastjet");
  gSystem->Load("$FASTJET/lib/libsiscone");
  gSystem->Load("$FASTJET/lib/libsiscone_spherical");
  gSystem->Load("$FASTJET/lib/libfastjetplugins");
  gSystem->Load("$FASTJET/lib/libfastjettools");
  gSystem->Load("$FASTJET/lib/libfastjetcontribfragile");

  // add include path to use its functionality
  gSystem->AddIncludePath("-I$FASTJET/include");

  // load the system libraries - these were defaults
  gROOT->LoadMacro("$STAR/StRoot/StMuDSTMaker/COMMON/macros/loadSharedLibraries.C");
  loadSharedLibraries();

  // these are needed for new / additional classes
  gSystem->Load("libStPicoEvent");
  gSystem->Load("libStPicoDstMaker");

  // my libraries
  gSystem->Load("libStRefMultCorr");
  gSystem->Load("libTStarEventClass");
  gSystem->Load("libStMyAnalysisMaker");
  gSystem->Load("libStPythiaEventMaker");
  gSystem->Load("libStMyJetMaker");

  gSystem->ListLibraries();
} 

void readPicoDstAnalysisMaker(string inputFile="st_physics_adc_15134055_raw_0000000.MuDst.root", string outputFile="test_3.root", int nEvents = 100000000){
//void readPicoDstAnalysisMaker(string inputtype="Pythia6Embedding", string trgsetup="AuAu_200_production_2014", string production="P18ih", string library="SL18h", string suffix="20192901_MuToPico20230718", int pthmin = 30, int pthmax = 40, int jobid = 0, string outputFile="test_4.root", int nEvents = 100000000){
      //string inputFile = Form("fileLists/%s_%s_%s_%s_%s/pt%d_%d_%d.list", inputtype.c_str(), trgsetup.c_str(), production.c_str(), library.c_str(), suffix.c_str(), pthmin, pthmax, jobid);
      // Load necessary libraries and macros
      // check if input file is a picoDst file
      if(inputFile.find(".picoDst.root") != std::string::npos){
        cout << "Input file is a picoDst file" << endl;
        outputFile = inputFile.substr(inputFile.find_last_of("/")+1);
        outputFile.replace(outputFile.find(".picoDst.root"), 13, ".root");
      }

      if(inputFile.find(".MuDst.root") != std::string::npos){
        cout << "Input file is a MuDst file" << endl;
        outputFile = inputFile.substr(inputFile.find_last_of("/")+1);
        outputFile.replace(outputFile.find(".MuDst.root"), 13, ".root");
        //cout<<inputFile<<endl;
        //gROOT->ProcessLine(Form(".x genDst.C(-1,\"picoDst,PicoVtxMode:PicoVtxVpdOrDefault,PicoCovMtxMode:PicoCovMtxWrite\",\"%s\"", inputFile.c_str()));
        inputFile.replace(inputFile.find(".MuDst.root"), 11, ".picoDst.root");
        cout<<inputFile<<endl;
      }

      LoadLibs();

      // create chain to take in makers
      StChain* makerChain = new StChain();

      //boolean flags
      bool doEmbedding = true;
      bool useEmcPidTraits = false;
      bool doJetAnalysis = true;
      bool doFullJets = true;
      bool doPythiaEvent = false;
      bool keepJettyEventsOnly = true;

      // create the picoMaker maker:  (PicoIoMode, inputFile, name="picoDst")
      // - Write PicoDst's: PicoIoMode::IoWrite -> StPicoDstMaker::IoWrite
      // - Read  PicoDst's: PicoIoMode::IoRead  -> StPicoDstMaker::IoRead
      StPicoDstMaker *picoMaker = new StPicoDstMaker(StPicoDstMaker::IoRead, inputFile.c_str(), "picoDst");
        picoMaker->setVtxMode((int)(StPicoDstMaker::PicoVtxMode::Default));
        picoMaker->SetStatus("*", 0);
        picoMaker->SetStatus("Event", 1);
        picoMaker->SetStatus("Track", 1);
        //picoMaker->SetStatus("EmcTrigger", 1);
        picoMaker->SetStatus("BTowHit", 1);
        if(useEmcPidTraits) picoMaker->SetStatus("EmcPidTraits", 1);
      if(doEmbedding){ 
        picoMaker->SetStatus("McVertex", 1);
        picoMaker->SetStatus("McTrack", 1); 
      }

      string histoOutputFile = outputFile;
      histoOutputFile.insert(histoOutputFile.find(".root"), ".hist");
      TFile *histOut = new TFile(histoOutputFile.c_str(), "RECREATE");
      histOut->Close();

      string eventOutputFile = outputFile;
      eventOutputFile.insert(eventOutputFile.find(".root"), ".tree");

      TFile *outFile = new TFile(eventOutputFile.c_str(), "RECREATE");
      //outFile->Close();
      TTree *outTree = new TTree("Events", "Tree with event Info");
      //outTree->SetDirectory(gDirectory);

      float R = 0.4;
      double jetConstituentPtCut = 2.0;

      TStarArrays *tsArrays = new TStarArrays();

      TStarArrays::addArray("event");
      TStarArrays::addArray("tracks");
      TStarArrays::addArray("towers");

      StMyAnalysisMaker *anaMaker = new StMyAnalysisMaker("analysisMaker", outputFile);
      anaMaker->setRunFlag(StMyAnalysisMaker::RunFlags::kRun14);
      anaMaker->setDoppAnalysis(false);
      anaMaker->setDoRunbyRun(true);
      anaMaker->setSelectHTEventsOnly(true);
      anaMaker->setDoJetAnalysis(doJetAnalysis);
      anaMaker->setDoEmbedding(doEmbedding);

      //anaMaker->setDoTrackDebug(true);
      //anaMaker->setDoJetDebug(true);
      //anaMaker->setDoTrackDebug(true);
      //anaMaker->setDoGenDebug(true);   
      anaMaker->setJetConstituentMinPt(jetConstituentPtCut);
      anaMaker->setAbsZVtxMax(30);
      anaMaker->setExcludeNoJetEvents(keepJettyEventsOnly);
  
      anaMaker->setTrackDCAMax(3.0);
      anaMaker->setTrackNHitsFitMin(15);
      anaMaker->setTrackNHitsRatioMin(0.52);
      anaMaker->setTrackPtMin(0.2);
      anaMaker->setTrackEtaMin(-1.0);
      anaMaker->setTrackEtaMax(1.0);

      //anaMaker->setTrackPtMax(30.0);

      anaMaker->setTowerEtaMin(-1.0);
      anaMaker->setTowerEtaMax(1.0);
      anaMaker->setTowerEnergyMin(0.2);
      anaMaker->setTowerHadronicCorrType(StMyAnalysisMaker::HadronicCorrectionType::kFull);

      anaMaker->addHist1D("hEventStats", "Event Statistics", 11, -0.5, 10.5);
      anaMaker->addHist1D("hTriggerStats", "Trigger Statistics", 10, -0.5, 9.5);
      anaMaker->addHist1D("hTrackStats", "Track Statistics", 10, -0.5, 9.5);
      anaMaker->addHist1D("hGenTrackStats", "GenTrack Statistics", 10, -0.5, 9.5);
      anaMaker->addHist1D("hTowerStats", "Tower Statistics", 10, -0.5, 9.5); 

      anaMaker->getHist1D("hEventStats")->GetXaxis()->SetBinLabel(1, "ALL");
      anaMaker->getHist1D("hEventStats")->GetXaxis()->SetBinLabel(2, "RUN GOOD");
      anaMaker->getHist1D("hEventStats")->GetXaxis()->SetBinLabel(3, "VZ PASS");
      anaMaker->getHist1D("hEventStats")->GetXaxis()->SetBinLabel(4, "HAS HT TRIGGER");
      anaMaker->getHist1D("hEventStats")->GetXaxis()->SetBinLabel(5, "CENTRALITY GOOD");
      anaMaker->getHist1D("hEventStats")->GetXaxis()->SetBinLabel(6, "CENTRALITY PASS");
      anaMaker->getHist1D("hEventStats")->GetXaxis()->SetBinLabel(7, "HAS JET CONSTIT");
      anaMaker->getHist1D("hEventStats")->GetXaxis()->SetBinLabel(8, "PT MAX PASS");
      anaMaker->getHist1D("hEventStats")->GetXaxis()->SetBinLabel(9, "HAS JET");
      anaMaker->getHist1D("hEventStats")->GetXaxis()->SetBinLabel(10,"HAS GEN JET");
      anaMaker->getHist1D("hEventStats")->GetXaxis()->SetBinLabel(11,"GOOD");

      anaMaker->getHist1D("hTriggerStats")->GetXaxis()->SetBinLabel(1, "ALL");
      anaMaker->getHist1D("hTriggerStats")->GetXaxis()->SetBinLabel(2, "MBmon");
      anaMaker->getHist1D("hTriggerStats")->GetXaxis()->SetBinLabel(3, "MB5");
      anaMaker->getHist1D("hTriggerStats")->GetXaxis()->SetBinLabel(4, "MB30");
      anaMaker->getHist1D("hTriggerStats")->GetXaxis()->SetBinLabel(5, "HT1xMB30");
      anaMaker->getHist1D("hTriggerStats")->GetXaxis()->SetBinLabel(6, "HT2xMB30");
      anaMaker->getHist1D("hTriggerStats")->GetXaxis()->SetBinLabel(7, "HT3");

      anaMaker->getHist1D("hTrackStats")->GetXaxis()->SetBinLabel(1, "ALL");
      anaMaker->getHist1D("hTrackStats")->GetXaxis()->SetBinLabel(2, "DCA PASS");
      anaMaker->getHist1D("hTrackStats")->GetXaxis()->SetBinLabel(3, "nHitsFit PASS");
      anaMaker->getHist1D("hTrackStats")->GetXaxis()->SetBinLabel(4, "nHitsRatio PASS");
      anaMaker->getHist1D("hTrackStats")->GetXaxis()->SetBinLabel(5, "PT PASS");
      anaMaker->getHist1D("hTrackStats")->GetXaxis()->SetBinLabel(6, "ETA PASS");
      anaMaker->getHist1D("hTrackStats")->GetXaxis()->SetBinLabel(7, "GOOD");
      anaMaker->getHist1D("hTrackStats")->GetXaxis()->SetBinLabel(8, "TOWER MATCHED");

      anaMaker->getHist1D("hGenTrackStats")->GetXaxis()->SetBinLabel(1, "ALL");
      anaMaker->getHist1D("hGenTrackStats")->GetXaxis()->SetBinLabel(2, "FINAL STATE");
      anaMaker->getHist1D("hGenTrackStats")->GetXaxis()->SetBinLabel(3, "PT PASS");
      anaMaker->getHist1D("hGenTrackStats")->GetXaxis()->SetBinLabel(4, "ETA PASS");
      anaMaker->getHist1D("hGenTrackStats")->GetXaxis()->SetBinLabel(5, "GOOD");
      
      anaMaker->getHist1D("hTowerStats")->GetXaxis()->SetBinLabel(1, "ALL");
      anaMaker->getHist1D("hTowerStats")->GetXaxis()->SetBinLabel(2, "GOOD");
      anaMaker->getHist1D("hTowerStats")->GetXaxis()->SetBinLabel(3, "ALIVE");
      anaMaker->getHist1D("hTowerStats")->GetXaxis()->SetBinLabel(4, "RawE PASS");
      anaMaker->getHist1D("hTowerStats")->GetXaxis()->SetBinLabel(5, "ETA PASS");
      anaMaker->getHist1D("hTowerStats")->GetXaxis()->SetBinLabel(6, "Et PASS");
      anaMaker->getHist1D("hTowerStats")->GetXaxis()->SetBinLabel(7, "ALL GOOD");
      anaMaker->getHist1D("hTowerStats")->GetXaxis()->SetBinLabel(8, "NO TRACKS MATCHED");

      StMyJetMaker *jetMaker = NULL;
      StMyJetMaker *genJetMaker = NULL;
      StPythiaEventMaker *pyMaker = NULL;

      string jetAlgo = "antikt_algorithm";
      string recombScheme = "BIpt2_scheme";

      if(doJetAnalysis){
    TStarArrays::addArray("jets");

        jetMaker = new StMyJetMaker("jetMaker", outputFile);
        jetMaker->setJetAlgorithm(jetAlgo);
        jetMaker->setRecombScheme(recombScheme);
        jetMaker->setJetRadius(R);
        jetMaker->setDoFullJet(doFullJets);
        jetMaker->setJetConstituentCut(jetConstituentPtCut);
        jetMaker->setJetPtMin(5.0);
        jetMaker->setJetPtCSMin(jetConstituentPtCut);
        jetMaker->setJetAbsEtaMax(0.6);//1.0-R
      }

      if(doEmbedding){
        TStarArrays::addArray("genTracks");
        if(doPythiaEvent){
          TStarArrays::addArray("pythiaEvent");
          string pythiaInputFile = inputFile;
          pythiaInputFile.insert(pythiaInputFile.find(".list"), ".pythia");
          pyMaker = new StPythiaEventMaker("pythiaEventMaker", inputFile);
        }
        if(doJetAnalysis){
          TStarArrays::addArray("genJets");
          genJetMaker = new StMyJetMaker("genJetMaker", outputFile);
          genJetMaker->setJetAlgorithm(jetAlgo);
          genJetMaker->setRecombScheme(recombScheme);
          genJetMaker->setJetRadius(R);
          genJetMaker->setDoFullJet(doFullJets);
          genJetMaker->setJetConstituentCut(jetConstituentPtCut);
          genJetMaker->setJetPtMin(10.0);
          genJetMaker->setJetPtCSMin(8.0);
          genJetMaker->setJetAbsEtaMax(0.6);//1.0-R
        }
      }

      TStarArrays::setBranch(outTree);
  
       // initialize chain
      makerChain->Init();
      cout<<"makerChain->Init();"<<endl;
      int total = picoMaker->chain()->GetEntries();
      cout << " Total entries = " << total << endl;
      if(nEvents > total) nEvents = total;

      cout<<"Run Flag = "<<TStarEvent::runFlag()<<endl;

      for (Int_t i = 0; i < nEvents; i++){
        if(i%1000 == 0) cout << "Working on eventNumber " << i << endl;
        //cout << "Working on eventNumber " << i << endl;
        TStarArrays::clearArrays();
        makerChain->Clear();
        int iret = makerChain->Make(i); 
        if (iret) { cout << "Bad return code!" << iret << endl; break;}
        //cout<<"n reco jets = "<<TStarArrays::numberOfJets()<<" n gen jets = "<<TStarArrays::numberOfGenJets()<<endl;
        if(doJetAnalysis && keepJettyEventsOnly){
          bool hasJets = (TStarArrays::numberOfJets() > 0);
          if(hasJets)anaMaker->fillHist1D("hEventStats", 8);
          if(doEmbedding){
            bool hasGenJets = (TStarArrays::numberOfGenJets() > 0);
            if(hasGenJets)anaMaker->fillHist1D("hEventStats", 9);
            if(!hasJets && !hasGenJets) continue;
          }else if(!hasJets){continue;}
        }
        anaMaker->fillHist1D("hEventStats", 10);
        outTree->Fill();	
        total++;		
	    }
	
	cout << "****************************************** " << endl;
	cout << "Work done... now its time to close up shop!"<< endl;
	cout << "****************************************** " << endl;
	makerChain->Finish();
	cout << "****************************************** " << endl;
	cout << "total number of events  " << nEvents << endl;
	cout << "****************************************** " << endl;
	
	delete makerChain;	

  outFile->WriteObject(outTree, "Events");
  // close output file if open
  if(outFile->IsOpen()) outFile->Close();
  if(histOut->IsOpen()) histOut->Close();

  //StMemStat::PrintMem("load StChain");
}
