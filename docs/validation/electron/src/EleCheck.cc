#include <EleCheck/EleCheck.h>
#include "Acc.h"
#include <cstdlib>
void EleCheck::acc(const std::string &k, double v) { auto &p = sum_[k]; p.first += v; p.second += v * v; ++num_[k]; }
double EleCheck::mean(const std::string &k) const { auto it = sum_.find(k); if (it == sum_.end()) return 0; return it->second.first / num_.at(k); }
void EleCheck::rep(const std::string &k) const {
    auto it = sum_.find(k); if (it == sum_.end()) { std::cout << "  " << k << ": (none)\n"; return; }
    VAcc a; a.s = it->second.first; a.s2 = it->second.second; a.n = num_.at(k); a.mn = a.mx = 0; vreport(k, a);
}
void EleCheck::initializeAnalyzer() {
    if (!myCorr) myCorr = new MyCorrection(DataEra, DataPeriod, IsDATA ? DataStream : MCSample, IsDATA);
    if (const char *p = std::getenv("VALIDATE_DUMP")) {
        dump_ = std::make_shared<std::ofstream>(p);
        *dump_ << "isData,run,raw,scEta,r9,gain,pt,sUp,sDn,smUp,smDn\n";
        dump_->precision(9);
    }
}
void EleCheck::executeEvent() {
    using V = MyCorrection::variation;
    ++nEv_;
    ElectronViewCollection els = GetAllElectronViews();
    ElectronViewCollection noCrack = GetAllElectronViews(true);
    if (noCrack.size() > els.size()) ++cnt_["crackMore"];
    for (std::size_t i = 0; i < noCrack.size(); ++i) {
        const float a = std::fabs(noCrack[i].ScEta());
        if (a > 1.4442f && a < 1.566f) ++cnt_["crackLeft"];
    }
    for (std::size_t i = 0; i < els.size(); ++i) {
        const auto &e = els[i];
        const float raw = e.MiniAODPt();
        if (!(raw > 5.f)) continue;
        ++cnt_["ele"];
        const float pt = e.Pt(), su = e.ScaleUpPt(), sd = e.ScaleDownPt(), mu = e.SmearUpPt(), md = e.SmearDownPt();
        const float sc = e.ScEta(), r9 = e.r9();
        const unsigned char g = e.SeedGain();
        acc("Pt/MiniAODPt", pt / raw);
        if (IsDATA) {
            const float exp = raw * myCorr->GetElectronScaleCorr(sc, g, RunNumber, r9, raw);
            if (!relEq(pt, exp)) ++cnt_["dataScaleMismatch"];
            if (su != pt || sd != pt || mu != pt || md != pt) ++cnt_["dataVarNotNominal"];
        } else {
            const float w = myCorr->GetElectronSmearWidth(raw, r9, sc, V::nom);
            const float wu = myCorr->GetElectronSmearWidth(raw, r9, sc, V::up);
            const float wd = myCorr->GetElectronSmearWidth(raw, r9, sc, V::down);
            if (w > 0.f) {
                const double draw = (pt / raw - 1.) / w;   // the Gaussian pull the framework used
                acc("inferred draw (should be N(0,1))", draw);
                if (!relEq(mu, raw * (1. + draw * wu), 5e-5)) ++cnt_["smearUpMismatch"];
                if (!relEq(md, raw * (1. + draw * wd), 5e-5)) ++cnt_["smearDownMismatch"];
            }
            // EGM: scale uncertainty evaluated on the original pt, applied to the smeared pt
            const float fu = myCorr->GetElectronScaleUnc(sc, g, RunNumber, r9, raw, V::up);
            const float fd = myCorr->GetElectronScaleUnc(sc, g, RunNumber, r9, raw, V::down);
            if (!relEq(su, pt * fu) || !relEq(sd, pt * fd)) ++cnt_["scaleVarMismatch"];
            acc("ScaleUpPt/Pt", su / pt); acc("ScaleDownPt/Pt", sd / pt);
            acc("SmearUpPt/Pt", mu / pt); acc("SmearDownPt/Pt", md / pt);
        }
        // SFs must read the uncalibrated pt
        if (raw > 10.f && std::fabs(e.Eta()) < 2.5f) {
            ++cnt_["sf"];
            const float viaView = myCorr->GetElectronIDSF("Tight", e);
            const float viaRaw = myCorr->GetElectronIDSF("Tight", std::fabs(e.Eta()), raw, e.Phi());
            if (viaView != viaRaw) ++cnt_["idsfNotRaw"];
            if (pt < 10.f) ++cnt_["idsfCorrBelowEdge"];   // would throw: below the SF map's 10 GeV edge
            else if (myCorr->GetElectronIDSF("Tight", std::fabs(e.Eta()), pt, e.Phi()) != viaRaw) ++cnt_["idsfBinMigration"];
            const float recoView = myCorr->GetElectronRECOSF(e);
            const float recoRaw = myCorr->GetElectronRECOSF(std::fabs(e.Eta()), raw, e.Phi());
            if (recoView != recoRaw) ++cnt_["recosfNotRaw"];
        }
        if (dump_)
            *dump_ << (IsDATA ? 1 : 0) << ',' << static_cast<unsigned int>(RunNumber) << ',' << raw << ',' << sc << ',' << r9 << ','
                   << int(g) << ',' << pt << ',' << su << ',' << sd << ',' << mu << ',' << md << '\n';
    }
}
void EleCheck::WriteHist() {
    using V = MyCorrection::variation;
    std::cout << "\n===== EleCheck  " << (IsDATA ? "DATA " + DataStream : "MC " + MCSample) << "  events " << nEv_ << "  electrons " << c("ele") << "\n";
    rep("Pt/MiniAODPt");
    if (IsDATA) {
        vcheck("data: Pt == MiniAODPt * Scale(run,scEta,r9,raw,gain)", c("dataScaleMismatch") == 0, std::to_string(c("dataScaleMismatch")) + " mismatches");
        vcheck("data: all variation lanes equal Pt", c("dataVarNotNominal") == 0);
    } else {
        rep("inferred draw (should be N(0,1))");
        rep("ScaleUpPt/Pt"); rep("ScaleDownPt/Pt"); rep("SmearUpPt/Pt"); rep("SmearDownPt/Pt");
        vcheck("smear up/down reuse the nominal draw on the raw pt", c("smearUpMismatch") + c("smearDownMismatch") == 0,
               std::to_string(c("smearUpMismatch")) + "+" + std::to_string(c("smearDownMismatch")) + " mismatches");
        vcheck("scale up/down = smeared pt * scale_up/down(raw pt)", c("scaleVarMismatch") == 0, std::to_string(c("scaleVarMismatch")) + " mismatches");
    }
    vcheck("ID SF via ElectronView uses MiniAODPt", c("idsfNotRaw") == 0, std::to_string(c("idsfNotRaw")) + "/" + std::to_string(c("sf")));
    vcheck("RECO SF via ElectronView uses MiniAODPt", c("recosfNotRaw") == 0, std::to_string(c("recosfNotRaw")) + "/" + std::to_string(c("sf")));
    std::cout << "  (with the corrected pt instead: SF differs for " << c("idsfBinMigration") << "/" << c("sf")
              << " electrons, and " << c("idsfCorrBelowEdge") << " fall below the 10 GeV map edge and would throw)\n";
    vcheck("skipCrack collection has no crack electrons", c("crackLeft") == 0 && c("crackMore") == 0);
    vcheck("HEEP SF configured", myCorr->HasElectronHEEPIDSF());
    if (myCorr->HasElectronHEEPIDSF()) {
        const float n = myCorr->GetElectronHEEPIDSF(0.5f, 120.f), u = myCorr->GetElectronHEEPIDSF(0.5f, 120.f, V::up), d = myCorr->GetElectronHEEPIDSF(0.5f, 120.f, V::down);
        std::cout << "  HEEP SF(scEta 0.5, pt 120): nom " << n << " up " << u << " down " << d << "\n";
        vcheck("HEEP SF sane (0.8<nom<1.1, down<=nom<=up)", n > 0.8f && n < 1.1f && d <= n && n <= u);
        vcheck("HEEP SF clamps below 35 GeV", myCorr->GetElectronHEEPIDSF(-1.9f, 20.f) == myCorr->GetElectronHEEPIDSF(-1.9f, 35.5f));
        std::cout << "  HEEP SF signed eta: scEta -2.0 -> " << myCorr->GetElectronHEEPIDSF(-2.0f, 60.f) << ", +2.0 -> " << myCorr->GetElectronHEEPIDSF(2.0f, 60.f) << "\n";
    }
    std::cout << "=====\n";
    if (dump_) dump_->close();
    AnalyzerCore::WriteHist();
}
