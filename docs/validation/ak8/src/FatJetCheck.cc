#include <FatJetCheck/FatJetCheck.h>
#include "Acc.h"
#include <cstdlib>
void FatJetCheck::acc(const std::string &k, double v) { auto &p = sum_[k]; p.first += v; p.second += v * v; ++num_[k]; }
double FatJetCheck::mean(const std::string &k) const { auto it = sum_.find(k); if (it == sum_.end()) return 0; return it->second.first / num_.at(k); }
void FatJetCheck::rep(const std::string &k) const {
    auto it = sum_.find(k); if (it == sum_.end()) { std::cout << "  " << k << ": (none)\n"; return; }
    VAcc a; a.s = it->second.first; a.s2 = it->second.second; a.n = num_.at(k); a.mn = a.mx = 0; vreport(k, a);
}
void FatJetCheck::initializeAnalyzer() {
    if (!myCorr) myCorr = new MyCorrection(DataEra, DataPeriod, IsDATA ? DataStream : MCSample, IsDATA);
    if (const char *p = std::getenv("VALIDATE_DUMP")) {
        dump_ = std::make_shared<std::ofstream>(p);
        *dump_ << "isData,run,pt,rawFactor,area,eta,phi,rho,corr,smNom,smUp,smDn,jesUp,jesDn,sd,sdNom,sdJerUp,sdJerDn,sdJesUp,sdJesDn,sj1,sj2\n";
        dump_->precision(9);
    }
}
void FatJetCheck::executeEvent() {
    ++nEv_;
    const float rho = static_cast<float>(Rho_fixedGridRhoFastjetAll.get());
    FatJetViewCollection fjs = GetAllFatJets();
    for (std::size_t i = 0; i < fjs.size(); ++i) {
        const auto fj = fjs[i];
        if (!(fj.Pt() > 170.f)) continue;
        ++cnt_["fj"];
        const float corr = fj.CorrectedPt(), nom = fj.SmearedPtNominal();
        const float su = fj.SmearedPtUp(), sd = fj.SmearedPtDown(), ju = fj.JesPtUp(), jd = fj.JesPtDown();
        acc("CorrectedPt / Pt(stored)", corr / fj.Pt());
        if (!std::isfinite(corr) || !std::isfinite(nom) || !std::isfinite(ju) || !std::isfinite(su)) ++cnt_["nonfinite"];
        if (IsDATA) { if (nom != corr || su != corr || sd != corr || ju != corr || jd != corr) ++cnt_["dataVar"]; }
        else {
            acc("SmearedPtNominal / CorrectedPt", nom / corr);
            acc("SmearedPtUp / nominal", su / nom); acc("SmearedPtDown / nominal", sd / nom);
            acc("JesPtUp / nominal", ju / nom); acc("JesPtDown / nominal", jd / nom);
            if (!(ju > nom && jd < nom)) ++cnt_["jesOrder"];
        }
        const float sdm = fj.SDMass();
        if (sdm > 0.f) {
            ++cnt_["sd"];
            const float sdn = fj.SDMassNominal();
            acc("SDMassNominal / SDMass", sdn / sdm);
            if (fj.SubJetIdx1() < 0 || fj.SubJetIdx2() < 0) { ++cnt_["noSub"]; if (sdn != sdm) ++cnt_["noSubChanged"]; }
            if (!IsDATA && sdn > 0.f) {
                acc("SDMassJerUp / nominal", fj.SDMassJerUp() / sdn); acc("SDMassJerDown / nominal", fj.SDMassJerDown() / sdn);
                acc("SDMassJesUp / nominal", fj.SDMassJesUp() / sdn); acc("SDMassJesDown / nominal", fj.SDMassJesDown() / sdn);
            }
        }
        if (dump_)
            *dump_ << (IsDATA ? 1 : 0) << ',' << static_cast<unsigned int>(RunNumber) << ',' << fj.Pt() << ',' << fj.RawFactor() << ',' << fj.Area() << ','
                   << fj.Eta() << ',' << fj.Phi() << ',' << rho << ',' << corr << ',' << nom << ',' << su << ',' << sd << ',' << ju << ',' << jd << ','
                   << sdm << ',' << fj.SDMassNominal() << ',' << fj.SDMassJerUp() << ',' << fj.SDMassJerDown() << ',' << fj.SDMassJesUp() << ','
                   << fj.SDMassJesDown() << ',' << fj.SubJetIdx1() << ',' << fj.SubJetIdx2() << '\n';
    }
}
void FatJetCheck::WriteHist() {
    std::cout << "\n===== FatJetCheck  " << (IsDATA ? "DATA " + DataStream : "MC " + MCSample) << "  events " << nEv_ << "  AK8 (pt>170) " << c("fj") << "\n";
    for (auto k : {"CorrectedPt / Pt(stored)", "SmearedPtNominal / CorrectedPt", "SmearedPtUp / nominal", "SmearedPtDown / nominal",
                   "JesPtUp / nominal", "JesPtDown / nominal", "SDMassNominal / SDMass", "SDMassJerUp / nominal", "SDMassJerDown / nominal",
                   "SDMassJesUp / nominal", "SDMassJesDown / nominal"}) rep(k);
    vcheck("all lanes finite", c("nonfinite") == 0);
    if (IsDATA) vcheck("data: variation lanes equal corrected pt", c("dataVar") == 0, std::to_string(c("dataVar")));
    else {
        vcheck("MC: JES up > nominal > down for every jet", c("jesOrder") == 0, std::to_string(c("jesOrder")) + " violations");
        vcheck("MC: JER up/down move in opposite directions on average", mean("SmearedPtUp / nominal") > 1 && mean("SmearedPtDown / nominal") < 1);
        vcheck("MC: SD mass JER up/down straddle nominal", mean("SDMassJerUp / nominal") > 1 && mean("SDMassJerDown / nominal") < 1);
        vcheck("MC: SDMassNominal != SDMass (subjet JER applied)", std::fabs(mean("SDMassNominal / SDMass") - 1.) > 1e-5);
    }
    vcheck("AK8 without subjet pair keeps SD mass untouched", c("noSubChanged") == 0, std::to_string(c("noSub")) + " without pair");
    // fixed-point reference quoted in the PR
    std::cout.precision(6);
    std::cout << "  GetFJESSF(area 0.5, eta 1.2, pt 300, phi 0.3, rho 25): this sample -> "
              << myCorr->GetFJESSF(0.5f, 1.2f, 300.f, 0.3f, 25.f, IsDATA ? 386000u : 1u) << "   (PR: MC 1.127057, data run 386000 1.153181)\n";
    std::cout << "=====\n";
    if (dump_) dump_->close();
    AnalyzerCore::WriteHist();
}
