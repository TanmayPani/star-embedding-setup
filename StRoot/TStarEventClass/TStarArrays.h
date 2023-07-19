#ifndef TStarArrays_h
#define TStarArrays_h

#include <map>
#include <string>

class TClonesArray;
class TTree;

class TObject;
class TStarEvent;
class TStarJet;
class TStarTrack;
class TStarTower;
class TStarGenTrack;

class StPythiaEvent;

class TStarArrays {
public:
    TStarArrays();
    virtual ~TStarArrays();

    void addArray(std::string name);
    void setBranch(TTree* tree);
    void clearArrays();

    TStarEvent* addEvent();
    StPythiaEvent* addPythiaEvent();
    TStarTrack* addTrack();
    TStarTower* addTower();
    TStarJet* addJet();
    TStarGenTrack* addGenTrack();
    TStarJet* addGenJet();

    unsigned int numberOfTracks(){return nArrayElements("tracks");}
    unsigned int numberOfTowers(){return nArrayElements("towers");}
    unsigned int numberOfJets(){return nArrayElements("jets");}
    unsigned int numberOfGenTracks(){return nArrayElements("genTracks");}
    unsigned int numberOfGenJets(){return nArrayElements("genJets");}
    
    TStarEvent* getEvent();
    StPythiaEvent* getPythiaEvent();
    TStarTrack* getTrack(unsigned int i);
    TStarTower* getTower(unsigned int i);
    TStarJet* getJet(unsigned int i);
    TStarGenTrack* getGenTrack(unsigned int i);
    TStarJet* getGenJet(unsigned int i);

    TClonesArray* getArray(const std::string& name);

private:
    TObject* addArrayElement(const std::string& name);

    unsigned int nArrayElements(const std::string& name);
    TObject* getArrayElement(const std::string& name, unsigned int i);

    static std::map<std::string, TClonesArray*> Arrays;

    static std::map<std::string, std::string> Types;
    static std::map<std::string, int> Sizes;
};



#endif