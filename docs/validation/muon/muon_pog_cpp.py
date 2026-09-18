# Cross-check framework muon lanes against the POG's own C++ MuonScaRe.cc.
import sys, os, csv
import numpy as np
import ROOT, correctionlib
correctionlib.register_pyroot_binding()
ex = sys.argv[1] + "/external/jsonpog-integration/examples"
path = "/cvmfs/cms-griddata.cern.ch/cat/metadata/MUO/Run3-24CDEReprocessingFGHIPrompt-Summer24-NanoAODv15/latest/muon_scalesmearing.json.gz"
ROOT.gROOT.ProcessLine(f'auto cset = correction::CorrectionSet::from_file("{path}");')
ROOT.gROOT.ProcessLine(f'#include "{ex}/MuonScaRe.cc"')
# Same as the POG pt_resol, except the uniform number comes from the file's own
# RandomSmearing (hashprng) node instead of SeedSequence+TRandom3.
ROOT.gROOT.ProcessLine('''
double pt_resol_hash(double pt, double eta, double phi, float nL, int evt, int lumi, double low_pt_threshold = 26) {
    double mean = cset->at("cb_params")->evaluate({std::abs(eta), nL, 0});
    double sigma = cset->at("cb_params")->evaluate({std::abs(eta), nL, 1});
    double n = cset->at("cb_params")->evaluate({std::abs(eta), nL, 2});
    double alpha = cset->at("cb_params")->evaluate({std::abs(eta), nL, 3});
    CrystalBall cb(mean, sigma, alpha, n);
    double rndm = cb.invcdf(cset->at("RandomSmearing")->evaluate({evt, lumi, phi}));
    double std = get_std(pt, eta, nL);
    double k = get_k(eta, "nom");
    double ptc = pt * (1 + k * std * rndm);
    if (std::isnan(ptc)) ptc = pt;
    if (ptc / pt > 2 || ptc / pt < 0.1 || ptc < 0 || pt < low_pt_threshold || pt > 200) ptc = pt;
    return ptc;
}''')
for dump in sys.argv[2:]:
    rows = list(csv.DictReader(open(dump)))
    isd = int(rows[0]["isData"])
    mism = {k: 0 for k in ("pt", "sUp", "sDn", "rUp", "rDn")}; maxrel = dict.fromkeys(mism, 0.)
    for r in rows:
        f32 = lambda k: float(np.float32(float(r[k])))  # the framework hands float inputs to correctionlib
        raw, eta, phi, q = f32("raw"), f32("eta"), f32("phi"), int(r["charge"])
        if raw > 200 or raw < 26:
            ref = {"pt": raw} if isd else {k: raw for k in mism}
        elif isd:
            ref = {"pt": ROOT.pt_scale(True, raw, eta, phi, q)}
        else:
            sc = ROOT.pt_scale(False, raw, eta, phi, q)
            evt = int(np.array(int(r["event"]) & 0xFFFFFFFF, dtype=np.uint32).astype(np.int32))
            corr = ROOT.pt_resol_hash(sc, eta, phi, float(r["nL"]), evt, int(r["lumi"]))
            ref = {"pt": corr, "sUp": ROOT.pt_scale_var(corr, eta, phi, q, "up"), "sDn": ROOT.pt_scale_var(corr, eta, phi, q, "dn"),
                   "rUp": ROOT.pt_resol_var(sc, corr, eta, "up"), "rDn": ROOT.pt_resol_var(sc, corr, eta, "dn")}
        for k, v in ref.items():
            fw = float(r[k]); rel = abs(fw - v) / max(abs(v), 1e-9)
            maxrel[k] = max(maxrel[k], rel)
            if rel > 1e-4: mism[k] += 1
    print(f"\n{os.path.basename(os.path.dirname(dump))}: {len(rows)} muons ({'DATA' if isd else 'MC'}) vs POG MuonScaRe.cc (RandomSmearing uniform, pt>200 left to high-pT path)")
    for k in (["pt"] if isd else mism):
        print(f"  {k:4s}: max rel diff {maxrel[k]:.2e}   >1e-4: {mism[k]}")
    print("  [%s] framework lanes match the POG C++ reference" % ("PASS" if not any(mism[k] for k in (['pt'] if isd else mism)) else "FAIL"))
