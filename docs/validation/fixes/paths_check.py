import sys, glob, os, importlib.util, json
def load(path):
    spec = importlib.util.spec_from_file_location("sp_" + str(abs(hash(path))), path)
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m); return m
new = load(sys.argv[1] + "/python/sample_paths.py")
root = "/gv0/DATA/SKNano/NanoAODv15_RNTuple"
os.environ["SKNANO_INPUT_ROOT"] = root
pd = "QCD_Bin-PT-600to800_Fil-MuEnriched_TuneCP5_13p6TeV_pythia8"
pattern = f"{root}/2024/{pd}/**/*.root"
raw = [p for p in glob.glob(pattern, recursive=True) if os.path.isfile(p) and not os.path.basename(p).startswith(".")]
print(f"raw glob entries: {len(raw)}   unique files: {len(set(raw))}   duplicates: {len(raw)-len(set(raw))}")
info = {"PD": pd, "isMC": True, "name": "QCD_PT600to800_MuEnriched"}
try:
    paths = new.resolve_sample_paths(info, era="2024")
except TypeError:
    import inspect; print(inspect.signature(new.resolve_sample_paths)); raise
print(f"resolve_sample_paths (pr/fixes): {len(paths)} paths, unique {len(set(paths))}")
print("[%s] no duplicate inputs" % ("PASS" if len(paths) == len(set(paths)) == len(set(raw)) else "FAIL"))
