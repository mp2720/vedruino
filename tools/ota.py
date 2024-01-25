#!/usr/bin/python

import argparse
import requests
import sys
import config
import urllib3

from datetime import datetime
from dateutil.tz import tzutc, tzlocal
from helpers import escape_str

urllib3.disable_warnings(urllib3.exceptions.SubjectAltNameWarning)

def ts2str(ts: int) -> str:
    utc = datetime.utcfromtimestamp(ts)
    utc = utc.replace(tzinfo=tzutc())
    return utc.astimezone(tzlocal()).strftime('%d-%m-%Y %H:%M:%S')


def print_firmware_info(fi: dict):
    boards = ','.join(fi['boards'])
    print(
        f"id: {fi['id']}\n"
        f"uuid: {fi['uuid']}\n"
        f"repo name: {fi['repo_name']}\n"
        f"description: {fi['description']}\n"
        f"commit id: {fi['commit_id']}\n"
        f"boards: {boards}\n"
        f"created at: {ts2str(fi['created_at'])}\n"
        f"created by: {fi['created_by']}\n"
        f"md5: {fi['md5']}\n"
        f"size: {fi['size']}"
    )


def check_http_resp(resp: requests.Response):
    if not resp.ok:
        print(f"{resp.request.method} {resp.url} failed with code {resp.status_code}",
              file=sys.stderr)
        print(f"response: {resp.text}")
        exit(1)


def print_user_info(user: dict):
    print(
        f"name: {user['name']}\n"
        f"type: {'board' if user['is_board'] else 'dev'}"
    )


ap = argparse.ArgumentParser(
    description='OTA dev client. Obtains some options from conf/config.ini',
    epilog="commands:\n"
    "\tlist - list all firmwares on server\n"
    "\tlast - get info about last firmware for board\n"
    "\tnew - create new firmware record on server and generate src/ota_ver.h, then returns firmware's uuid\n"
    "\tpub - publish firmware to server (use after running 'new' and build)\n"
    "\tuser - info about dev user\n"
    "\tboard - info about board user",
    formatter_class=argparse.RawTextHelpFormatter
)
ap.add_argument(
    'command', choices=[
        'list',
        'last',
        'new', 
        'pub',
        'user',
        'board',
    ], help="command"
)
ap.add_argument(
    '-d',
    '--description',
    dest='description',
    action='store',
    help='set description field for firmware (only with pub command)'
)
ap.add_argument(
    '-r',
    '--repo',
    dest="repo",
    action='store',
    help='set repo (only with last command)'
)
ap.add_argument(
    '-i',
    '--uuid',
    dest="uuid",
    action='store',
    help="set firmware's uuid (only with pub command)"
)
ap.add_argument(
    '-b',
    '--boards',
    dest="boards",
    action='store',
    help='set boards list (only with new command)'
)
args = ap.parse_args()

if args.command != "new" and args.description is not None:
    ap.error("cannot use -d/--description without new command, run with -h for help")
if args.command != "last" and args.repo is not None:
    ap.error("cannot use -r/--repo without last command, run with -h for help")
if args.command != "pub" and args.uuid is not None:
    ap.error("cannot use -i/--uuid without pub command, run with -h for help")
if args.command != "new" and args.boards is not None:
    ap.error("cannot use -b/--boards without new command, run with -h for help")

repo_name = config.getparam("common", "repo_name")
host = config.getparam("ota", "host")

dev_headers = {"X-Token": config.getparam("ota.dev", "token")}
board_headers = {"X-Token": config.getparam("ota.board", "token")}

cert_path = config.getparam("ota", "cert_path")

if args.command == "list":
    resp = requests.get(host + "/api/v1/firmwares", headers=dev_headers, verify=cert_path)
    check_http_resp(resp)
    for firmware in resp.json():
        print_firmware_info(firmware['info'])
        print("----")

elif args.command == "last":
    if args.repo is not None:
        repo_name = args.repo

    resp = requests.get(host + "/api/v1/firmwares/latest", {'repo': repo_name},
                        headers=board_headers, verify=cert_path)
    check_http_resp(resp)
    print_firmware_info(resp.json()['info'])

elif args.command == "new":
    if args.description:
        desc = args.description
    else:
        desc = ""

    if args.boards is None:
        boards = config.getparam("ota.dev", "boards").split(',')
    else:
        boards = args.boards.split(',')

    resp = requests.post(host + "/api/v1/firmwares", headers=dev_headers, json={
        "commit_id": "55aa",
        "repo_name": repo_name,
        "boards": boards,
        "description": desc
    }, verify=cert_path)
    check_http_resp(resp)

    info = resp.json()['info']
    ota_ver_h = f"""/*
 * Файл сгенерирован автоматически с помощью tools/ota.py
 * Изменять его не нужно.
 *
 * Данные текущей загруженной OTA прошивки.
 */

#define OTA_VER_ID {info['id']}
#define OTA_VER_UUID "{escape_str(info['uuid'])}"
#define OTA_VER_REPO_NAME "{escape_str(info['repo_name'])}"
#define OTA_VER_COMMIT_ID "{escape_str(info['commit_id'])}"
#define OTA_VER_BOARDS "{escape_str(','.join(boards))}"
#define OTA_VER_CREATED_AT {info['created_at']}
#define OTA_VER_CREATED_BY "{escape_str(info['created_by'])}"
#define OTA_VER_DESCRIPTION "{escape_str(info['description'])}"

"""
    with open(f"./src/ota_ver.h", "w+") as f:
        f.write(ota_ver_h)

    print(info['uuid'])

elif args.command == "pub":
    if args.uuid is None:
        print("-i UUID should be provided")
        exit(1)

    bin_path = f"./build/{repo_name}.ino.bin"
    with open(bin_path, 'rb') as f:
        resp = requests.post(host + f"/api/v1/bin/{args.uuid}", files={'file': f}, headers=dev_headers,
                             verify=cert_path)
        check_http_resp(resp)

elif args.command == "user":
    resp = requests.get(host + "/api/v1/users/me", headers=dev_headers, verify=cert_path)
    check_http_resp(resp)
    print_user_info(resp.json())

elif args.command == "board":
    resp = requests.get(host + "/api/v1/users/me", headers=board_headers, verify=cert_path)
    check_http_resp(resp)
    print_user_info(resp.json())

