# Independent check: apply the EGM example recipe (egmScaleAndSmearingExample.py) with correctionlib
import sys, csv, os, numpy as np, correctionlib
f = "/cvmfs/cms-griddata.cern.ch/cat/metadata/EGM/Run3-24CDEReprocessingFGHIPrompt-Summer24-NanoAODv15/latest/electronSS_EtDependent.json.gz"
cs = correctionlib.CorrectionSet.from_file(f); scale = cs.compound["Scale"]; ss = cs["SmearAndSyst"]
for dump in sys.argv[1:]:
    rows = list(csv.DictReader(open(dump)))
    c = lambda k: np.array([np.float32(float(r[k])) for r in rows], dtype=np.float64)
    isd = int(rows[0]["isData"]); raw, sc, r9, g, run = c("raw"), c("scEta"), c("r9"), c("gain"), c("run")
    fw = {k: c(k) for k in ("pt", "sUp", "sDn", "smUp", "smDn")}
    rel = lambda a, b: np.abs(a - b) / np.abs(b)
    print(f"\n{os.path.basename(os.path.dirname(dump))}: {len(rows)} electrons ({'DATA' if isd else 'MC'}) vs EGM recipe")
    res = {}
    if isd:
        res["pt = raw*Scale('scale',run,ScEta,r9,pt,gain)"] = rel(fw["pt"], raw * scale.evaluate("scale", run, sc, r9, raw, g))
    else:
        w = ss.evaluate("smear", raw, r9, sc); wu = ss.evaluate("smear_up", raw, r9, sc); wd = ss.evaluate("smear_down", raw, r9, sc)
        ok = w > 0
        draw = (fw["pt"] / raw - 1) / np.where(ok, w, 1)
        print(f"  draw: mean {draw[ok].mean():+.4f} rms {draw[ok].std():.4f} (N(0,1) expected)")
        res["smear_up   = raw*(1+draw*smear_up)"] = rel(fw["smUp"], raw * (1 + draw * wu))[ok]
        res["smear_down = raw*(1+draw*smear_down)"] = rel(fw["smDn"], raw * (1 + draw * wd))[ok]
        res["scale_up   = smeared*scale_up(raw)"] = rel(fw["sUp"], fw["pt"] * ss.evaluate("scale_up", raw, r9, sc))
        res["scale_down = smeared*scale_down(raw)"] = rel(fw["sDn"], fw["pt"] * ss.evaluate("scale_down", raw, r9, sc))
    allok = True
    for k, v in res.items():
        bad = int((v > 1e-5).sum()); allok &= bad == 0
        print(f"  {k:45s} max rel {v.max():.1e}  >1e-5: {bad}")
    print("  [%s] framework matches the EGM recipe" % ("PASS" if allok else "FAIL"))
