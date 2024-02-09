#!/usr/bin/python

import json
import sys

FLAGS_WHITELIST = (
        "-I",
        "-D",
        "-o",
        "-Wa",
        "-Wdeprecated",
        "-Wp",
        )
EXTRA_ARGS = (
        "-Wall",
        "-Wextra",
        # "-std=c++11",
        "-DSSIZE_MAX=2147483647",
        "-m32"
        )

dic = json.load(sys.stdin)


def args_filter(arg: str) -> bool:
    if not arg.startswith("-"):
        return True

    # Если параметр начинается с разрешённого префикса, то его можно оставить.
    return any(map(lambda x: arg.startswith(x), FLAGS_WHITELIST))


for entry in dic:
    entry["arguments"] = list(filter(args_filter, entry["arguments"]))
    for arg in EXTRA_ARGS:
        if arg not in entry["arguments"]:
            entry["arguments"].append(arg)

json.dump(dic, sys.stdout, indent=4)

