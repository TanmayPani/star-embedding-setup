#ifndef TStarJet_h
#define TStarJet_h

#include <map>

#include "TStarVector.h"

class TStarJet : public TStarVector {
public:
    TStarJet();
    TStarJet(unsigned int i, float pt, float eta, float phi, float e);
    TStarJet(const TStarJet& j);
    virtual ~TStarJet();

    float area(){return _A;}
    float Ax(){return _Ax;}
    float Ay(){return _Ay;}
    float Az(){return _Az;}
    float localRho(){return _Rho;}
    float localSigma(){return _Sigma;}
    float numberOfConstituents(){return _ConstituentIndices.size();}
    std::multimap<unsigned int, short> constituentIndices(){return _ConstituentIndices;}

    void setArea(float a, float ax = 0, float ay = 0, float az = 0){_A = a; _Ax = ax; _Ay = ay; _Az = az;}   
    void setLocalRho(float rho){_Rho = rho;}
    void setLocalSigma(float sig){_Sigma = sig;}
    void addConstituent(unsigned int i, short ch){_ConstituentIndices.emplace(i, ch);}
    void addConstituent(TStarVector& v){_ConstituentIndices.emplace(v.index(), v.charge());}

    void setJet(const TStarJet& jet);

    void clearConstituentArray(){_ConstituentIndices.clear();}

    unsigned int _Index = 0;
    float _A = 0;
    float _Ax = 0;
    float _Ay = 0;
    float _Az = 0;
    float _Rho = 0;
    float _Sigma = 0;

    std::multimap<unsigned int, short> _ConstituentIndices;

    ClassDef(TStarJet, 1);
};

#endif