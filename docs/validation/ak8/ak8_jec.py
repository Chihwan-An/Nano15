# Independent check: recompute AK8 JEC (compound L1L2L3Res) and JES Total with correctionlib
import sys, csv, os, numpy as np, correctionlib
f = "/cvmfs/cms-griddata.cern.ch/cat/metadata/JME/Run3-24CDEReprocessingFGHIPrompt-Summer24-NanoAODv15/latest/fatJet_jerc.json.gz"
cs = correctionlib.CorrectionSet.from_file(f)
for dump in sys.argv[1:]:
    rows = list(csv.DictReader(open(dump)))
    c = lambda k: np.array([np.float32(float(r[k])) for r in rows], dtype=np.float64)
    isd = int(rows[0]["isData"])
    tag = "Summer24Prompt24_V5_%s_L1L2L3Res_AK8PFPuppi" % ("DATA" if isd else "MC")
    jec = cs.compound[tag]
    rawpt = np.float32(c("pt") * (1 - c("rawFactor")))
    vals = {"JetA": c("area"), "JetEta": c("eta"), "JetPt": rawpt, "Rho": c("rho"), "JetPhi": c("phi"), "run": c("run")}
    args = [vals[i.name] for i in jec.inputs]
    ref = rawpt * jec.evaluate(*args)
    rel = np.abs(c("corr") - ref) / ref
    print(f"\n{os.path.basename(os.path.dirname(dump))}: {len(rows)} AK8 jets, {tag} inputs {[i.name for i in jec.inputs]}")
    print(f"  CorrectedPt vs raw*L1L2L3Res: max rel {rel.max():.1e}  >1e-5: {(rel > 1e-5).sum()}")
    ok = (rel > 1e-5).sum() == 0
    if not isd:
        unc = cs["Summer24Prompt24_V5_MC_Total_AK8PFPuppi"].evaluate(c("eta"), c("corr"))
        r_up = np.abs(c("jesUp") / c("smNom") - 1 - unc); r_dn = np.abs(1 - c("jesDn") / c("smNom") - unc)
        print(f"  JesPtUp/Down vs nominal*(1 +- Total(eta, CorrectedPt)): max |diff| {max(r_up.max(), r_dn.max()):.1e}  >1e-5: {(r_up > 1e-5).sum() + (r_dn > 1e-5).sum()}")
        ok &= (r_up > 1e-5).sum() + (r_dn > 1e-5).sum() == 0
    print("  [%s] framework matches correctionlib" % ("PASS" if ok else "FAIL"))
