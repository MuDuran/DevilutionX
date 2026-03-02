#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

DOCKER_IMAGE="devkitpro/devkita64:latest"
BUILD_TYPE="${BUILD_TYPE:-RelWithDebInfo}"
BUILD_DIR="build-switch"
OUTPUT_NRO="devilutionx.nro"
CONTAINER_NAME="devilutionx-switch-build-$$"

usage() {
	cat <<-EOF
	Usage: $(basename "$0") [options]

	Build the Nintendo Switch version of DevilutionX in a Docker container.

	Options:
	  --build-type TYPE   CMake build type (default: RelWithDebInfo)
	  --pull              Pull the latest Docker image before building
	  --shell             Drop into an interactive shell instead of building
	  -h, --help          Show this help message
	EOF
}

cleanup() {
	echo "==> Cleaning up container..."
	docker rm -f "${CONTAINER_NAME}" >/dev/null 2>&1 || true
}

PULL=0
SHELL_MODE=0

while [[ $# -gt 0 ]]; do
	case "$1" in
		--build-type)
			BUILD_TYPE="$2"
			shift 2
			;;
		--pull)
			PULL=1
			shift
			;;
		--shell)
			SHELL_MODE=1
			shift
			;;
		-h|--help)
			usage
			exit 0
			;;
		*)
			echo "Unknown option: $1" >&2
			usage
			exit 1
			;;
	esac
done

if ! command -v docker &>/dev/null; then
	echo "Error: docker is not installed or not in PATH." >&2
	exit 1
fi

if (( PULL )); then
	echo "==> Pulling ${DOCKER_IMAGE}..."
	docker pull "${DOCKER_IMAGE}"
fi

trap cleanup EXIT

BUILD_SCRIPT=$(cat <<INNEREOF
set -euo pipefail

apt-get update -qq && \
apt-get install -y -qq --no-install-recommends --no-install-suggests gettext >/dev/null

cmake \
    -S /src \
    -B /src/${BUILD_DIR} \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_TOOLCHAIN_FILE=/opt/devkitpro/cmake/Switch.cmake

cmake --build /src/${BUILD_DIR} -j\$(nproc)
INNEREOF
)

if (( SHELL_MODE )); then
	echo "==> Launching interactive shell in ${DOCKER_IMAGE}..."
	docker run --rm -it --name "${CONTAINER_NAME}" "${DOCKER_IMAGE}" bash
	exit 0
fi

echo "==> Building Nintendo Switch version (${BUILD_TYPE})..."

docker create --name "${CONTAINER_NAME}" "${DOCKER_IMAGE}" bash -c "${BUILD_SCRIPT}"
docker cp "${PROJECT_ROOT}/." "${CONTAINER_NAME}:/src"
docker start -a "${CONTAINER_NAME}"

mkdir -p "${PROJECT_ROOT}/${BUILD_DIR}"
docker cp "${CONTAINER_NAME}:/src/${BUILD_DIR}/${OUTPUT_NRO}" "${PROJECT_ROOT}/${BUILD_DIR}/${OUTPUT_NRO}"

echo "==> Build complete: ${BUILD_DIR}/${OUTPUT_NRO}"
