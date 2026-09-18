import ast, os, sys
src = open(sys.argv[1] + "/scripts/makeSamplePathInfo.py").read()
tree = ast.parse(src)
fn = [n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == "parse_rootfiles_from"][0]
ns = {"os": os}; exec(compile(ast.Module([fn], []), "f", "exec"), ns)
root = "/gv0/DATA/SKNano/NanoAODv15_RNTuple/2024"
found = None
for pd in sorted(os.listdir(root)):
    d = os.path.join(root, pd)
    for r, _, fs in os.walk(d, followlinks=True):
        if any(f.startswith(".tmp") and f.endswith(".root") for f in fs):
            found = d; break
    if found: break
print("dataset with .tmp files:", found)
got = ns["parse_rootfiles_from"](found)
ondisk = sum(1 for r,_,fs in os.walk(found, followlinks=True) for f in fs if f.startswith(".tmp") and f.endswith(".root"))
bad = [p for p in got if os.path.basename(p).startswith(".tmp")]
print(f".tmp on disk {ondisk}, returned {len(got)} files, .tmp among them {len(bad)}")
print("[%s] .tmp partial outputs skipped" % ("PASS" if ondisk and not bad else "FAIL"))
