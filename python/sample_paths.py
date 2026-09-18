"""Resolve the input ROOT files of a sample.

A sample's files live under a single production root, which is site-local and
therefore an environment variable rather than repository content:

    $SKNANO_INPUT_ROOT/<era>/<dataset>/.../*.root

Only the part below that root is sample metadata, and for almost every sample
it is already implied by fields the JSON carries anyway -- ``PD`` for MC, the
data-taking period for DATA. So the common case needs no path field at all:

    {"name": "TTto2L2Nu", "isMC": 1, "PD": "TTto2L2Nu_TuneCP5_...", "xsec": 762.1}

Two escape hatches remain, in precedence order:

``path_glob``
    A glob relative to the input root, or absolute if it starts with "/".
    Use it when a production does not follow the layout -- a private
    reprocessing, a directory that has to be pinned to one CRAB submission.

``path``
    An explicit list of absolute files. Reproducible but unportable: it names
    one mount on one cluster and goes stale as soon as a production is
    extended. Kept for samples that genuinely need a hand-picked file list.
"""

from __future__ import annotations

import glob
import os
import re
from typing import Any, Mapping, Optional

INPUT_ROOT_ENV = "SKNANO_INPUT_ROOT"

# "C" -> run period C, "I_v2" -> run period I, reprocessing v2.  The version is
# a suffix on the CRAB directory rather than part of the run name, so it cannot
# go into the glob and is applied as a filter afterwards.
_PERIOD_RE = re.compile(r"([A-Z])(?:_(v\d+))?$")


def input_root() -> str:
    root = os.environ.get(INPUT_ROOT_ENV)
    if not root:
        raise ValueError(
            f"{INPUT_ROOT_ENV} is not set; source setup.sh, or set it in "
            "config/config.$USER, so sample metadata does not have to carry "
            "absolute paths")
    return root.rstrip("/")


def sample_glob(sample_info: Mapping[str, Any], era: str,
                period: Optional[str] = None) -> str:
    """Return the glob, relative to the input root, for one sample."""
    pattern = sample_info.get("path_glob")
    if pattern:
        if not isinstance(pattern, str):
            raise ValueError("sample 'path_glob' must be a string")
        return pattern

    # MC is one directory per dataset; DATA is one directory per primary
    # dataset with the period in the CRAB directory below it, so a data sample
    # cannot be resolved without knowing which period is being asked for.
    dataset = sample_info.get("PD") or sample_info.get("name")
    if not dataset:
        raise ValueError("sample needs 'PD' or 'name' to derive its inputs")

    if sample_info.get("isMC"):
        return f"{era}/{dataset}/**/*.root"
    if not period:
        raise ValueError(
            f"data sample '{dataset}' needs a period to select its CRAB "
            "directory; pass period= or give the sample a 'path_glob'")
    match = _PERIOD_RE.match(period)
    if not match:
        raise ValueError(f"unrecognised data period '{period}'")
    return f"{era}/{dataset}/*_Run{era}{match.group(1)}_*/**/*.root"


def _keeps_period_version(path: str, dataset: str, period: str) -> bool:
    """Reject a reprocessing that the period did not ask for.

    ``*_Run2024I_*`` matches both ``..._MINIv6NANOv15`` and its ``_v2``
    reprocessing, and glob cannot express "not the v2 one". Compare the CRAB
    directory -- the component directly below the dataset -- instead.
    """
    version = _PERIOD_RE.match(period).group(2)
    parts = path.split("/")
    try:
        submission = parts[parts.index(dataset) + 1]
    except (ValueError, IndexError):
        return True
    if version:
        return submission.endswith(f"_{version}")
    return re.search(r"_v\d+$", submission) is None


def resolve_sample_paths(sample_info: Mapping[str, Any],
                         era: Optional[str] = None,
                         period: Optional[str] = None) -> list[str]:
    """Return the input ROOT files for a sample, sorted."""
    if "path" in sample_info:
        paths = sample_info["path"]
        if not isinstance(paths, list) or not all(
                isinstance(path, str) for path in paths):
            raise ValueError("sample 'path' must be a list of strings")
        return list(paths)

    if era is None:
        raise ValueError(
            "resolving a sample without an explicit 'path' needs its era")

    pattern = sample_glob(sample_info, era, period)
    if not os.path.isabs(pattern):
        pattern = os.path.join(input_root(), pattern)

    # A set, not just a sort: the storage backing some 2024 datasets
    # (QCD_Bin-PT-600to800_Fil-MuEnriched) returns the same directory
    # entry more than once, so glob yields each file up to eight times.
    # Handing the duplicates to a job multiplies that sample's event and
    # weight sums, which nothing downstream would flag.
    paths = sorted({
        path for path in glob.glob(pattern, recursive=True)
        if os.path.isfile(path) and path.endswith(".root")
        # A production being written into right now leaves partial files
        # behind; feeding one to a job silently truncates the sample. glob
        # already skips leading dots, this is the belt to that pair of braces.
        and not os.path.basename(path).startswith(".")
    })
    if not sample_info.get("isMC") and not sample_info.get("path_glob"):
        dataset = sample_info.get("PD") or sample_info.get("name")
        paths = [p for p in paths
                 if _keeps_period_version(p, dataset, period)]
    if not paths:
        raise ValueError(f"no ROOT files matched: {pattern}")
    return paths
