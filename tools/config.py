#!/usr/bin/python

import configparser
import argparse
import typing
import sys

DEFAULT_CONF_PATH = "./config_default.ini"


class Colors:
    error = '\033[91m'
    enabled = '\033[92m'
    disabled = '\033[93m'
    reset = '\033[0m'


def error(msg: str) -> typing.NoReturn:
    print(Colors.error + msg + Colors.reset, file=sys.stderr)
    exit(1)


ap = argparse.ArgumentParser(description='Config tool')
ap.add_argument(
    'path',
    help="path to config file"
)
ap.add_argument(
        '-c',
        '--gen-header',
        action='store_true',
        help='generate src/conf.h'
)
ap.add_argument(
        '-g',
        '--get-value',
        dest='config_key',
        action='store',
        help='get value from config'
)
args = ap.parse_args()

if bool(args.config_key) == args.gen_header:
    error('provide exactly one flag (run with -h for help)')

conf = configparser.ConfigParser()
if not conf.read([DEFAULT_CONF_PATH, args.path]):
    error(f"error opening {DEFAULT_CONF_PATH}")


def get_param_or_fail(section: str, key: str) -> str:
    try:
        return conf[section][key]
    except KeyError:
        error(f"[{section}]:{key} not found")


if args.config_key:
    toks = args.config_key.split(':')
    if len(toks) != 2:
        error("key must look like 'section:key' or 'key'")

    value = get_param_or_fail(*toks)

    print(value)


def escape_str(s: str) -> str:
    s = s.replace('"', '\\"')
    s = s.replace('\n', '\\n')
    return s


def add_def(section: str, key: str, value = None, type_: str | None = None):
    global conf_h

    if value is None:
        assert type_ is not None
        value = get_param_or_fail(section, key)
        if type_ == "int":
            try:
                value = int(value)
            except ValueError:
                raise ValueError(f"{section}:{key} expected to be int")
        elif type_ == 'str':
            pass
        elif type_ == 'bool':
            try:
                value = {'true': True, 'false': False}[value]
            except IndexError:
                raise ValueError("f{section}:{key} expected to be true or false")
        else:
            assert False, f"unknown type {type_}"

    if type(value) is int:
        value = value
    elif type(value) is str:
        value = f'"{escape_str(value)}"'
    elif type(value) is bool:
        value = int(value)
    else:
        assert False, f"type {type(value)} is not supported"

    section = section.replace('.', '_')
    key = key.replace('.', '_')

    conf_h += f"#define CONF_{section.upper()}_{key.upper()} {value}\n"


def is_module_enabled(section: str):
    return section in conf and get_param_or_fail(section, 'enabled') == 'true'


def add_section_comm(section: str):
    global conf_h
    conf_h += f"\n// [{section}]\n\n"


def add_module_start(section: str) -> bool:
    add_section_comm(section)
    enabled = is_module_enabled(section)
    if not enabled:
        print(f"{section} {Colors.disabled}DISABLED{Colors.reset}")
        add_def(section, "enabled", value=False)
        return False
    else:
        print(f"{section} {Colors.enabled}ENABLED{Colors.reset}")
        add_def(section, "enabled", type_="bool")
        return True


def add_module(section: str, key_types: dict[str, str]):
    enabled = add_module_start(section)
    for k, t in key_types.items():
        if enabled:
            add_def(section, k, type_=t)
        else:
            default_value = {'int': 0, 'bool': 0, 'str': ""}[t]
            add_def(section, k, value=default_value)


if args.gen_header:
    conf_h = "// Файл сгенерирован автоматически с помощью tools/conf.py\n"
    conf_h += "// Изменять нужно ./config.ini\n"
    conf_h += "#pragma once\n"
    
    try:
        add_section_comm("board")
        add_def("board", "startup_delay", type_='int')

        add_section_comm("log")
        add_def("log", "lib_level", type_='int')
        add_def("log", "app_level", type_='int')
        add_def("log", "print_file_line", type_='bool')
        add_def("log", "print_time", type_='bool')
        add_def("log", "print_color", type_='bool')

        add_module('mdns', {
            'hostname': 'str'
        })

        add_module('ip', {})

        add_module("wifi", {
            'ssid': 'str',
            'password': 'str',
            'retry': 'int'
        })

        add_module("netlog", {
            'max_clients': 'int',
            'buf_size': 'int',
        })

        add_module("mqtt", {
            'host': 'str',
            'port': 'int',
            'user': 'str',
            'password': 'str'
        })

        add_module("ota", {
            'port': 'int',
        })

        add_module("sysmon", {
            'enable_log': 'bool',
            'log_interval_ms': 'int',
            'monitor_cpu': 'bool',
            'monitor_heap': 'bool',
            'monitor_tasks': 'bool',
            'dualcore': 'bool',
        })

        add_module('json', {})

        deps = {
            'ip': ['wifi'],
            'netlog': ['ip'],
            'mqtt': ['wifi'],
            'ota': ['ip']
        }

        for m, dep_list in deps.items():
            if not is_module_enabled(m):
                continue

            for d in dep_list:
                if not is_module_enabled(d):
                    error(f"module {d} is required for module {m}")
    except ValueError as e:
        error(repr(e))

    open('src/conf.h', 'w+').write(conf_h)

