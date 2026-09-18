# Validation: batch environment, RNTuple output, merge, snapshot

Environment: tamsa2, Nano15 micromamba environment (ROOT 6.40.02, correctionlib 2.6.4), corrections from `/cvmfs/cms-griddata.cern.ch/cat/metadata` (`latest` snapshots of the 2024 campaign). Inputs are 2024 NanoAODv15 files under `/gv0/DATA/SKNano/NanoAODv15_RNTuple/2024`.

**Build.** `./scripts/build.sh` and `ctest` (5/5, including `merge_engine`) pass on this branch alone.

| Change | How | Result |
|---|---|---|
| RNTuples hadd can merge | `InfraCheck` books `Central/SR_Tree` (filled) and `Central/Empty_Tree` (never filled), run on two inputs (3,000 + 2,000 events); `infra_merge.py` merges them | Outputs hold `Central_SR_Tree` at the top level and no empty RNTuple. Both `sknano_merge.py` and `hadd` produce 5,000 entries and the histogram sums to 5,000. |
| Merge memory/time | `mkshards.py`: 4 shards x 60,000 TH1D; `sknano_merge.py` before (main) and after, under `/usr/bin/time` | 629 s / 876 MB -> 37.6 s / 779 MB; merged contents identical. |
| Batch environment | `SKNano.py -a ExampleRun -i DYto2Mu_MLL50to120 -e 2024 -n 1 --no_exec`, then the generated `hadd.sh` activation under `env -i` with a wrong `ROOTSYS`/`LD_LIBRARY_PATH` | `run.sh`/`hadd.sh` activate Nano15 by name, then by prefix; the activation lands in `envs/Nano15` with ROOT 6.40.02. |
| Source snapshot | Same submission with a git module holding a tracked file, an untracked analyzer, a `.gitignore`d 50 MB output and a `.tsv` | Snapshot holds the 4 source files; the ignored output is absent; the `.tsv` is listed as skipped; manifest `selection: git`. |

Rerun: build with `./scripts/build.sh --analysis-module-dir docs/validation/<topic>`, then `docs/validation/common/run_check.sh <checkout> <Class> <isData> <sample|stream> <period> <file> <maxevent> <tag>`; the analyzer prints each check, and `VALIDATE_DUMP` receives the per-object table the python cross-checks read.
