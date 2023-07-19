#define TStarTrack_cxx

#include "TStarTrack.h"
#include "TVector3.h"

#include <iostream>

ClassImp(TStarTrack);

using namespace std;

TStarTrack::TStarTrack(){

}

TStarTrack::TStarTrack(const TStarTrack& t) : TStarVector(t){
    _GenMatchId = t._GenMatchId;
    _trackingEff = t._trackingEff;
    _MatchedTowerIndex = t._MatchedTowerIndex;
    _nSigmaPion = t._nSigmaPion;
    _nSigmaKaon = t._nSigmaKaon;
    _nSigmaProton = t._nSigmaProton;
    _nSigmaElectron = t._nSigmaElectron;
}

TStarTrack::~TStarTrack(){
    
}

void TStarTrack::Print(Option_t *) const {
    cout << "_____TStarTrack: ____________" << endl;
    TStarVector::Print();
    cout << "Tracking Efficiency: " << _trackingEff <<" Matched Gen Track: "<< _GenMatchId << " Matched Tower Index: " << _MatchedTowerIndex << endl;
    cout << "nSigmaPion: " << _nSigmaPion << " nSigmaKaon: " << _nSigmaKaon <<" nSigmaProton: " << _nSigmaProton <<" nSigmaElectron: " << _nSigmaElectron << endl;
}

