# Validation: silent-failure fixes

Environment: tamsa2, Nano15 micromamba environment (ROOT 6.40.02, correctionlib 2.6.4), corrections from `/cvmfs/cms-griddata.cern.ch/cat/metadata` (`latest` snapshots of the 2024 campaign). Inputs are 2024 NanoAODv15 files under `/gv0/DATA/SKNano/NanoAODv15_RNTuple/2024`.

**Build.** `./scripts/build.sh` and `ctest` (5/5) pass on this branch alone.

| Fix | How | Result |
|---|---|---|
| AK4 JES lanes | `FixCheck` over 20k TTto2L2Nu MC and 20k Muon0 (2024D) events, jets with pt > 15 | MC: `JesPtUp/SmearedPtNominal` = 1.0312, `JesPtDown` = 0.9688 over 114,578 jets, none zero. Data: every lane equals the nominal over 51,451 jets. |
| LHEScaleWeight slot | `GetScaleVariation` for the four single-scale shifts on 20k TTto2L2Nu events | All events carry 9 weights; slot 4 is 1 in every event and muF-up reads slot 5 in every event. muF up/down 0.980 / 1.026, muR up/down 0.896 / 1.114. |
| Submodule URL | `git ls-remote` on the recorded URL | Reachable; the pinned commit `ea5e2a9` is the upstream master. |
| Sample inputs | `paths_check.py`, `tmp_check.py` | `resolve_sample_paths` returns a duplicate-free list (764 files for QCD PT-600to800 MuEnriched). `parse_rootfiles_from` on `2024/DATA` returns 10,200 files and none of the 26 crab `.tmp_*` partials. |

Rerun: build with `./scripts/build.sh --analysis-module-dir docs/validation/<topic>`, then `docs/validation/common/run_check.sh <checkout> <Class> <isData> <sample|stream> <period> <file> <maxevent> <tag>`; the analyzer prints each check, and `VALIDATE_DUMP` receives the per-object table the python cross-checks read.
