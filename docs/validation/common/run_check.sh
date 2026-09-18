#!/bin/bash
# run_check.sh <worktree> <Class> <isData 0|1> <sample|stream> <period> <file> <maxevent> <tag>
wt=$1; cls=$2; isdata=$3; smp=$4; per=$5; f=$6; nmax=$7; tag=$8
R=${VALIDATE_OUT:-$PWD/validation_runs}
cd $wt && source setup.sh > /dev/null 2>&1
export ROOT_INCLUDE_PATH="${SKNANO_INSTALLDIR}/include${CONDA_PREFIX:+:${CONDA_PREFIX}/include}${ROOT_INCLUDE_PATH:+:${ROOT_INCLUDE_PATH}}"
out=$R/$tag; mkdir -p $out; cd $out
export VALIDATE_DUMP=$out/dump.csv
if [ "$isdata" = 1 ]; then who="module.IsDATA = true; module.DataStream = \"$smp\";"; else who="module.IsDATA = false; module.MCSample = \"$smp\"; module.xsec = 1; module.sumW = 1; module.sumSign = 1;"; fi
cat > run_$tag.C <<EOC
void run_$tag() {
    $cls module;
    module.SetRNTupleName("Events");
    module.SetAnalyzerName("$cls");
    module.SetOutputThreads(1);
    module.LogEvery = 100000;
    $who
    module.SetEra("2024");
    module.SetPeriod("$per");
    module.AddFile("$f");
    module.MaxEvent = $nmax;
    module.SetOutfilePath("$out/out.root");
    module.SetFailurePolicy("fail-fast");
    module.SetMaxEventErrors(1);
    module.Init();
    module.initializeAnalyzer();
    module.Loop();
    module.WriteHist();
}
EOC
root -l -b -q run_$tag.C > log.txt 2>&1; rc=$?
echo "rc=$rc"; sed -n '/^=====/,/^=====$/p' log.txt
[ $rc -ne 0 ] && tail -30 log.txt
exit $rc
