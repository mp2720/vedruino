#!/usr/bin/python

import configparser
import argparse
import sys

from helpers import escape_str

CONF_PATH = "./config.ini"

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
        conf_h = f"""/*
 * Файл сгенерирован автоматически с помощью tools/conf.py
 * Изменять нужно ./config.ini
 */

#pragma once

#define CONF_BAUD {conf['board']['baud']}
#define CONF_STARTUP_DELAY {int(conf['board']['startup_delay'])}

#define CONF_WIFI_SSID "{escape_str(conf['wifi']['ssid'])}"
#define CONF_WIFI_PASSWD "{escape_str(conf['wifi']['password'])}"

#define CONF_MQTT_HOST "{escape_str(conf['mqtt']['host'])}"
#define CONF_MQTT_PORT {int(conf['mqtt']['port'])}
#define CONF_MQTT_USER "{escape_str(conf['mqtt']['user'])}"
#define CONF_MQTT_PASSWD "{escape_str(conf['mqtt']['password'])}"

#define CONF_LOG_HOST "{escape_str(conf['tcp_log']['host'])}"
#define CONF_LOG_PORT "{escape_str(conf['tcp_log']['port'])}"
#define CONF_LOG_LEVEL {int(conf['tcp_log']['level'])}
#define CONF_LOG_COLOR_ENABLED {int(conf['tcp_log']['color'] == 'true')}

#define CONF_TCP_OTA_ENABLED {int(conf['tcp_ota']['enabled'] == 'true')}
#define CONF_TCP_OTA_PORT {int(conf['tcp_ota']['port'])}

#define CONF_SYSMON_ENABLED {int(conf['sysmon']['enabled'] == 'true')}
#define CONF_SYSMON_LOG_INTERVAL_MS {int(conf['sysmon']['log_interval_ms'])}
#define CONF_SYSMON_MONITOR_CPU {int(conf['sysmon']['monitor_cpu'] == 'true')}
#define CONF_SYSMON_MONITOR_HEAP {int(conf['sysmon']['monitor_heap'] == 'true')}
#define CONF_SYSMON_MONITOR_TASKS {int(conf['sysmon']['monitor_tasks'] == 'true')}
#define CONF_SYSMON_DUALCORE {int(conf['sysmon']['dualcore'] == 'true')}
"""
        with open("src/conf.h", "w+") as f:
            f.write(conf_h)
    
