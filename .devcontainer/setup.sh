#!/usr/bin/env bash
# Installs the ARM compiler and Renode on Ubuntu 24.04.
# GitHub Codespaces runs this automatically; CI runs it too.
set -euo pipefail

RENODE_VERSION=1.17.0
case "$(uname -m)" in
  x86_64)  RENODE_PKG=renode-${RENODE_VERSION}.linux-portable.tar.gz ;;
  aarch64) RENODE_PKG=renode-${RENODE_VERSION}.linux-arm64-portable.tar.gz ;;
  *) echo "unsupported CPU: $(uname -m)"; exit 1 ;;
esac

sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  gcc-arm-none-eabi libnewlib-arm-none-eabi make curl ca-certificates python3-venv

sudo mkdir -p /opt/renode
curl -fsSL "https://github.com/renode/renode/releases/download/v${RENODE_VERSION}/${RENODE_PKG}" \
  | sudo tar xz -C /opt/renode --strip-components=1

# renode-test (used by `make test`) needs a few Python packages
sudo python3 -m venv /opt/renode/venv
sudo /opt/renode/venv/bin/pip install --quiet -r /opt/renode/tests/requirements.txt

sudo ln -sf /opt/renode/renode /usr/local/bin/renode
sudo tee /usr/local/bin/renode-test > /dev/null <<'EOF'
#!/usr/bin/env bash
PATH=/opt/renode/venv/bin:$PATH exec /opt/renode/renode-test "$@"
EOF
sudo chmod +x /usr/local/bin/renode-test

arm-none-eabi-gcc --version | head -1
renode --version | head -1
