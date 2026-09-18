#include <FixCheck/FixCheck.h>
#include "Acc.h"

void FixCheck::add(A &a, double v) { a.s += v; a.s2 += v * v; ++a.n; a.mn = std::min(a.mn, v); a.mx = std::max(a.mx, v); }
void FixCheck::rep(const char *l, const A &a) const {
    VAcc v; v.s = a.s; v.s2 = a.s2; v.n = a.n; v.mn = a.mn; v.mx = a.mx; vreport(l, v);
}
void FixCheck::initializeAnalyzer() {
    if (!myCorr) myCorr = new MyCorrection(DataEra, DataPeriod, IsDATA ? DataStream : MCSample, IsDATA);
}
void FixCheck::executeEvent() {
    using V = MyCorrection::variation;
    ++nEv_;
    // A1: AK4 JES lanes
    JetViewCollection jets = GetAllJetViews();
    for (std::size_t i = 0; i < jets.size(); ++i) {
        const auto &j = jets[i];
        const float nom = j.SmearedPtNominal();
        if (!(nom > 15.f)) continue;
        ++nJets_;
        const float up = j.JesPtUp(), dn = j.JesPtDown();
        if (up == 0.f || dn == 0.f) ++nJesZero_;
        if (IsDATA) { if (up != nom || dn != nom) ++nJesDataMismatch_; }
        else { add(jesUp_, up / nom); add(jesDown_, dn / nom); }
    }
    if (IsDATA) return;
    // A4: LHEScaleWeight slot
    const int n = static_cast<int>(nLHEScaleWeight);
    ++nLHE_[n];
    if (n > 0) {
        ++nLHEev_;
        const float fu = GetScaleVariation(V::up, V::nom), fd = GetScaleVariation(V::down, V::nom);
        const float ru = GetScaleVariation(V::nom, V::up), rd = GetScaleVariation(V::nom, V::down);
        add(muFUp_, fu); add(muFDown_, fd); add(muRUp_, ru); add(muRDown_, rd);
        if (fu == 1.f) ++nMuFUpIsOne_;
        if (n == 9) {
            if (fu != LHEScaleWeight[5]) ++nMuFUpNotSlot5_;
            if (std::fabs(LHEScaleWeight[4] - 1.f) > 1e-4f) ++nSlot4NotOne_;
            if (LHEScaleWeight[4] == 1.f) ++nOldMuFUpIsOne_;  // what the old fixed mapping returned
        }
    }
}
void FixCheck::WriteHist() {
    std::cout << "\n===== FixCheck  " << (IsDATA ? "DATA " + DataStream : "MC " + MCSample) << "  events " << nEv_ << "\n";
    std::cout << "-- A1 AK4 JES lanes (" << nJets_ << " jets, pt>15)\n";
    vcheck("no JES lane equals 0", nJesZero_ == 0, std::to_string(nJesZero_) + " zero");
    if (IsDATA) vcheck("data: JES lanes equal the nominal", nJesDataMismatch_ == 0, std::to_string(nJesDataMismatch_) + " mismatches");
    else {
        rep("JesPtUp / SmearedPtNominal", jesUp_); rep("JesPtDown / SmearedPtNominal", jesDown_);
        vcheck("MC: up > 1 > down on average", jesUp_.n && jesUp_.s / jesUp_.n > 1. && jesDown_.s / jesDown_.n < 1.);
    }
    if (!IsDATA) {
        std::cout << "-- A4 LHEScaleWeight\n  nLHEScaleWeight:";
        for (auto &[k, v] : nLHE_) std::cout << " [" << k << "]=" << v;
        std::cout << "\n";
        rep("muF up", muFUp_); rep("muF down", muFDown_); rep("muR up", muRUp_); rep("muR down", muRDown_);
        vcheck("muF-up is not identically 1", nMuFUpIsOne_ < nLHEev_, std::to_string(nMuFUpIsOne_) + "/" + std::to_string(nLHEev_) + " equal 1");
        vcheck("9-entry: muF-up reads slot 5", nMuFUpNotSlot5_ == 0, std::to_string(nMuFUpNotSlot5_) + " mismatches");
        vcheck("9-entry: slot 4 is the nominal (==1)", nSlot4NotOne_ == 0, std::to_string(nSlot4NotOne_) + " not 1");
        std::cout << "  (old fixed mapping would have returned slot 4 == 1 for muF-up in " << nOldMuFUpIsOne_ << " events)\n";
    }
    std::cout << "=====\n";
    AnalyzerCore::WriteHist();
}
