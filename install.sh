#!/usr/bin/env bash
set -euo pipefail

REPO_URL="${ENGINE9000_REPO_URL:-https://github.com/tsilva/engine9000-public.git}"
BRANCH="${ENGINE9000_BRANCH:-main}"
INSTALL_DIR="${ENGINE9000_INSTALL_DIR:-}"
SKIP_APT=0
SKIP_BUILD=0

PACKAGES=(
  ca-certificates
  git
  build-essential
  make
  libsdl2-dev
  libsdl2-image-dev
  libsdl2-ttf-dev
  libreadline-dev
  pkg-config
  zenity
  libpng-dev
  libgl-dev
  zlib1g-dev
)

usage() {
  cat <<EOF
Usage: install.sh [options]

Clone and build Engine9000 on Debian/Ubuntu.

Options:
  --dir PATH       Install or reuse the checkout at PATH.
                   Default: ~/engine9000-public for sudo users, otherwise
                   ./engine9000-public from the current directory.
  --repo URL       Git repository URL. Default: $REPO_URL
  --branch NAME    Branch or tag to clone. Default: $BRANCH
  --skip-apt       Do not install apt packages.
  --skip-build     Clone and configure only; do not run make.
  -h, --help       Show this help.

Example:
  curl -fsSL https://raw.githubusercontent.com/tsilva/engine9000-public/main/install.sh | sudo bash
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --dir)
      [[ $# -ge 2 ]] || { echo "Missing value for --dir" >&2; exit 2; }
      INSTALL_DIR="$2"
      shift 2
      ;;
    --repo)
      [[ $# -ge 2 ]] || { echo "Missing value for --repo" >&2; exit 2; }
      REPO_URL="$2"
      shift 2
      ;;
    --branch)
      [[ $# -ge 2 ]] || { echo "Missing value for --branch" >&2; exit 2; }
      BRANCH="$2"
      shift 2
      ;;
    --skip-apt)
      SKIP_APT=1
      shift
      ;;
    --skip-build)
      SKIP_BUILD=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

log() {
  printf '\n==> %s\n' "$*"
}

die() {
  echo "error: $*" >&2
  exit 1
}

have_cmd() {
  command -v "$1" >/dev/null 2>&1
}

default_install_dir() {
  if [[ -n "$INSTALL_DIR" ]]; then
    printf '%s\n' "$INSTALL_DIR"
    return
  fi

  if [[ "${EUID:-$(id -u)}" -eq 0 && -n "${SUDO_USER:-}" && "$SUDO_USER" != "root" ]]; then
    local sudo_home
    sudo_home="$(getent passwd "$SUDO_USER" | cut -d: -f6 || true)"
    if [[ -n "$sudo_home" ]]; then
      printf '%s\n' "$sudo_home/engine9000-public"
      return
    fi
  fi

  printf '%s\n' "$PWD/engine9000-public"
}

owner_user() {
  if [[ "${EUID:-$(id -u)}" -eq 0 && -n "${SUDO_USER:-}" && "$SUDO_USER" != "root" ]]; then
    printf '%s\n' "$SUDO_USER"
  else
    printf '%s\n' ""
  fi
}

run_as_owner() {
  local user
  user="$(owner_user)"
  if [[ -n "$user" && "$(id -u "$user")" -ne 0 && "$(id -u)" -eq 0 ]] && have_cmd sudo; then
    sudo -H -u "$user" "$@"
  else
    "$@"
  fi
}

run_as_root() {
  if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
    "$@"
    return
  fi

  if have_cmd sudo && sudo -n true 2>/dev/null; then
    sudo "$@"
    return
  fi

  die "apt package installation needs root. Re-run with sudo, for example: curl -fsSL https://raw.githubusercontent.com/tsilva/engine9000-public/main/install.sh | sudo bash"
}

is_engine9000_checkout() {
  [[ -f "$1/Makefile" && -d "$1/e9k-debugger" && -d "$1/ami9000" && -d "$1/geo9000" ]]
}

script_repo_root() {
  local src dir
  src="${BASH_SOURCE[0]:-$0}"
  if [[ -n "$src" && -f "$src" ]]; then
    dir="$(cd "$(dirname "$src")" && pwd)"
    if is_engine9000_checkout "$dir"; then
      printf '%s\n' "$dir"
    fi
  fi
}

install_apt_packages() {
  if [[ "$SKIP_APT" -eq 1 ]]; then
    log "Skipping apt package installation"
    return
  fi

  have_cmd apt-get || die "apt-get was not found. This setup script currently supports Debian/Ubuntu apt packages."

  log "Installing apt packages"
  run_as_root env DEBIAN_FRONTEND=noninteractive apt-get update
  run_as_root env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends "${PACKAGES[@]}"
}

clone_or_reuse_repo() {
  local root parent owner
  root="$1"
  parent="$(dirname "$root")"
  owner="$(owner_user)"

  if is_engine9000_checkout "$root"; then
    log "Using existing checkout at $root"
    return
  fi

  if [[ -e "$root" ]]; then
    die "$root already exists but does not look like an Engine9000 checkout"
  fi

  log "Cloning Engine9000 into $root"
  mkdir -p "$parent"
  if [[ -n "$owner" && "$(id -u)" -eq 0 ]]; then
    chown "$owner:$(id -gn "$owner")" "$parent"
  fi

  if [[ -n "$BRANCH" ]]; then
    run_as_owner git clone --branch "$BRANCH" "$REPO_URL" "$root"
  else
    run_as_owner git clone "$REPO_URL" "$root"
  fi
}

update_submodules() {
  local root
  root="$1"
  log "Initializing submodules"
  run_as_owner git -C "$root" submodule update --init --recursive
}

build_engine9000() {
  local root
  root="$1"

  if [[ "$SKIP_BUILD" -eq 1 ]]; then
    log "Skipping build"
    return
  fi

  log "Building Engine9000"
  run_as_owner make -C "$root"
}

write_helpers() {
  local root
  root="$1"

  log "Writing helper scripts"
  cat > "$root/build-engine9000.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"
exec make "$@"
EOF

  cat > "$root/run-engine9000.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT/e9k-debugger"
exec ./e9k-debugger "$@"
EOF

  chmod +x "$root/build-engine9000.sh" "$root/run-engine9000.sh"

  local owner
  owner="$(owner_user)"
  if [[ -n "$owner" && "$(id -u)" -eq 0 ]]; then
    chown "$owner:$(id -gn "$owner")" "$root/build-engine9000.sh" "$root/run-engine9000.sh"
  fi
}

main() {
  local root existing_root
  existing_root="$(script_repo_root || true)"

  if [[ -n "$existing_root" && -z "$INSTALL_DIR" ]]; then
    root="$existing_root"
  else
    root="$(default_install_dir)"
  fi

  root="$(mkdir -p "$(dirname "$root")" && cd "$(dirname "$root")" && pwd)/$(basename "$root")"

  install_apt_packages

  have_cmd git || die "git is required but was not found after apt installation"
  have_cmd make || die "make is required but was not found after apt installation"

  clone_or_reuse_repo "$root"
  update_submodules "$root"
  write_helpers "$root"
  build_engine9000 "$root"

  cat <<EOF

Setup complete.
Checkout: $root
Run:      $root/run-engine9000.sh
Help:     $root/run-engine9000.sh --help
Build:    $root/build-engine9000.sh
EOF
}

main "$@"
