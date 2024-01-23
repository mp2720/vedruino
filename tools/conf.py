#!/usr/bin/python

import configparser
import argparse
import sys

CONF_PATH = "./conf/config.ini"

ap = argparse.ArgumentParser(description='Config tool')
ap.add_argument(
        '-c',
        '--gen-cpp',
        action='store_true',
        help='generate src/conf.cpp and src/conf.h'
)

ap.add_argument(
        '-g',
        '--get-value',
        dest='config_key',
        action='store',
        help='get value from config.ini'
)

args = ap.parse_args()

if bool(args.config_key) == args.gen_cpp:
    ap.error('provide exactly one flag (run with -h for help)')

conf = configparser.ConfigParser()
if not conf.read(CONF_PATH):
    print(f"error opening {CONF_PATH}", file=sys.stderr)
    sys.exit(1)

if args.config_key:
    toks = args.config_key.split('.')
    if len(toks) > 2:
        print("only keys with one or zero dots are supported")
        sys.exit(1)

    if len(toks) == 2:
        value = conf[toks[0]][toks[1]]
    else:
        value = conf[toks[0]]

    print(value)


def escape_str(s: str) -> str:
    s = s.replace('"', '\\"')
    s = s.replace('\n', '\\n"')
    return s


if args.gen_cpp:
    conf_h = f"""/*
 * Файл сгенерирован автоматически с помощью tools/conf.py
 * Изменять нужно conf/config.ini
 */

#pragma once

#define BAUD {conf['arduino']['baud']}

#define WIFI_SSID "{escape_str(conf['wifi']['ssid'])}"
#define WIFI_PASSWD "{escape_str(conf['wifi']['password'])}"

#define MQQT_HOST "{escape_str(conf['mqqt']['host'])}"
#define MQQT_USER "{escape_str(conf['mqqt']['user'])}"
#define MQQT_PASSWD "{escape_str(conf['mqqt']['password'])}"

#define OTA_HOST "{escape_str(conf['ota']['host'])}"
#define OTA_TOKEN "{escape_str(conf['ota']['board_token'])}"
#define OTA_TAG "{escape_str(conf['ota']['board_tag'])}"
#define OTA_REPO "{escape_str(conf['common']['repo_name'])}"
"""
    with open("src/conf.h", "w+") as f:
        f.write(conf_h)

