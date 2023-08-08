void checkHist(string intputfile = "mergedHistograms/pt50_-1_0.hist.root"){
    TFile *f = new TFile(intputfile.c_str());
    TH1D *h = (TH1D*)f->Get("genJetMaker/hJetPt");
    cout << h->GetEntries() << endl;
}
