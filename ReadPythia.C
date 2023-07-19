#include <TSystem>

// basic STAR classes
class StMemStat;
class StMaker;
class StChain;
class StPythiaEventMaker;
class StMyEmbeddingMaker;
class StMuDstMaker;
class TStarEvent;

void LoadLibs(){
  // load the system libraries - these were defaults
  gROOT->LoadMacro("$STAR/StRoot/StMuDSTMaker/COMMON/macros/loadSharedLibraries.C");
  loadSharedLibraries();

  // Load shared library for StythiaEvent
  gSystem->Load("libStPythiaEventMaker");
  gSystem->Load("libTStarEmbeddingClass");
  gSystem->Load("libStMyEmbeddingMaker");

  gSystem->ListLibraries();
}

void ReadPythia(string inputFileList = "/star/embed/embedding/pp200_production_2012/v2/Pythia6_pt25_35_100_20212001/P12id.SL12d/2012/047/13047003/st_zerobias_adc_13047003_raw_2570001_r0.MuDst.root", string outputFile = "test.root"){
  // Load necessary libraries and macros
  LoadLibs();

  int nEvents = 100000;

  StChain* chain = new StChain();

  StMuDstMaker *muMaker = new StMuDstMaker(0, 0, "", inputFileList.c_str(), "", 1000);
  muMaker->SetStatus("*", 0);
  muMaker->SetStatus("MuEventAll", 1);
  muMaker->SetStatus("StrangeAll", 1);
  muMaker->SetStatus("EmcAll", 1);

  StPythiaEventMaker *pythiaMaker = new StPythiaEventMaker("pythiaEvent", inputFileList);

  TFile *fout = new TFile(outputFile.c_str(), "RECREATE");
  fout->Close();

  StMyEmbeddingMaker *embedding = new StMyEmbeddingMaker("embeddingMaker", outputFile);

  chain->Init();
  cout<<"Initialized StChain"<<endl;
  int total = muMaker->chain()->GetEntriesFast();
  cout << "Total entries = "<<total<<endl;
  //if(nEvents > total) 
  nEvents = total;

  for (Int_t i = 0; i < nEvents; i++){
    if(i%100 == 0)
    cout << "Working on eventNumber " << i << endl;

    chain->Clear();
    int iret = chain->Make(i);	
    if (iret) { cout << "Bad return code!" << iret << endl; break;}
    total++;		
  }

  cout << "****************************************** " << endl;
  cout << "Work done... now its time to close up shop!"<< endl;
  cout << "****************************************** " << endl;
  chain->Finish();
  cout << "****************************************** " << endl;
  cout << "total number of events  " << nEvents << endl;
  cout << "****************************************** " << endl;

  delete chain;	

  if(fout->IsOpen())   fout->Close();

}
