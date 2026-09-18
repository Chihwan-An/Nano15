import sys, subprocess, os, ROOT
a, b, out = sys.argv[1], sys.argv[2], sys.argv[3]
def keys(p):
    f = ROOT.TFile.Open(p); ks = sorted((k.GetName(), k.GetClassName()) for k in f.GetListOfKeys()); f.Close(); return ks
def entries(p, name):
    return ROOT.RNTupleReader.Open(name, p).GetNEntries()
print("input A keys:", keys(a)); print("input B keys:", keys(b))
na, nb = entries(a, "Central_SR_Tree"), entries(b, "Central_SR_Tree")
ok = True
for tool, cmd in (("sknano_merge.py", ["python3", os.environ["SKNANO_HOME"] + "/scripts/sknano_merge.py", "--output", out + ".sknano.root", a, b]),
                  ("hadd", ["hadd", "-f", out + ".hadd.root", a, b])):
    r = subprocess.run(cmd, capture_output=True, text=True)
    p = out + (".sknano.root" if tool != "hadd" else ".hadd.root")
    if r.returncode: print(tool, "rc", r.returncode, r.stderr[-400:]); ok = False; continue
    n = entries(p, "Central_SR_Tree")
    f = ROOT.TFile.Open(p); h = f.Get("Central/nEvents"); hsum = h.GetEntries() if h else -1; empty = bool(f.GetKey("Central_Empty_Tree")); f.Close()
    good = (n == na + nb) and not empty and hsum == na + nb
    ok &= good
    print(f"  [{'PASS' if good else 'FAIL'}] {tool}: merged Central_SR_Tree entries {n} (= {na}+{nb}), histogram entries {hsum:.0f}, never-filled RNTuple written: {empty}")
print("[%s] RNTuple outputs merge and read back" % ("PASS" if ok else "FAIL"))
