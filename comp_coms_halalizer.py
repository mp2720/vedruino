import json

COMP_COMS_PATH = "./compile_commands.json"
FLAGS_WHITELIST = (
        "-I",
        "-D",
        "-o",
        "-Wa",
        "-Wdeprecated",
        "-Wp"
        )
EXTRA_ARGS = (
        "-Wall",
        "-Wextra",
        "-std=c++11"
        )

with open(COMP_COMS_PATH) as f:
    dic = json.load(f)


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

with open(COMP_COMS_PATH, "w") as f:
    json.dump(dic, f, indent=4)

