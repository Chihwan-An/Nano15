import ROOT, sys, os
ROOT.TH1.AddDirectory(False)
d = sys.argv[1]; n = int(sys.argv[2]); nh = int(sys.argv[3]); os.makedirs(d, exist_ok=True)
for s in range(n):
    f = ROOT.TFile.Open(f"{d}/hists_{s}.root", "RECREATE", "", 404)
    for sub in range(10):
        dd = f.mkdir(f"Syst{sub}"); dd.cd()
        for i in range(nh // 10):
            h = ROOT.TH1D(f"h{i}", "", 20, 0, 1); h.Fill(0.5, s + 1); h.Write()
    f.Close()
print("made", n, "shards x", nh, "histograms")
