#!/usr/bin/env python3
import os
import json
import argparse
import glob
import re

parser = argparse.ArgumentParser()
parser.add_argument('--era', type=str, default='2016preVFP', help='era')
args = parser.parse_args()

#This script is used to generate the path information for the sample data
#Sample information is stored in the CommonSampleInfo.json
#This script will generate the path information for the sample data and save it to the ForSNU folder
if args.era == "2016preVFP" or args.era == "2016postVFP":
    YEAR = "2016"
    BASEPATH = os.environ["SKNANO_RUN2_NANOAODPATH"]
elif args.era == "2017":
    YEAR = "2017"
    BASEPATH = os.environ["SKNANO_RUN2_NANOAODPATH"]
elif args.era == "2018":
    YEAR = "2018"
    BASEPATH = os.environ["SKNANO_RUN2_NANOAODPATH"]
elif args.era == "2022":
    YEAR = "2022"
    BASEPATH = os.environ["SKNANO_RUN3_NANOAODPATH"]
elif args.era == "2022EE":
    YEAR = "2022"
    BASEPATH = os.environ["SKNANO_RUN3_NANOAODPATH"]
elif args.era == "2023":
    YEAR = "2023"
    BASEPATH = os.environ["SKNANO_RUN3_NANOAODPATH"]
elif args.era == "2023BPix":
    YEAR = "2023"
    BASEPATH = os.environ["SKNANO_RUN3_NANOAODPATH"]
elif args.era == "2024":
    YEAR = "2024"
    BASEPATH = os.environ["SKNANO_RUN3_NANOAODPATH"]
else:
    raise ValueError(f"Unknown era: {args.era}")

def parse_rootfiles_from(basePath):
    if not os.path.exists(basePath):
        print(f"No {basePath}")

    filePaths = []
    for root, _, files in os.walk(basePath):
        for file in files:
            # crab leaves partially staged outputs behind as .tmp_<id>_<name>;
            # they are 4% of the 2024 area and counting them would corrupt the
            # event and weight sums the effective-lumi step derives.
            if file.startswith(".tmp"):
                continue
            if file.endswith(".root"):
                filePaths.append(os.path.join(root, file))
    
    # sort the file paths by the number in the file name
    def extract_number(file_path):
        file_name = os.path.basename(file_path)
        # Remove the file extension
        file_name = os.path.splitext(file_name)[0]
        # Extract the number after the last underscore
        try:
            number = int(file_name.split('_')[-1])
            return number
        except ValueError:
            # If conversion fails, return the original string to maintain stable sorting
            return file_name

    filePaths.sort(key=extract_number)
    return filePaths

PERIOD_WITH_VERSION_RE = re.compile(r"^(?P<base>[A-Z])(?:_(?P<version>v\d+))?$")
DIR_VERSION_SUFFIX_RE = re.compile(r"_v\d+$")

def split_period(period):
    match = PERIOD_WITH_VERSION_RE.fullmatch(period)
    if match is None:
        return period, None
    return match.group("base"), match.group("version")

def filter_matches_for_period(matches, period):
    _, version = split_period(period)
    if version is None:
        return [path for path in matches if DIR_VERSION_SUFFIX_RE.search(os.path.basename(path)) is None]
    return [path for path in matches if os.path.basename(path).endswith(f"_{version}")]

def main():
    sampleInfoJson = os.path.join(os.environ['SKNANO_DATA'], args.era, 'Sample', 'CommonSampleInfo.json')
    sampleInfos = json.load(open(sampleInfoJson))
    os.makedirs(os.path.join(os.environ['SKNANO_DATA'], args.era, 'Sample', 'ForSNU'), exist_ok=True)
    for alias, sampleInfo in sampleInfos.items():
        print(f"Processing {alias}...")
        # Check if the sample is MC or data
        isMC = sampleInfo["isMC"]
        if isMC:
            basePath = os.path.join(BASEPATH, args.era, sampleInfo["PD"])
            fileJsonPath = os.path.join(os.environ['SKNANO_DATA'], args.era, 'Sample', 'ForSNU', alias+'.json')
            jsonData = {
                "name": alias,
                "isMC": 1,
                "PD": sampleInfo["PD"],
                "xsec": sampleInfo["xsec"],
                "sumsign": -1,
                "sumW": -1,
                "nmc": -1,
                "path": parse_rootfiles_from(basePath)
            }
            with open(fileJsonPath, 'w') as f:
                json.dump(jsonData, f, indent=4)
        else:
            for period in sampleInfo["periods"]:
                period_base, _ = split_period(period)
                pattern = os.path.join(BASEPATH, args.era, f"{alias}/*Run{YEAR}{period_base}*")
                matches = sorted(glob.glob(pattern))
                matches = filter_matches_for_period(matches, period)
                print(matches)

                if len(matches) == 0:
                    print(f"No path matches for {alias}_{period} (pattern: {pattern})")
                    continue
                if len(matches) > 1:
                    raise RuntimeError(f"Ambiguous paths for {alias}_{period}: {matches}")

                basePath = matches[0]
                fileJsonPath = os.path.join(
                    os.environ['SKNANO_DATA'],
                    args.era,
                    'Sample',
                    'ForSNU',
                    f"{alias}_{period}.json"
                )
                jsonData = {
                    "name": alias,
                    "isMC": 0,
                    "NEvents": [],
                    "path": parse_rootfiles_from(basePath)
                }
                with open(fileJsonPath, 'w') as f:
                    json.dump(jsonData, f, indent=4)


if __name__ == "__main__":
    main()
