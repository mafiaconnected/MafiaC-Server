#!/usr/bin/env bash
#
# Builds MafiaC-Server from source on a Raspberry Pi 4 running Debian 13 "Trixie", 64-bit (arm64/aarch64):
#
#     packages -> clone repos -> Dependencies -> Galactic -> SpiderMonkey runtime -> MafiaC-Server -> server dir
#
# Copy this one file to the Pi and run it as a normal user with sudo rights:
#
#     bash Linux_Build_Pi.sh                      # asks for your GitHub name and personal access token
#     bash Linux_Build_Pi.sh ~/github-creds.txt   # reads them from a file instead
#
# Galactic is a private repo, so GitHub credentials are needed. They're asked for (or read) first, and checked
# against Galactic straight away, so nothing long-running starts with bad ones. The credentials file is two lines:
#
#     your-github-name
#     ghp_yourPersonalAccessToken
#
# Keep it private (chmod 600). The token needs read access to the repos (classic token: "repo" scope; fine-grained
# token: Contents read-only on jack9267/Galactic). Credentials are only handed to git in memory for this run -
# nothing is written to git's config or credential store.
#
# Everything lives under BASE_DIR (default ~/mafiac-build). Every run clones any repo that's missing and pulls the
# latest commits (plus submodules) into the ones already there. The runnable server ends up in OUT_DIR (default
# BASE_DIR/server).
#
# Environment overrides (all optional):
#   BASE_DIR           where the repos and build trees go            (default: ~/mafiac-build)
#   OUT_DIR            where the finished server is assembled        (default: BASE_DIR/server)
#   GITHUB_CREDENTIALS credentials file, same as the first argument
#   DEPENDENCIES_URL   \
#   GALACTIC_URL        } clone URLs                                 (defaults: the GitHub repos below)
#   SPIDERMONKEY_URL    }
#   SERVER_URL         /
#   SKIP_APT=1         don't install packages (already have them, or no sudo)
#   JOBS               parallel build jobs                           (default: derived from cores and RAM)
#   MOZJS_LIB          use this aarch64 libmozjs-60.so instead of the Debian 10 package (see step 4)
#
# Build trees go in CMake.tmp/Linux/<arch> inside each repo, so re-running only rebuilds what changed.

set -euo pipefail

BUILD_TYPE=Release

BASE_DIR="${BASE_DIR:-$HOME/mafiac-build}"
OUT_DIR="${OUT_DIR:-$BASE_DIR/server}"

DEPENDENCIES_URL="${DEPENDENCIES_URL:-https://github.com/jack9267/Dependencies.git}"
GALACTIC_URL="${GALACTIC_URL:-https://github.com/jack9267/Galactic.git}"
SPIDERMONKEY_URL="${SPIDERMONKEY_URL:-https://github.com/VortrexFTW/SpiderMonkey.git}"
SERVER_URL="${SERVER_URL:-https://github.com/mafiaconnected/MafiaC-Server.git}"

DEPENDENCIES_PATH="$BASE_DIR/Dependencies"
GALACTIC_PATH="$BASE_DIR/Galactic"
SPIDERMONKEY_PATH="$BASE_DIR/SpiderMonkey"
SERVER_PATH="$BASE_DIR/MafiaC-Server"
TOOLS_DIR="$BASE_DIR/tools"

MACHINE="$(uname -m)"
TAG="Linux/$MACHINE"
START_TIME=$SECONDS

step() { printf '\n\033[1;36m==> %s\033[0m\n' "$*"; }
info() { printf '    %s\n' "$*"; }
warn() { printf '\033[1;33mwarning:\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31merror:\033[0m %s\n' "$*" >&2; exit 1; }

# ---------------------------------------------------------------------------------------------------------------
# 0. Platform
# ---------------------------------------------------------------------------------------------------------------
case "$MACHINE" in
	aarch64|arm64) ;;
	armv7l|armv6l) die "this is a 32-bit OS ($MACHINE). The server needs the 64-bit (arm64) Debian/Raspberry Pi OS image." ;;
	*)             warn "written for aarch64, but this machine is $MACHINE. Only the SpiderMonkey step is arch-specific; set MOZJS_LIB to a libmozjs-60.so for $MACHINE." ;;
esac

if [ -r /etc/os-release ]; then
	. /etc/os-release
	[ "${VERSION_CODENAME:-}" = "trixie" ] || warn "written for Debian 13 (trixie), this is ${PRETTY_NAME:-unknown}. Carrying on."
fi

command -v git >/dev/null || die "git is needed before anything else: sudo apt-get install -y git"

# ---------------------------------------------------------------------------------------------------------------
# GitHub credentials - asked for up front, so the rest can run unattended
# ---------------------------------------------------------------------------------------------------------------
step "GitHub credentials"
CREDENTIALS_FILE="${1:-${GITHUB_CREDENTIALS:-}}"
GITHUB_USER=""
GITHUB_TOKEN=""

if [ -n "$CREDENTIALS_FILE" ]; then
	[ -r "$CREDENTIALS_FILE" ] || die "can't read credentials file $CREDENTIALS_FILE"
	# tr: tolerate a file saved with Windows line endings
	GITHUB_USER="$(sed -n 1p "$CREDENTIALS_FILE" | tr -d '\r' | xargs)"
	GITHUB_TOKEN="$(sed -n 2p "$CREDENTIALS_FILE" | tr -d '\r' | xargs)"
	[ -n "$GITHUB_USER" ] && [ -n "$GITHUB_TOKEN" ] ||
		die "$CREDENTIALS_FILE must have two lines: your GitHub name, then your personal access token"
	if [ -n "$(find "$CREDENTIALS_FILE" -perm /077 2>/dev/null)" ]; then
		warn "$CREDENTIALS_FILE is readable by other users; consider: chmod 600 \"$CREDENTIALS_FILE\""
	fi
	info "using the credentials in $CREDENTIALS_FILE ($GITHUB_USER)"
elif [ -t 0 ]; then
	read -r -p "    GitHub name: " GITHUB_USER
	read -r -s -p "    Personal access token (hidden): " GITHUB_TOKEN
	echo
	[ -n "$GITHUB_USER" ] && [ -n "$GITHUB_TOKEN" ] || die "both the GitHub name and the token are needed"
else
	die "no terminal to ask for GitHub credentials; pass a credentials file: bash $0 /path/to/credentials.txt"
fi

# Give git the credentials through an in-memory helper that reads them from the environment. Environment-based
# config (GIT_CONFIG_COUNT, git 2.31+) reaches every git process this script starts, including the ones cloning
# submodules. Nothing is written to disk, and git's own prompt is disabled so bad credentials fail instead of hang.
export MAFIAC_GITHUB_USER="$GITHUB_USER" MAFIAC_GITHUB_TOKEN="$GITHUB_TOKEN"
export GIT_CONFIG_COUNT=2
export GIT_CONFIG_KEY_0="credential.https://github.com.helper" GIT_CONFIG_VALUE_0=""
export GIT_CONFIG_KEY_1="credential.https://github.com.helper"
export GIT_CONFIG_VALUE_1='!f() { test "$1" = get && printf "username=%s\npassword=%s\n" "$MAFIAC_GITHUB_USER" "$MAFIAC_GITHUB_TOKEN"; }; f'
export GIT_TERMINAL_PROMPT=0
unset GITHUB_TOKEN

git ls-remote "$GALACTIC_URL" HEAD >/dev/null 2>&1 ||
	die "can't access $GALACTIC_URL with those credentials. Check the name, that the token hasn't expired, and that it has read access to that repo."
info "access to Galactic confirmed"

# Ask for the sudo password now too, rather than partway through
if [ "${SKIP_APT:-0}" != "1" ] && [ "$(id -u)" -ne 0 ] && command -v sudo >/dev/null; then
	sudo -v
fi

# ---------------------------------------------------------------------------------------------------------------
# 1. Packages
# ---------------------------------------------------------------------------------------------------------------
# The server links libcurl.a and libssl.a statically. libcurl.a doesn't carry its own dependencies, so the -dev
# package of everything it was built against has to be present too. That list changes between Debian releases,
# so the optional ones are installed only if this release has them, and step 2 checks the result.
PACKAGES=(
	build-essential cmake git pkg-config ca-certificates curl binutils patchelf
	zlib1g-dev libbz2-dev libssl-dev libsdl2-dev libcurl4-openssl-dev
)
CURL_DEP_PACKAGES=(
	libnghttp2-dev libnghttp3-dev libngtcp2-dev libidn2-dev librtmp-dev libssh2-1-dev libssh-dev libpsl-dev
	libkrb5-dev libldap-dev libbrotli-dev libzstd-dev
)

if [ "${SKIP_APT:-0}" != "1" ]; then
	step "Installing packages"
	SUDO=""
	if [ "$(id -u)" -ne 0 ]; then
		command -v sudo >/dev/null || die "not root and sudo is missing; install the packages yourself and re-run with SKIP_APT=1"
		SUDO=sudo
	fi
	$SUDO apt-get update
	for package in "${CURL_DEP_PACKAGES[@]}"; do
		if apt-cache show "$package" >/dev/null 2>&1; then
			PACKAGES+=("$package")
		fi
	done
	$SUDO apt-get install -y "${PACKAGES[@]}"
else
	step "Skipping package installation (SKIP_APT=1)"
fi

# ---------------------------------------------------------------------------------------------------------------
# 2. Preflight - fail before anything long-running starts
# ---------------------------------------------------------------------------------------------------------------
step "Checking the toolchain"

for tool in git cmake make cc g++ pkg-config readelf patchelf curl dpkg-deb sha256sum; do
	command -v "$tool" >/dev/null || die "'$tool' not found (install the packages, or drop SKIP_APT=1)"
done

CMAKE_VERSION="$(cmake --version | head -n1 | awk '{print $3}')"
if [ "$(printf '%s\n3.18\n' "$CMAKE_VERSION" | sort -V | head -n1)" != "3.18" ]; then
	die "CMake $CMAKE_VERSION is too old; Galactic needs 3.18 or newer"
fi
info "cmake $CMAKE_VERSION, $(g++ --version | head -n1), $MACHINE"

pkg-config --static --exists libcurl || {
	pkg-config --static --print-errors libcurl || true
	die "pkg-config can't describe libcurl statically; install the -dev package named above"
}
# What libcurl.a needs, minus curl itself and OpenSSL (linked statically by the server's own CMake)
CURL_DEPENDENCIES="$(pkg-config --static --libs libcurl | tr ' ' '\n' | grep -vxE -- '-l(curl|ssl|crypto)' | awk 'NF && !seen[$0]++' | tr '\n' ' ')"
echo 'int main(void){return 0;}' | cc -x c - -o /dev/null $CURL_DEPENDENCIES 2>/dev/null ||
	die "can't link libcurl's dependencies ($CURL_DEPENDENCIES); a -dev package is missing"
[ -f "$(cc -print-file-name=libcurl.a)" ] || die "libcurl.a not found (libcurl4-openssl-dev)"
[ -f "$(cc -print-file-name=libssl.a)" ]  || die "libssl.a not found (libssl-dev)"
echo '#include <bzlib.h>' | cc -x c -fsyntax-only - 2>/dev/null || die "bzlib.h not found (libbz2-dev)"

# Job count: Release builds use LTO, and one link can want a couple of GB. Keep one job per 1.5 GB of RAM.
if [ -z "${JOBS:-}" ]; then
	CORES="$(nproc)"
	MEM_KB="$(awk '/^MemTotal:/ {print $2}' /proc/meminfo)"
	SWAP_KB="$(awk '/^SwapTotal:/ {print $2}' /proc/meminfo)"
	JOBS=$(( MEM_KB / 1500000 ))
	[ "$JOBS" -lt 1 ] && JOBS=1
	[ "$JOBS" -gt "$CORES" ] && JOBS=$CORES
	if [ $(( MEM_KB + SWAP_KB )) -lt 4000000 ]; then
		warn "only $(( MEM_KB / 1024 )) MB RAM + $(( SWAP_KB / 1024 )) MB swap. If the compiler gets killed (\"signal 9\"), add swap, e.g.:"
		warn "  sudo fallocate -l 2G /swapfile && sudo chmod 600 /swapfile && sudo mkswap /swapfile && sudo swapon /swapfile"
	fi
fi
info "using $JOBS parallel jobs"

# ---------------------------------------------------------------------------------------------------------------
# 3. Sources
# ---------------------------------------------------------------------------------------------------------------
step "Fetching sources into $BASE_DIR"
mkdir -p "$BASE_DIR" "$TOOLS_DIR"

fetch_repo() { # <url> <dir> <file this script patches, or ""> [extra clone args...]
	local url="$1" dir="$2" patched="$3"
	shift 3
	if [ -d "$dir/.git" ]; then
		info "pulling $(basename "$dir")"
		# Undo this script's own source fix from the last run (step 3 re-applies it if still needed), so it can't
		# collide with upstream changing the same lines
		if [ -n "$patched" ]; then
			cp -p "$dir/$patched" "$TOOLS_DIR/$(basename "$patched").previous"
			git -C "$dir" checkout -- "$patched"
		fi
		git -C "$dir" pull --ff-only ||
			die "pulling $(basename "$dir") failed (local changes, or its branch diverged from upstream). Fix it in $dir and re-run."
		git -C "$dir" submodule update --init --recursive
	else
		info "cloning $url"
		git clone --recurse-submodules "$@" "$url" "$dir" || die "cloning $url failed"
	fi
}

fetch_repo "$DEPENDENCIES_URL" "$DEPENDENCIES_PATH" ""
fetch_repo "$GALACTIC_URL" "$GALACTIC_PATH" include/Engine/GalacticStdlib.h
fetch_repo "$SPIDERMONKEY_URL" "$SPIDERMONKEY_PATH" esr60/include/js-config.h --depth 1 # only its headers are used
fetch_repo "$SERVER_URL" "$SERVER_PATH" ""

for repo in "$DEPENDENCIES_PATH" "$GALACTIC_PATH" "$SERVER_PATH"; do
	[ -f "$repo/cmake/j-common.cmake" ] || die "$repo/cmake is empty; run: git -C \"$repo\" submodule update --init --recursive"
done
[ -n "$(ls -A "$DEPENDENCIES_PATH/Projects/RakNet/src" 2>/dev/null)" ] ||
	die "$DEPENDENCIES_PATH/Projects/RakNet/src is empty; run: git -C \"$DEPENDENCIES_PATH\" submodule update --init --recursive"

# Two small source fixes that upstream doesn't have yet. Each is skipped once the repo already contains it. The
# sed expressions keep the files' CRLF line endings.
step "Applying Linux source fixes"

# GCC 13+ no longer pulls uint64_t & co. in through <string>.
STDLIB_H="$GALACTIC_PATH/include/Engine/GalacticStdlib.h"
if grep -q 'include <stdint.h>' "$STDLIB_H"; then
	info "GalacticStdlib.h already includes <stdint.h>"
else
	sed -i '0,/^#include <string>\(\r\?\)$/s//#include <stdint.h>\1\n#include <string>\1/' "$STDLIB_H"
	grep -q 'include <stdint.h>' "$STDLIB_H" || die "couldn't add <stdint.h> to $STDLIB_H"
	info "added <stdint.h> to GalacticStdlib.h"
fi

# js-config.h only knows the Windows 64-bit macros, so on 64-bit Linux it would pick the 32-bit value layout
# (NUNBOX32) and every JS value crossing into libmozjs would be misread.
JS_CONFIG="$SPIDERMONKEY_PATH/esr60/include/js-config.h"
if grep -q '__aarch64__' "$JS_CONFIG"; then
	info "js-config.h already handles 64-bit Linux"
else
	sed -i \
		-e 's/^#if !defined(_WIN64) && !defined(_M_X64)\(\r\?\)$/#if !defined(_WIN64) \&\& !defined(_M_X64) \&\& !defined(__x86_64__) \&\& !defined(__aarch64__)\1/' \
		-e 's/^#if defined(_WIN64) || defined(_M_X64)\(\r\?\)$/#if defined(_WIN64) || defined(_M_X64) || defined(__x86_64__) || defined(__aarch64__)\1/' \
		"$JS_CONFIG"
	[ "$(grep -c '__aarch64__' "$JS_CONFIG")" = "2" ] || die "couldn't patch $JS_CONFIG for 64-bit Linux"
	info "patched js-config.h for 64-bit Linux"
fi

# Undoing and re-applying a fix gives the file a new timestamp, which would make make rebuild everything that
# includes it (all of Galactic, for GalacticStdlib.h). If it came out identical to last run, put the old one back.
for file in "$STDLIB_H" "$JS_CONFIG"; do
	previous="$TOOLS_DIR/$(basename "$file").previous"
	if [ -f "$previous" ]; then
		cmp -s "$file" "$previous" && touch -r "$previous" "$file"
		rm -f "$previous"
	fi
done

# ---------------------------------------------------------------------------------------------------------------
# 4. SpiderMonkey 60 runtime
# ---------------------------------------------------------------------------------------------------------------
# The SpiderMonkey repo has headers and Windows libraries only. Current Debian no longer packages mozjs 60, but
# Debian 10 "buster" did, for arm64: its js-config.h matches this repo's apart from the ESR point release
# (60.2 vs 60.9), and it exports every JS API symbol the server uses. It needs ICU 63, which Trixie doesn't have
# either, so that comes from buster too. All of them get an $ORIGIN rpath so they load from the server's folder.
step "Preparing libmozjs-60.so"
MOZJS_DIR="$SPIDERMONKEY_PATH/esr60/Lib/x64" # 64-bit builds use Lib/x64 on every architecture
mkdir -p "$MOZJS_DIR"
RUNTIME_LIBS=(libmozjs-60.so)

if [ -n "${MOZJS_LIB:-}" ]; then
	[ -f "$MOZJS_LIB" ] || die "MOZJS_LIB=$MOZJS_LIB does not exist"
	cp -f "$MOZJS_LIB" "$MOZJS_DIR/libmozjs-60.so"
	info "using $MOZJS_LIB"
else
	[ "$MACHINE" = "aarch64" ] || [ "$MACHINE" = "arm64" ] || die "no prebuilt libmozjs-60.so for $MACHINE; set MOZJS_LIB"

	DEBIAN_ARCHIVE="https://archive.debian.org/debian/pool/main"
	DEBS=(
		"m/mozjs60/libmozjs-60-0_60.2.3-3_arm64.deb 0a0248e457980232afd0c7b5b9a84515d2801b14aed7deafb32e8f543bd594e6"
		"i/icu/libicu63_63.1-6+deb10u3_arm64.deb 6f68c1514e49692dc88c3adb3f3dbac4a8c8a9ca2cbebf6320f7c3836b7a5a8d"
	)
	DEB_DIR="$TOOLS_DIR/debs"
	EXTRACT_DIR="$TOOLS_DIR/mozjs-arm64"
	mkdir -p "$DEB_DIR"
	rm -rf "$EXTRACT_DIR"
	for entry in "${DEBS[@]}"; do
		path="${entry%% *}" sum="${entry##* }"
		file="$DEB_DIR/$(basename "$path")"
		if ! echo "$sum  $file" | sha256sum -c --status 2>/dev/null; then
			info "downloading $(basename "$path")"
			curl -fL --retry 3 -o "$file" "$DEBIAN_ARCHIVE/$path"
			echo "$sum  $file" | sha256sum -c --status || die "checksum mismatch for $file"
		fi
		dpkg-deb -x "$file" "$EXTRACT_DIR"
	done

	SRC_LIBS="$EXTRACT_DIR/usr/lib/aarch64-linux-gnu"
	install -m 755 "$SRC_LIBS/libmozjs-60.so.0.0.0" "$MOZJS_DIR/libmozjs-60.so"
	# Debian's soname is libmozjs-60.so.0; use the plain name every other MafiaC server build ships with
	patchelf --set-soname libmozjs-60.so "$MOZJS_DIR/libmozjs-60.so"
	for icu in libicuuc libicui18n libicudata; do
		install -m 755 "$SRC_LIBS/$icu.so.63.1" "$MOZJS_DIR/$icu.so.63"
		patchelf --set-rpath '$ORIGIN' "$MOZJS_DIR/$icu.so.63"
		RUNTIME_LIBS+=("$icu.so.63")
	done
fi
patchelf --set-rpath '$ORIGIN' "$MOZJS_DIR/libmozjs-60.so"

GOT_ELF="$(readelf -h "$MOZJS_DIR/libmozjs-60.so" | awk -F: '/Machine:/ {print $2}' | xargs)"
case "$MACHINE:$GOT_ELF" in
	aarch64:*AArch64*|arm64:*AArch64*|x86_64:*X86-64*) info "libmozjs-60.so is $GOT_ELF: OK" ;;
	*) die "libmozjs-60.so is '$GOT_ELF' but this machine is $MACHINE" ;;
esac

# ---------------------------------------------------------------------------------------------------------------
# Build settings shared by every project
# ---------------------------------------------------------------------------------------------------------------
# char is unsigned on ARM Linux but signed on x86 and MSVC, which this code base was written against.
C_FLAGS="-fsigned-char"
CXX_FLAGS="-fsigned-char"
# GCC 14 (Trixie) turned these old-C warnings into errors; some of the bundled third-party C code predates that.
C_FLAGS="$C_FLAGS -Wno-error=implicit-function-declaration -Wno-error=incompatible-pointer-types -Wno-error=int-conversion"

# The shared CMake modules read these as defaults for their cache variables.
export jdependencies_home="$DEPENDENCIES_PATH"
export galactic_home="$GALACTIC_PATH"
export jspidermonkey_home="$SPIDERMONKEY_PATH"

configure() { # <source dir> <build dir> [extra cmake args...]
	local src="$1" build="$2"
	shift 2
	cmake -S "$src" -B "$build" -G "Unix Makefiles" \
		-DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$src" \
		-DCMAKE_C_FLAGS="$C_FLAGS" -DCMAKE_CXX_FLAGS="$CXX_FLAGS" \
		-DDEPENDENCIES_PATH="$DEPENDENCIES_PATH" -DGALACTIC_PATH="$GALACTIC_PATH" -DSPIDERMONKEY_PATH="$SPIDERMONKEY_PATH" \
		"$@"
}

# ---------------------------------------------------------------------------------------------------------------
# 5. Dependencies
# ---------------------------------------------------------------------------------------------------------------
# On Linux this builds zlib, png, RakNet, lua, squirrel, tinyxml, mongoose, sqlite and enet. Installing fills
# Dependencies/include and Dependencies/Lib, which the later steps read from.
step "Building Dependencies"
DEPENDENCIES_BUILD="$DEPENDENCIES_PATH/CMake.tmp/$TAG"
# libpng's default ("check") is a hard #error on ARM64, where NEON is always present anyway.
configure "$DEPENDENCIES_PATH" "$DEPENDENCIES_BUILD" -DPNG_ARM_NEON=on
cmake --build "$DEPENDENCIES_BUILD" -j"$JOBS"
cmake --install "$DEPENDENCIES_BUILD" --config "$BUILD_TYPE" >/dev/null

# ---------------------------------------------------------------------------------------------------------------
# 6. Galactic
# ---------------------------------------------------------------------------------------------------------------
# Only the modules the server links are built: the root CMakeLists also adds vendor/CEF, which is Windows-only.
# That also makes a plain 'cmake --install' fail, so each module's own install script is run instead. Building
# alone does NOT update Galactic/Lib, which is where MafiaC-Server links from.
step "Building Galactic"
GALACTIC_BUILD="$GALACTIC_PATH/CMake.tmp/$TAG"
GALACTIC_MODULES=(Engine FileIntegrity JSScripting LuaScripting Multiplayer Network Scripting SquirrelScripting)
GALACTIC_LIBS=(Multiplayer Network Scripting JSScripting SquirrelScripting LuaScripting FileIntegrity Galactic)

configure "$GALACTIC_PATH" "$GALACTIC_BUILD"
cmake --build "$GALACTIC_BUILD" -j"$JOBS" --target $(printf '%s_static ' "${GALACTIC_LIBS[@]}")

for module in "${GALACTIC_MODULES[@]}"; do
	script="$GALACTIC_BUILD/src/$module/cmake_install.cmake"
	[ -f "$script" ] || die "missing $script"
	cmake -DCMAKE_INSTALL_PREFIX="$GALACTIC_PATH" -DCMAKE_INSTALL_CONFIG_NAME="$BUILD_TYPE" -P "$script" >/dev/null
done
GALACTIC_LIB_DIR="$GALACTIC_PATH/Lib/x64"
for lib in "${GALACTIC_LIBS[@]}"; do
	[ -f "$GALACTIC_LIB_DIR/lib${lib}_static.a" ] || die "lib${lib}_static.a was not installed into $GALACTIC_LIB_DIR"
done
info "installed to $GALACTIC_LIB_DIR"

# ---------------------------------------------------------------------------------------------------------------
# 7. MafiaC-Server
# ---------------------------------------------------------------------------------------------------------------
step "Building MafiaC-Server"

# The server's CMake wants Tools/bin2h.exe, which is Windows-only. This writes the same header.
BIN2H="$TOOLS_DIR/bin2h"
cat > "$BIN2H" <<'EOF'
#!/bin/sh
# Stand-in for the Windows-only Tools/bin2h.exe:  bin2h <input> -o <output.h>
set -e
IN="$1"; OUT="$3"
NAME=$(basename "$IN" | sed 's/\.[^.]*$//; s/\([a-z0-9]\)\([A-Z]\)/\1_\2/g; s/\([A-Z]\)\([A-Z][a-z]\)/\1_\2/g' | tr '[:lower:]' '[:upper:]')
SIZE=$(wc -c < "$IN" | tr -d ' ')
{
	printf '#ifndef %s_ARCHIVE_H\n#define %s_ARCHIVE_H\n\n' "$NAME" "$NAME"
	printf 'static const unsigned long %s_SIZE = %s;\n' "$NAME" "$SIZE"
	printf 'static const unsigned char %s[%s] =\n{\n' "$NAME" "$SIZE"
	od -An -v -tx1 "$IN" | sed 's/  */ /g; s/^ //; s/ $//; s/\([0-9a-f][0-9a-f]\)/0x\1,/g'
	printf '};\n\n#endif\n'
} > "$OUT"
EOF
chmod +x "$BIN2H"

# Link fixes, passed in from here so they work whether or not the checkout's CMakeLists has them yet:
#  - The Galactic archives reference each other (Network/Scripting -> Multiplayer). GNU ld reads each archive
#    once, in order, so they're repeated at the end inside a group it loops over until everything resolves.
#  - libcurl.a's own dependencies.
#  - Keep the static OpenSSL private to the executable, so shared libraries that load the system libssl don't
#    bind to this copy (two OpenSSLs sharing state crash at shutdown).
#  - -rpath-link lets ld find libmozjs's ICU dependencies next to it.
LINK_GROUP="-Wl,--start-group"
for lib in "${GALACTIC_LIBS[@]}"; do
	LINK_GROUP="$LINK_GROUP $GALACTIC_LIB_DIR/lib${lib}_static.a"
done
LINK_GROUP="$LINK_GROUP -Wl,--end-group"

SERVER_BUILD="$SERVER_PATH/CMake.tmp/$TAG"
configure "$SERVER_PATH" "$SERVER_BUILD" \
	-DBIN2H="$BIN2H" \
	-DCMAKE_EXE_LINKER_FLAGS="-Wl,--exclude-libs,libssl.a:libcrypto.a -Wl,-rpath-link,$MOZJS_DIR" \
	-DCMAKE_CXX_STANDARD_LIBRARIES="$LINK_GROUP $CURL_DEPENDENCIES"
cmake --build "$SERVER_BUILD" -j"$JOBS"

SERVER_BINARY="$SERVER_BUILD/Server/Server"
[ -x "$SERVER_BINARY" ] || die "build finished but $SERVER_BINARY is missing"

# ---------------------------------------------------------------------------------------------------------------
# 8. Server directory
# ---------------------------------------------------------------------------------------------------------------
# The binary, the JS runtime beside it (it has an $ORIGIN rpath) and MafiaCServer.tar. The sample server.xml and
# resources are only copied if OUT_DIR doesn't have its own yet.
step "Assembling $OUT_DIR"
mkdir -p "$OUT_DIR"
# Replace by rename, so a server already running from OUT_DIR keeps its old files and isn't disturbed
install_file() { # <source> <destination dir>
	install -m 755 "$1" "$2/.$(basename "$1").new"
	mv -f "$2/.$(basename "$1").new" "$2/$(basename "$1")"
}
install_file "$SERVER_BINARY" "$OUT_DIR"
for lib in "${RUNTIME_LIBS[@]}"; do
	install_file "$MOZJS_DIR/$lib" "$OUT_DIR"
done
cp -f "$SERVER_PATH/MafiaCServer.tar" "$OUT_DIR/"
[ -e "$OUT_DIR/server.xml" ] || cp "$SERVER_PATH/Release/server.xml" "$OUT_DIR/"
[ -e "$OUT_DIR/resources" ]  || cp -r "$SERVER_PATH/Release/resources" "$OUT_DIR/"

if MISSING="$(ldd "$OUT_DIR/Server" | grep 'not found')"; then
	die "the server has unresolved libraries:
$MISSING"
fi
info "all runtime libraries resolve"

printf '\n\033[1;32mDone in %dm%02ds.\033[0m\n' $(( (SECONDS - START_TIME) / 60 )) $(( (SECONDS - START_TIME) % 60 ))
cat <<EOF

  Server:  $OUT_DIR/Server
  Start:   cd "$OUT_DIR" && ./Server

The files next to it (libmozjs-60.so, libicu*.so.63, MafiaCServer.tar) have to stay with it if you move it.
EOF
