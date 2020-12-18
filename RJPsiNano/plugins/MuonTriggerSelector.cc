// class to produce 2 pat::MuonCollections
// one matched to the Park triggers
// another fitered wrt the Park triggers


#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Framework/interface/EDProducer.h"
#include "FWCore/Utilities/interface/StreamID.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"

#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"

#include "DataFormats/PatCandidates/interface/Muon.h"

#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "DataFormats/PatCandidates/interface/TriggerObjectStandAlone.h"
#include "DataFormats/PatCandidates/interface/PackedTriggerPrescales.h"
#include "DataFormats/PatCandidates/interface/TriggerPath.h"
#include "DataFormats/PatCandidates/interface/TriggerEvent.h"
#include "DataFormats/PatCandidates/interface/TriggerAlgorithm.h"

#include "MagneticField/Engine/interface/MagneticField.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"

#include <TLorentzVector.h>
#include "helper.h"

using namespace std;

constexpr bool debug = true;

class MuonTriggerSelector : public edm::EDProducer {
    
public:
    
    explicit MuonTriggerSelector(const edm::ParameterSet &iConfig);
    
    ~MuonTriggerSelector() override {};
    
    
private:

    virtual void produce(edm::Event&, const edm::EventSetup&);

    edm::EDGetTokenT<std::vector<pat::Muon>> muonSrc_;
    edm::EDGetTokenT<edm::TriggerResults> triggerBits_;
    edm::EDGetTokenT<std::vector<pat::TriggerObjectStandAlone>> triggerObjects_;
    edm::EDGetTokenT<pat::PackedTriggerPrescales> triggerPrescales_;
    edm::EDGetTokenT<reco::VertexCollection> vertexSrc_;

    //for trigger match
    const double maxdR_;

    //for filter wrt trigger
    const double dzTrg_cleaning_; // selects primary vertex

    const double ptMin_;          // min pT in all muons for B candidates
    const double absEtaMax_;      //max eta ""
    const bool softMuonsOnly_;    //cuts muons without soft ID
};


MuonTriggerSelector::MuonTriggerSelector(const edm::ParameterSet &iConfig):
  muonSrc_( consumes<std::vector<pat::Muon>> ( iConfig.getParameter<edm::InputTag>( "muonCollection" ) ) ),
  triggerBits_(consumes<edm::TriggerResults>(iConfig.getParameter<edm::InputTag>("bits"))),
  triggerObjects_(consumes<std::vector<pat::TriggerObjectStandAlone>>(iConfig.getParameter<edm::InputTag>("objects"))),
  triggerPrescales_(consumes<pat::PackedTriggerPrescales>(iConfig.getParameter<edm::InputTag>("prescales"))),
  vertexSrc_( consumes<reco::VertexCollection> ( iConfig.getParameter<edm::InputTag>( "vertexCollection" ) ) ), 
  maxdR_(iConfig.getParameter<double>("maxdR_matching")),
  dzTrg_cleaning_(iConfig.getParameter<double>("dzForCleaning_wrtTrgMuon")),
  ptMin_(iConfig.getParameter<double>("ptMin")),
  absEtaMax_(iConfig.getParameter<double>("absEtaMax")),
  softMuonsOnly_(iConfig.getParameter<bool>("softMuonsOnly"))
{
  // produce 2 collections: trgMuons (tags) and SelectedMuons (probes & tags if survive preselection cuts)
    produces<pat::MuonCollection>("trgMuons"); 
    produces<pat::MuonCollection>("SelectedMuons");
    produces<TransientTrackCollection>("SelectedTransientMuons");  
}



void MuonTriggerSelector::produce(edm::Event& iEvent, const edm::EventSetup& iSetup) {
    
  edm::ESHandle<MagneticField> bFieldHandle;
  iSetup.get<IdealMagneticFieldRecord>().get(bFieldHandle);

  edm::Handle<reco::VertexCollection> vertexHandle;
  iEvent.getByToken(vertexSrc_, vertexHandle);
  //const reco::Vertex & PV = vertexHandle->front();

  if(debug) std::cout << " MuonTriggerSelector::produce " << std::endl;

  edm::Handle<edm::TriggerResults> triggerBits;
  iEvent.getByToken(triggerBits_, triggerBits);
  const edm::TriggerNames &names = iEvent.triggerNames(*triggerBits);

  std::vector<pat::TriggerObjectStandAlone> triggeringMuons;

  //taken from https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookMiniAOD2016#Trigger
  edm::Handle<std::vector<pat::TriggerObjectStandAlone>> triggerObjects;
  iEvent.getByToken(triggerObjects_, triggerObjects);
  if(debug) std::cout << "\n TRIGGER OBJECTS " << std::endl;

  edm::Handle<std::vector<pat::Muon>> muons;
  iEvent.getByToken(muonSrc_, muons);

  std::unique_ptr<pat::MuonCollection>      trgmuons_out   ( new pat::MuonCollection );
  std::unique_ptr<pat::MuonCollection>      muons_out      ( new pat::MuonCollection );
  std::unique_ptr<TransientTrackCollection> trans_muons_out( new TransientTrackCollection );

  // Getting the indexes of the HLT paths
  unsigned int index_dimuon0 = names.triggerIndex("HLT_Dimuon0_Jpsi3p5_Muon2_v5");
  unsigned int index_jpsiTrk = names.triggerIndex("HLT_DoubleMu4_JpsiTrk_Displaced_v15");
  
  
  bool pass_dimuon0= false;
  bool pass_jpsiTrk = false;
  int from_dimuon0 = 0;
  int from_jpsitrk = 0;
  if(index_dimuon0 != triggerBits->size()) 
    pass_dimuon0 = triggerBits->accept(index_dimuon0);

  if(index_jpsiTrk != triggerBits->size()) 
    pass_jpsiTrk = triggerBits->accept(index_jpsiTrk);

  if(debug) std::cout << "pass_dimuuon0 " << pass_dimuon0<<" pass_trk "<<pass_jpsiTrk<<std::endl;
  //if(pass_dimuon0) std::cout << "pass_dimuuon0" << std::endl;
  std::vector<bool> jpsiMuonFlags;
  std::vector<bool> dimuon0Flags;
  std::vector<bool> jpsiTrkFlags;

  bool isMuonFromJpsi = false;
  if(pass_dimuon0 || pass_jpsiTrk) {  //is it trigger by the Dimuon0 trigger?
    for (pat::TriggerObjectStandAlone obj : *triggerObjects) 
    { // note: not "const &" since we want to call unpackPathNames
      obj.unpackFilterLabels(iEvent, *triggerBits);
      obj.unpackPathNames(names);

      isMuonFromJpsi = false;
      //      if(obj.hasFilterLabel("hltVertexmumuFilterJpsiMuon3p5") )
      if(obj.hasFilterLabel("hltDisplacedmumuFilterDoubleMu4Jpsi") )
        isMuonFromJpsi = true;


      from_dimuon0 = 0;
      from_jpsitrk = 0;
      if(obj.hasFilterLabel("hltTripleMuL3PreFiltered222")){
	from_dimuon0 = 1;
      }
      if(obj.hasFilterLabel("hltJpsiTkVertexFilter")){
	from_jpsitrk =1;
      }

      { 
	//for each triggered muon I know which trigger it passes
        dimuon0Flags.push_back(from_dimuon0); //unpaired muon passes the dimuon0
	jpsiTrkFlags.push_back(from_jpsitrk); //unpaired muon passes the jpsitrk
        jpsiMuonFlags.push_back(isMuonFromJpsi); //the muon is from the jpsi
        triggeringMuons.push_back(obj);
        if(debug){ 
	  if(from_dimuon0 == 1){ // print only muons that pass the from dimuon0
          std::cout << "\tTrigger object:  pt " << obj.pt() << ", eta " << obj.eta() << ", phi " << obj.phi() << std::endl;
	        // Print trigger object collection and type
	        std::cout << "\t   Collection: " << obj.collection() << std::endl;
	  }
	}
      } 
    }//trigger objects
  }

  if(debug)
  {
    std::cout << "\n total n of triggering muons = " << triggeringMuons.size() << std::endl;
    for(auto ij : triggeringMuons)
    {
	    std::cout << " >>> components (pt, eta, phi) = " << ij.pt() << " " << ij.eta() << " " << ij.phi() << std::endl;
    }
  }
  //now check for reco muons matched to triggering muons
  std::vector<int> muonIsTrigger(muons->size(), 0);
  std::vector<int> muonIsFromJpsi(muons->size(), 0);
  std::vector<int> muonIsDimuon0Trg(muons->size(), 0);
  std::vector<int> muonIsJpsiTrkTrg(muons->size(), 0);

  for(const pat::Muon & muon : *muons)
  {
    //this is for triggering muon not really need to be configurable
    unsigned int iMuo(&muon - &(muons->at(0)) );
    //if(!(muon.isLooseMuon() && muon.isSoftMuon(PV))) continue;
    //if(muon.triggerObjectMatchByPath("HLT_Dimuon0_Jpsi3p5_Muon2_v5")==nullptr &&  muon.triggerObjectMatchByPath("HLT_DoubleMu4_JpsiTrk_Displaced_v15")==nullptr) continue;
    //    if(muon.triggerObjectMatchByPath("HLT_Dimuon0_Jpsi3p5_Muon2_v5")==nullptr) continue;
    if(muon.triggerObjectMatchByPath("HLT_DoubleMu4_JpsiTrk_Displaced_v15")==nullptr) continue;


    float dRMuonMatching = -1.;
    int recoMuonMatching_index = -1;
    int trgMuonMatching_index = -1;
    for(unsigned int iTrg=0; iTrg<triggeringMuons.size(); ++iTrg)
    {
      if(!dimuon0Flags[iTrg] && !jpsiTrkFlags[iTrg]) continue;
      float dR = reco::deltaR(triggeringMuons[iTrg], muon);

      //it passes the dimuon0 trigger

      //If I don't do this, it mixes the triggers
      if(jpsiTrkFlags[iTrg] && (dR < dRMuonMatching || dRMuonMatching == -1)  && dR < maxdR_)
      {
	      dRMuonMatching = dR;
	      recoMuonMatching_index = iMuo;
	      trgMuonMatching_index = iTrg;
	      if(debug) std::cout<<"trigger dimuon0 "<< dimuon0Flags[iTrg]<<std::endl;
	      if(debug) std::cout << " dR = " << dR <<std::endl;
	      if(debug) std::cout << " HLT = " << triggeringMuons[iTrg].pt() << " " << triggeringMuons[iTrg].eta() << " " << triggeringMuons[iTrg].phi()          << std::endl;
	      if(debug) std::cout << " reco = " << muon.pt() << " " << muon.eta() << " " << muon.phi()          << std::endl;

      }
	    
    }

    //save reco muon 
    if(recoMuonMatching_index != -1)
    {


      if(debug)      std::cout << "HERE" << std::endl;
      if(debug) std::cout << "trgMuonMatching_index: " << trgMuonMatching_index << std::endl;
      if(debug) std::cout << "jpsiMuonFlags.push_back(isMuonFromJpsi)"<< jpsiMuonFlags[trgMuonMatching_index] << std::endl;
      if(debug) std::cout << "dimuon0Flags.push_back(pass_dimuon0)"<< dimuon0Flags[trgMuonMatching_index] << std::endl;
      if(debug) std::cout << "jpsiTrkFlags.push_back(pass_jpsiTrk)"<< jpsiTrkFlags[trgMuonMatching_index] << std::endl;
      if(debug)    std::cout  << "----- reco = " << muon.pt() << " " << muon.eta() << " " << muon.phi() << " " 
		              << " HLT = " << triggeringMuons[trgMuonMatching_index].pt() << " " << triggeringMuons[trgMuonMatching_index].eta() << " " << triggeringMuons[trgMuonMatching_index].phi()
		              << std::endl;

	    pat::Muon recoTriggerMuonCand(muon);
	    recoTriggerMuonCand.addUserInt("trgMuonIndex", trgMuonMatching_index);
	    trgmuons_out->emplace_back(recoTriggerMuonCand);
	    //keep track of original muon index for SelectedMuons collection

	    muonIsTrigger[iMuo] = 1;
	    muonIsFromJpsi[iMuo] = (int)jpsiMuonFlags[trgMuonMatching_index];
	    muonIsDimuon0Trg[iMuo] = (int)dimuon0Flags[trgMuonMatching_index];
	    muonIsJpsiTrkTrg[iMuo] = (int)jpsiTrkFlags[trgMuonMatching_index];
    }


  }


 
  // now produce output for analysis (code simplified loop of trg inside)
  // trigger muon + all compatible in dz with any tag
  for(unsigned int muIdx=0; muIdx<muons->size(); ++muIdx) 
  {
    const pat::Muon& mu = (*muons)[muIdx];
    //selection cuts
    if (mu.pt() < ptMin_) continue;
    if (fabs(mu.eta()) > absEtaMax_) continue;
    //following ID is needed for trigger muons not here
    // anyway it is off in the configuration
    //G: if (softMuonsOnly_ && !mu.isSoftMuon(PV)) continue;

    /* // same PV as the tag muon, both tag and probe only dz selection
    bool SkipMuon=true;
    for (const pat::Muon & trgmu : *trgmuons_out) {
	    if( fabs(mu.vz()-trgmu.vz()) > dzTrg_cleaning_ && dzTrg_cleaning_ >0 )
	      continue;
	    SkipMuon=false;
    } 
    // needs decission: what about events without trg muon? now we SKIP them
    if (SkipMuon)  continue;
    */

    // build transient track
    const reco::TransientTrack muonTT((*(mu.bestTrack())), &(*bFieldHandle)); //sara: check, why not using inner track for muons? 
    if (!muonTT.isValid()) continue;

    muons_out->emplace_back(mu);
    muons_out->back().addUserInt("isTriggering", muonIsTrigger[muIdx]);
    muons_out->back().addUserInt("isJpsiMuon", muonIsFromJpsi[muIdx]);
    muons_out->back().addUserInt("isDimuon0Trg", muonIsDimuon0Trg[muIdx]);
    muons_out->back().addUserInt("isJpsiTrkTrg", muonIsJpsiTrkTrg[muIdx]);

    trans_muons_out->emplace_back(muonTT);
  }

  iEvent.put(std::move(trgmuons_out),    "trgMuons");
  iEvent.put(std::move(muons_out),       "SelectedMuons");
  iEvent.put(std::move(trans_muons_out), "SelectedTransientMuons");
}

DEFINE_FWK_MODULE(MuonTriggerSelector);
