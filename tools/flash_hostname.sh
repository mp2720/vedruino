#!/usr/bin/sh

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 ESP_IDF_PATH HOSTNAME PORT"
    exit 1
fi

source $1/export.sh

# same as in partitions.csv
NVS_SIZE=0x5000

nvs_csv=$(mktemp /tmp/hostname-nvs.XXXXXX.csv)
nvs_bin=$(mktemp /tmp/hostname-nvs.XXXXXX.bin)

trap 'rm -f "$nvs_csv" "$nvs_bin"' EXIT

cat > $nvs_csv <<EOF
key,type,encoding,value
mdns,namespace,,
hostname,data,string,"$2"
EOF

$IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate "$nvs_csv" "$nvs_bin" $NVS_SIZE
$IDF_PATH/components/partition_table/parttool.py -p "$3" -b 921600 write_partition --partition-name=nvs --input="$nvs_bin"

