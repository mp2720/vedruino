#!/usr/bin/python

import configparser
import argparse
import sys

from helpers import escape_str

CONF_PATH = "./conf/config.ini"

conf = configparser.ConfigParser()
if not conf.read(CONF_PATH):
    print(f"error opening {CONF_PATH}", file=sys.stderr)
    sys.exit(1)


def getparam(section: str, key: str) -> str:
    return conf[section][key]


if __name__ == "__main__":
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
    
    
    if args.config_key:
        toks = args.config_key.split(':')
        if len(toks) > 2:
            print("key must look like 'section:key' or 'key'")
            sys.exit(1)
    
        if len(toks) == 2:
            value = conf[toks[0]][toks[1]]
        else:
            value = conf[toks[0]]
    
        print(value)
    
    if args.gen_cpp:
        with open(conf['ota']['cert_path']) as f:
            cert = f.read()
            
        conf_h = f"""/*
 * Файл сгенерирован автоматически с помощью tools/conf.py
 * Изменять нужно conf/config.ini
 */

#pragma once

#define CONF_UTC_OFFSET {conf['common']['utc_offset']}

#define CONF_BAUD {conf['arduino']['baud']}

#define CONF_WIFI_SSID "{escape_str(conf['wifi']['ssid'])}"
#define CONF_WIFI_PASSWD "{escape_str(conf['wifi']['password'])}"

#define CONF_MQTT_HOST "{escape_str(conf['mqtt']['host'])}"
#define CONF_MQTT_USER "{escape_str(conf['mqtt']['user'])}"
#define CONF_MQTT_PASSWD "{escape_str(conf['mqtt']['password'])}"

#define CONF_OTA_ENABLED {int(conf['ota']['enabled'] == 'true')}
#define CONF_OTA_HOST "{escape_str(conf['ota']['host'])}"
#define CONF_OTA_TOKEN "{escape_str(conf['ota.board']['token'])}"
#define CONF_OTA_REPO "{escape_str(conf['common']['repo_name'])}"
#define CONF_OTA_CERT "{escape_str(cert)}"
#define CONF_OTA_RETRIES {int(conf['ota.board']['retries'])}
"""
        with open("src/conf.h", "w+") as f:
            f.write(conf_h)
    
