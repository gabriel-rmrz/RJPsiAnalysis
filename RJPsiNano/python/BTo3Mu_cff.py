import FWCore.ParameterSet.Config as cms
from PhysicsTools.RJPsiNano.common_cff import *
from PhysicsTools.RJPsiNano.primaryVertices_cff import *


muonPairsForBTo3Mu = cms.EDProducer(
    'DiMuonBuilder',
    src = cms.InputTag('muonTrgSelector', 'SelectedMuons'),  #try to change
    transientTracksSrc = cms.InputTag('muonTrgSelector', 'SelectedTransientMuons'), #try to change
    muon1Selection = cms.string('pt > 1.5'),
    muon2Selection = cms.string(''),
    preVtxSelection = cms.string('abs(userCand("mu1").vz - userCand("mu2").vz) <= 1. && mass() < 5 '
                                 '&& mass() > 0 && charge() == 0 && userFloat("muons12_deltaR") > 0.03'),
    postVtxSelection = cms.string('userFloat("sv_chi2") < 998 && userFloat("sv_prob") > 1.e-5')
)

BTo3Mu = cms.EDProducer(
    'BTo3MuBuilder',
    dimuons = cms.InputTag('muonPairsForBTo3Mu', 'muonPairsForBTo3Mu'),
    pvSelected = cms.InputTag('pvSelector', 'bestVertex'),
    muonTransientTracks = muonPairsForBTo3Mu.transientTracksSrc,
    muons = cms.InputTag('muonTrgSelector', 'SelectedMuons'),
    #kaonsTransientTracks = cms.InputTag('muonTrgSelector', 'SelectedTransientMuons'),
    beamSpot = cms.InputTag("offlineBeamSpot"),
    tracks = cms.InputTag("packedPFCandidates"),
    lostTracks = cms.InputTag("lostTracks"),
    muonSelection = cms.string(''),
    isoTracksSelection = cms.string('pt > 0.7 && abs(eta)<2.5'),
    # This in principle can be different between electrons and muons
    preVtxSelection = cms.string(
        'pt > 3. && userFloat("min_dr") > 0.03'
        #'&& mass < 7. && mass > 4.'
        ),
    postVtxSelection = cms.string(
        #'userInt("sv_OK") == 1 && userFloat("sv_prob") > 0.001 '
        #'&& userFloat("fitted_cos_theta_2D") >= 0'
        'userFloat("fitted_cos_theta_2D") >= 0.'
        #'&& userFloat("fitted_mass") > 4.5 && userFloat("fitted_mass") < 6.'
    ),
    #GEN

    srcGen = cms.InputTag("prunedGenParticles"),
)

BTo3MuTable = cms.EDProducer(
    'SimpleCompositeCandidateFlatTableProducer',
    src = cms.InputTag("BTo3Mu"),
    cut = cms.string(""),
    name = cms.string("BTo3Mu"),
    doc = cms.string("BTo3Mu Variable"),
    singleton=cms.bool(False),
    extension=cms.bool(False),
    variables=cms.PSet(
        # pre-fit quantities                                                      
        RJpsiCandVars,
        #nome branch= nome variabile del .cc
        mu1Idx = uint('mu1_idx'),
        mu2Idx = uint('mu2_idx'),
        mu3Idx = uint('mu3_idx'),
        minDR = ufloat('min_dr'),
        maxDR = ufloat('max_dr'),
        # fit and vtx info                                                                                                    
        #chi2 = ufloat('sv_chi2'),
        ip3D = ufloat('ip3D'),
        ip3D_e = ufloat('ip3D'),
                         
        svprob = ufloat('sv_prob'),
        l_xy = ufloat('l_xy'),
        l_xy_unc = ufloat('l_xy_unc'),
        vtx_x = ufloat('vtx_x'),
        vtx_y = ufloat('vtx_y'),
        vtx_z = ufloat('vtx_z'),
        vtx_ex = ufloat('vtx_ex'), ## only saving diagonal elements of the cov matrix                                         
        vtx_ey = ufloat('vtx_ey'),
        vtx_ez = ufloat('vtx_ez'),
        vtx_chi2 = ufloat('vtx_chi2'),

        jpsi_vtx_x = ufloat('jpsi_vtx_x'),
        jpsi_vtx_y = ufloat('jpsi_vtx_y'),
        jpsi_vtx_z = ufloat('jpsi_vtx_z'),
        jpsi_vtx_ex = ufloat('jpsi_vtx_ex'),
        jpsi_vtx_ey = ufloat('jpsi_vtx_ey'),
        jpsi_vtx_ez = ufloat('jpsi_vtx_ez'),
        jpsi_vtx_chi2 = ufloat('jpsi_vtx_chi2'),

        pv_x = ufloat('pv_x'),
        pv_y = ufloat('pv_y'),
        pv_z = ufloat('pv_z'),
        pv_ex = ufloat('pv_ex'),
        pv_ey = ufloat('pv_ey'),
        pv_ez = ufloat('pv_ez'),
        pv_exy = ufloat('pv_exz'),
        pv_eyz = ufloat('pv_eyz'),
        pv_exz = ufloat('pv_exz'),
        pv_chi2 = ufloat('pv_chi2'),
        # Mll                                                                                                                 
        mll_raw = Var('userCand("dimuon").mass()', float),
        mll_llfit = Var('userCand("dimuon").userFloat("fitted_mass")', float), # this might not work                        
        mllErr_llfit = Var('userCand("dimuon").userFloat("fitted_massErr")', float), # this might not work                  
        mll_fullfit = ufloat('fitted_mll'),
        mll_vtxex= Var('userCand("dimuon").userFloat("vtx_ex")',float),
#        mll_vtxx= Var('userCand("dimuon").userFloat("vtx_x")',float),
#        mll_vtxy= Var('userCand("dimuon").userFloat("vtx_y")',float),
#        mll_vtxz= Var('userCand("dimuon").userFloat("vtx_z")',float),
#        mll_vtxex= Var('userCand("dimuon").userFloat("vtx_ex")',float),
#        mll_vtxey= Var('userCand("dimuon").userFloat("vtx_ey")',float),
#        mll_vtxez= Var('userCand("dimuon").userFloat("vtx_ez")',float),

        # Cos(theta)                                                                                                          
        cos2D = ufloat('cos_theta_2D'),
        fit_cos2D = ufloat('fitted_cos_theta_2D'),
        # post-fit momentum                                                                                                   
        fit_mass = ufloat('fitted_mass'),
        fit_massErr = ufloat('fitted_massErr'),
        fit_pt = ufloat('fitted_pt'),
        fit_eta = ufloat('fitted_eta'),
        fit_phi = ufloat('fitted_phi'),
        fit_mu1_pt = ufloat('fitted_mu1_pt'),
        fit_mu1_eta = ufloat('fitted_mu1_eta'),
        fit_mu1_phi = ufloat('fitted_mu1_phi'),
        fit_mu2_pt = ufloat('fitted_mu2_pt'),
        fit_mu2_eta = ufloat('fitted_mu2_eta'),
        fit_mu2_phi = ufloat('fitted_mu2_phi'),
        fit_mu3_pt = ufloat('fitted_mu3_pt'),
        fit_mu3_eta = ufloat('fitted_mu3_eta'),
        fit_mu3_phi = ufloat('fitted_mu3_phi'),
        mu1_iso03 = ufloat('mu1_iso03'),
        mu1_iso04 = ufloat('mu1_iso04'),
        mu2_iso03 = ufloat('mu2_iso03'),
        mu2_iso04 = ufloat('mu2_iso04'),
        mu3_iso03  = ufloat('mu3_iso03'),
        mu3_iso04  = ufloat('mu3_iso04'),
        b_iso03  = ufloat('b_iso03'),
        b_iso04  = ufloat('b_iso04'),
        n_mu3_used = uint('n_mu3_used'),
        n_mu1_used = uint('n_mu1_used'),
        n_mu2_used = uint('n_mu2_used'),
        #my variables
        #pass_3mu=uint('pass_3mu'),
        m_miss_sq=ufloat('m_miss_2'),
        Q_sq=ufloat('Q_2'),
        pt_miss=ufloat('pt_miss'),
        pt_miss_vec=ufloat('pt_miss_vec'),
        pt_var=ufloat('pt_var'),
        DR=ufloat('DR'),
        E_mu_star=ufloat('E_mu_star'),
        E_mu_canc=ufloat('E_mu_#'),
        m_jpsi=ufloat('m_jpsi'),
        #jPsi_mass_online=ufloat('jPsi_mass_online')                                                         

        #Gen Variables
        is_jpsi_mu=uint("is_jpsi_mu"),
        is_psi2s_mu=uint("is_psi2s_mu"),
        is_chic0_mu=uint("is_chic0_mu"),
        is_chic1_mu=uint("is_chic1_mu"),
        is_chic2_mu=uint("is_chic2_mu"),
        is_hc_mu=uint("is_hc_mu"),
        is_jpsi_tau=uint("is_jpsi_tau"),
        is_psi2s_tau=uint("is_psi2s_tau"),
        is_jpsi_pi=uint("is_jpsi_pi"),
        is_jpsi_3pi=uint("is_jpsi_3pi"),
        is_jpsi_hc=uint("is_jpsi_hc"),
        is_error=uint("is_error"),
        weightGen= ufloat("weightGen")
    )
)

 
CountBTo3Mu = cms.EDFilter("PATCandViewCountFilter",
    maxNumber = cms.uint32(999999),
    minNumber = cms.uint32(0),
    src = cms.InputTag("BTo3Mu")
)


BTo3MuSequence = cms.Sequence(
    (muonPairsForBTo3Mu * pvSelector * BTo3Mu)
)

BToKLLTables = cms.Sequence(BTo3MuTable)

