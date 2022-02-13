#!/bin/bash
set -e
__dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "Update index: Files"
source ${__dir}/sist2-update-files.sh
echo "Update index: Nextcloud"
source ${__dir}/sist2-update-nextcloud.sh
echo "Done. Restarting sist2."
docker restart sist2-sist2-1
