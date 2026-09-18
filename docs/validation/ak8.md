# Validation: AK8 JERC

Environment: tamsa2, Nano15 micromamba environment (ROOT 6.40.02, correctionlib 2.6.4), corrections from `/cvmfs/cms-griddata.cern.ch/cat/metadata` (`latest` snapshots of the 2024 campaign). Inputs are 2024 NanoAODv15 files under `/gv0/DATA/SKNano/NanoAODv15_RNTuple/2024`.

**Build.** `./scripts/build.sh` and `ctest` (5/5) pass.

**Against correctionlib.** `FatJetCheck` over 40k TTto2L2Nu MC and 100k Muon0 (2024D) events dumps every AK8 jet with pt > 170;
`ak8_jec.py` recomputes it from `fatJet_jerc.json.gz`.

| Sample | Jets | Check | Max rel. diff |
|---|---|---|---|
| TTto2L2Nu MC | 10,320 | `CorrectedPt` = raw pt x `Summer24Prompt24_V5_MC_L1L2L3Res_AK8PFPuppi` (inputs filled by name) | 1.1e-7 |
| Muon0 data | 11,399 | same with the DATA compound (adds `run`) | 1.1e-7 |
| TTto2L2Nu MC | 10,320 | `JesPtUp/Down` = nominal x (1 +- `Total`(eta, CorrectedPt)) | 1.1e-7 |

`GetFJESSF(0.5, 1.2, 300, 0.3, 25)` returns 1.127057 (MC) and 1.153181 (data, run 386000).

**Lane behaviour (MC).** JES up > nominal > down for every jet; JER up/down 1.0018 / 0.9982; `SDMassNominal/SDMass` = 1.0023
(subjet JER applied); SD mass JER up/down 1.0008 / 0.9993, JES up/down 1.0157 / 0.9843. On data every variation lane equals
`CorrectedPt`. Jets without a subjet pair keep the stored SD mass.

Rerun: build with `./scripts/build.sh --analysis-module-dir docs/validation/<topic>`, then `docs/validation/common/run_check.sh <checkout> <Class> <isData> <sample|stream> <period> <file> <maxevent> <tag>`; the analyzer prints each check, and `VALIDATE_DUMP` receives the per-object table the python cross-checks read.
