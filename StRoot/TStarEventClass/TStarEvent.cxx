#define TStarEvent_cxx

#include "TStarEvent.h"

#include "TVector3.h"
#include "TLorentzVector.h"
#include "TClonesArray.h"
#include "TStarTrack.h"
#include "TStarTower.h"
#include "TStarGenTrack.h"

#include <iostream>

ClassImp(TStarEvent);

using namespace std;

map<string, vector<unsigned int>> TStarEvent::_triggerMap = {
    {"MBmon", {450011, 450021}}, 
    {"VPDMB5", {450005, 450008, 450009, 450014, 450015, 450018, 450024, 450025, 450050, 450060}},
    {"VPDMB30", {450010, 450020}}, {"HT1", {450201, 450211}}, {"HT2", {450202, 450212}},
    {"HT3", {450203, 450213}}, {"HT", {450201, 450211, 450202, 450212, 450203, 450213}}
};

TStarEvent::TStarEvent(){

}

TStarEvent::TStarEvent(unsigned int runid, unsigned int eventid){
    _RunID = runid;
    _EventID = eventid;
}

TStarEvent::TStarEvent(const TStarEvent& ev){setEvent(ev);}

TStarEvent::~TStarEvent(){

}

void TStarEvent::setEvent(const TStarEvent& ev){
    _RunID         =  ev._RunID        ;
    _EventID       =  ev._EventID      ;
    _gRefMult      =  ev._gRefMult     ;
    _RefMult       =  ev._RefMult      ;
    _RefMultCorr   =  ev._RefMultCorr  ;
    _Centrality    =  ev._Centrality   ;
    _Weight        =  ev._Weight       ;
    _Triggers      =  ev._Triggers     ;
    _pVtx_Z        =  ev._pVtx_Z       ;
    _pVtx_r        =  ev._pVtx_r       ;
    _VPD_Vz        =  ev._VPD_Vz       ;
    _ZDCxx         =  ev._ZDCxx        ;
    _BBCxx         =  ev._BBCxx        ;
    _Rho           =  ev._Rho          ;
    _Sigma         =  ev._Sigma        ;
    _MaxTrackPt    =  ev._MaxTrackPt   ;
    _MaxGenTrackPt =  ev._MaxGenTrackPt;
    _MaxTowerEt    =  ev._MaxTowerEt   ;
    _MaxJetPt      =  ev._MaxJetPt     ;
    _MaxGenJetPt   =  ev._MaxGenJetPt  ;
}

void TStarEvent::setPrimaryVertex(TVector3& pVtx){
    _pVtx_Z = pVtx.z();
    _pVtx_r = pVtx.Perp();
}

bool TStarEvent::isTriggered(unsigned int trig) const {
    return (std::find(_Triggers.begin(), _Triggers.end(), trig) != _Triggers.end());
}

bool TStarEvent::isTriggered(std::string trig) const {
    if(_triggerMap.find(trig) == _triggerMap.end()) return false;
    bool res = false;
    for(auto t : _triggerMap[trig]) res = res || isTriggered(t);
    return res;
}

void TStarEvent::Print(Option_t* ) const{
    cout<<"********** Event Info from TStarEvent **********"<<endl;
    cout<<"RunID: "<<_RunID<<" EventID: "<<_EventID<<endl;
    cout<<"gRefMult: "<<_gRefMult<<" RefMult: "<<_RefMult<<" RefMultCorr: "<<_RefMultCorr<<endl;
    cout<<"Centrality: "<<_Centrality<<" Weight: "<<_Weight<<endl;
    cout<<"pVtx_Z: "<<_pVtx_Z<<" pVtx_r: "<<_pVtx_r<<"VPD Vz: "<<_VPD_Vz<<endl;
    cout<<"ZDCxx: "<<_ZDCxx<<" BBCxx: "<<_BBCxx<<endl;
    cout<<"MaxTrackPt: "<<_MaxTrackPt<<" MaxTowerEt: "<<_MaxTowerEt<<" MaxGenTrackPt: "<<_MaxGenTrackPt<<endl;
    cout<<"MaxJetPt: "<<_MaxJetPt<<" MaxGenJetPt: "<<_MaxGenJetPt<<endl;
    cout<<"Rho: "<<_Rho<<" Sigma: "<<_Sigma<<endl;

    for(unsigned int i = 0; i < _Triggers.size(); ++i){
        cout<<"Trigger "<<i<<": "<<_Triggers[i]<<endl;
    }
    cout<<"MB_mon: "<<isMBmon()<<endl;
    cout<<"MB30: "<<isMB30()<<endl;
    cout<<"MB5: "<<isMB5()<<endl;
    cout<<"HT1: "<<isHT1()<<endl;
    cout<<"HT2: "<<isHT2()<<endl;
    cout<<"HT3: "<<isHT3()<<endl;

    cout<<"********** End of Event Info **********"<<endl;
}


