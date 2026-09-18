# Validation: muon momentum corrections

Environment: tamsa2, Nano15 micromamba environment (ROOT 6.40.02, correctionlib 2.6.4), corrections from `/cvmfs/cms-griddata.cern.ch/cat/metadata` (`latest` snapshots of the 2024 campaign). Inputs are 2024 NanoAODv15 files under `/gv0/DATA/SKNano/NanoAODv15_RNTuple/2024`.

**Build.** `./scripts/build.sh` and `ctest` (5/5) pass; the six `MuonScaleSmearing` gtests run and pass.

**Against the POG reference.** `MuonCheck` over 30k DYto2Mu MC and 30k Muon0 (2024D) events dumps every muon;
`muon_pog_cpp.py` recomputes each one with the POG's own `examples/MuonScaRe.cc` (`pt_scale`, `pt_resol`,
`pt_scale_var`, `pt_resol_var`), drawing the uniform number from the file's `RandomSmearing` node as the framework does.

| Sample | Muons | nominal | scale up/down | resolution up/down |
|---|---|---|---|---|
| DYto2Mu MC | 46,166 | max rel. diff 4.1e-6 | 4.1e-6 | 4.1e-6 |
| Muon0 data | 45,598 | 8.6e-8 | = nominal | = nominal |

**Lane behaviour** (same runs): outside 26 < pt < 200 GeV every lane equals the input pt (14,835 MC / 25,446 data muons);
on data the resolution lanes equal the nominal; below the TuneP boundary the high-pT lanes equal the medium-pT lanes for every muon;
in the high-pT regime `HighPtPt/TunePPt` = 1 (249 MC / 409 data muons), as expected without 2024 GE maps.

Notes: the smearing uniform comes from the correction file's `RandomSmearing` (hashprng) node rather than the example's
SeedSequence+TRandom3, and muons above 200 GeV are left to the high-pT path, as in the POG python example.

Rerun: build with `./scripts/build.sh --analysis-module-dir docs/validation/<topic>`, then `docs/validation/common/run_check.sh <checkout> <Class> <isData> <sample|stream> <period> <file> <maxevent> <tag>`; the analyzer prints each check, and `VALIDATE_DUMP` receives the per-object table the python cross-checks read.
