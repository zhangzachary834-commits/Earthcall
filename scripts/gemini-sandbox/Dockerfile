FROM node:22-bookworm-slim

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
       build-essential \
       ca-certificates \
       clang \
       cmake \
       git \
       jq \
       ninja-build \
       pkg-config \
       python3 \
       python3-pip \
       ripgrep \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# No ENTRYPOINT: the broker supplies a structured argv directly. Runtime
# confinement (--network none, cap-drop, read-only root, limits, user mapping)
# is applied by earthcall-sandbox.js rather than baked into this image.
CMD ["node", "--version"]
