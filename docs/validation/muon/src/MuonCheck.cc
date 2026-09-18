#include <MuonCheck/MuonCheck.h>
#include "Acc.h"
#include <cstdlib>
void MuonCheck::acc(const std::string &k, double v) { auto &p = sum_[k]; p.first += v; p.second += v * v; ++num_[k]; }
double MuonCheck::mean(const std::string &k) const { auto it = sum_.find(k); if (it == sum_.end()) return 0; return it->second.first / num_.at(k); }
void MuonCheck::rep(const std::string &k) const {
    auto it = sum_.find(k); if (it == sum_.end()) { std::cout << "  " << k << ": (none)\n"; return; }
    VAcc a; a.s = it->second.first; a.s2 = it->second.second; a.n = num_.at(k); a.mn = a.mx = 0; vreport(k, a);
}
void MuonCheck::initializeAnalyzer() {
    if (!myCorr) myCorr = new MyCorrection(DataEra, DataPeriod, IsDATA ? DataStream : MCSample, IsDATA);
    if (const char *p = std::getenv("VALIDATE_DUMP")) {
        dump_ = std::make_shared<std::ofstream>(p);
        *dump_ << "isData,event,lumi,charge,raw,eta,phi,nL,pt,sUp,sDn,rUp,rDn,regime,tuneP,highPt,hsUp,hsDn,hrUp,hrDn\n";
        dump_->precision(9);
    }
}
void MuonCheck::executeEvent() {
    ++nEv_;
    if (!myCorr->HasMuonScaleSmearing()) ++cnt_["noPOG"];
    MuonViewCollection mus = GetAllMuonViews();
    for (std::size_t i = 0; i < mus.size(); ++i) {
        const auto &m = mus[i];
        const float raw = m.MiniAODPt();
        if (!(raw > 0.f)) continue;
        ++cnt_["mu"];
        const float pt = m.Pt(), su = m.MomentumScaleUp(), sd = m.MomentumScaleDown(), ru = m.MomentumResUp(), rd = m.MomentumResDown();
        if (!std::isfinite(pt) || !std::isfinite(su) || !std::isfinite(ru)) ++cnt_["nonfinite"];
        if (raw < 26.f || raw > 200.f) { ++cnt_["outside"]; if (pt != raw || su != raw || sd != raw || ru != raw || rd != raw) ++cnt_["outsideMismatch"]; }
        else { acc("Pt/MiniAODPt (26<raw<200)", pt / raw); acc("ScaleUp/Pt", su / pt); acc("ScaleDown/Pt", sd / pt); acc("ResUp/Pt", ru / pt); acc("ResDown/Pt", rd / pt); }
        if (IsDATA && (ru != pt || rd != pt)) ++cnt_["dataResNotNominal"];
        const bool reg = m.IsHighPtRegime();
        const float tp = m.TunePPt(), hp = m.HighPtPt();
        if (reg) { ++cnt_["highReg"]; acc("HighPtPt/TunePPt (regime)", hp / tp); }
        else if (hp != pt || m.HighPtResUp() != ru || m.HighPtResDown() != rd || m.HighPtScaleUp() != su || m.HighPtScaleDown() != sd) ++cnt_["lowLaneMismatch"];
        if (dump_)
            *dump_ << (IsDATA ? 1 : 0) << ',' << static_cast<unsigned long long>(event) << ',' << static_cast<unsigned int>(luminosityBlock) << ','
                   << m.Charge() << ',' << raw << ',' << m.Eta() << ',' << m.Phi() << ',' << int(m.nTrackerLayers()) << ','
                   << pt << ',' << su << ',' << sd << ',' << ru << ',' << rd << ',' << int(reg) << ',' << tp << ',' << hp << ','
                   << m.HighPtScaleUp() << ',' << m.HighPtScaleDown() << ',' << m.HighPtResUp() << ',' << m.HighPtResDown() << '\n';
    }
    // opt-in selection is callable and returns a subset
    auto sel = SelectHighPtMuonIndices(mus, MuonView::MuonID::POG_TIGHT, 50.f, 2.4f);
    if (sel.size() > mus.size()) ++cnt_["badSel"];
}
void MuonCheck::WriteHist() {
    std::cout << "\n===== MuonCheck  " << (IsDATA ? "DATA " + DataStream : "MC " + MCSample) << "  events " << nEv_ << "  muons " << c("mu") << "\n";
    vcheck("MUO POG scale/smearing configured (not Rochester)", c("noPOG") == 0);
    rep("Pt/MiniAODPt (26<raw<200)"); rep("ScaleUp/Pt"); rep("ScaleDown/Pt"); rep("ResUp/Pt"); rep("ResDown/Pt"); rep("HighPtPt/TunePPt (regime)");
    vcheck("all lanes finite", c("nonfinite") == 0);
    vcheck("outside 26<pt<200 every lane equals the input pt", c("outsideMismatch") == 0, std::to_string(c("outsideMismatch")) + "/" + std::to_string(c("outside")));
    if (IsDATA) vcheck("data: resolution lanes equal nominal", c("dataResNotNominal") == 0);
    else vcheck("MC: ScaleUp > 1 > ScaleDown on average", mean("ScaleUp/Pt") > 1 && mean("ScaleDown/Pt") < 1);
    vcheck("below 200 GeV high-pT lanes mirror medium-pT lanes", c("lowLaneMismatch") == 0, std::to_string(c("lowLaneMismatch")) + " mismatches");
    std::cout << "  high-pT regime muons: " << c("highReg") << "\n";
    vcheck("SelectHighPtMuonIndices usable", c("badSel") == 0);
    std::cout << "=====\n";
    if (dump_) dump_->close();
    AnalyzerCore::WriteHist();
}
