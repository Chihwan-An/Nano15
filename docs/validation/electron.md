# Validation: electron energy scale and smearing, SFs and HEEP

Environment: tamsa2, Nano15 micromamba environment (ROOT 6.40.02, correctionlib 2.6.4), corrections from `/cvmfs/cms-griddata.cern.ch/cat/metadata` (`latest` snapshots of the 2024 campaign). Inputs are 2024 NanoAODv15 files under `/gv0/DATA/SKNano/NanoAODv15_RNTuple/2024`.

**Build.** `./scripts/build.sh` and `ctest` (5/5) pass.

**Against the EGM recipe.** `EleCheck` over 30k DYto2E MC and 30k EGamma0 (2024D) events dumps every electron;
`ele_egm.py` applies `egmScaleAndSmearingExample.py` with correctionlib on `electronSS_EtDependent.json.gz`.

| Sample | Electrons | Check | Max rel. diff |
|---|---|---|---|
| EGamma0 data | 27,872 | `Pt = MiniAODPt x Scale("scale", run, ScEta, r9, pt, seedGain)` | 1.1e-7 |
| DYto2E MC | 44,193 | smear up/down = raw pt x (1 + draw x smear_up/down), same draw | 2.6e-7 |
| DYto2E MC | 44,193 | scale up/down = smeared pt x scale_up/down(raw pt) | 1.1e-7 |

`electronSS_EtDependent.json.gz` is loaded and evaluated for every electron: the scale lanes above read `scale_up`/`scale_down` from it (ScaleUpPt/Pt = 1.00135 on average).
The smearing pull recovered from `Pt/MiniAODPt` has mean -0.0035 and rms 0.9998 (N(0,1)). Data variation lanes equal `Pt()`.

**Scale factors.** For every electron (37,473 MC, 22,026 data) `GetElectronIDSF("Tight", electron)` and `GetElectronRECOSF(electron)`
equal the evaluation at `MiniAODPt()`. Evaluated at the corrected pt instead, 3.6% of the MC SFs would change bin and 41 electrons
would fall below the map's 10 GeV edge.

**Other.** `GetAllElectronViews(true)` contains no electron with 1.4442 < |ScEta| < 1.566. HEEP SF at (ScEta 0.5, pt 120):
0.97958 (up 0.98333, down 0.97582); pt below 35 GeV returns the 35 GeV value; signed ScEta: -2.0 -> 0.928, +2.0 -> 0.975.

Rerun: build with `./scripts/build.sh --analysis-module-dir docs/validation/<topic>`, then `docs/validation/common/run_check.sh <checkout> <Class> <isData> <sample|stream> <period> <file> <maxevent> <tag>`; the analyzer prints each check, and `VALIDATE_DUMP` receives the per-object table the python cross-checks read.
