#!/usr/bin/env bash
# One-time, optional, free: create a self-signed code-signing certificate
# named "Earthcall Local Signing" in your login keychain.
#
#   scripts/macos/make_local_signing_identity.sh
#
# Why: macOS remembers microphone, Screen Recording and Accessibility grants by
# the app's signing identity. An ad-hoc signature changes with every build, so
# each rebuild is a stranger and the grants vanish. A stable local certificate
# keeps them. It is trusted by this Mac only; it does not make the app
# shareable (that is Developer ID + notarization, a later release step), and
# no Earthcall identity, key, or ownership has anything to do with it.
#
# Run it yourself: it writes to your login keychain, and macOS asks for your
# password to trust the certificate for code signing.
set -euo pipefail

NAME="Earthcall Local Signing"
if security find-identity -p codesigning | grep -q "\"$NAME\""; then
  echo "\"$NAME\" already exists; nothing to do."
  exit 0
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
OPENSSL=/usr/bin/openssl   # LibreSSL: its PKCS#12 output is what `security import` reads

cat > "$WORK/cert.cnf" <<EOF
[req]
distinguished_name = dn
prompt = no
x509_extensions = ext
[dn]
CN = $NAME
[ext]
basicConstraints = critical, CA:false
keyUsage = critical, digitalSignature
extendedKeyUsage = critical, codeSigning
EOF

"$OPENSSL" req -x509 -newkey rsa:2048 -nodes -days 3650 -sha256 \
  -config "$WORK/cert.cnf" -keyout "$WORK/key.pem" -out "$WORK/cert.pem" 2>/dev/null
PASS="earthcall-local-$$"
"$OPENSSL" pkcs12 -export -inkey "$WORK/key.pem" -in "$WORK/cert.pem" \
  -name "$NAME" -out "$WORK/identity.p12" -passout "pass:$PASS"

KEYCHAIN="$HOME/Library/Keychains/login.keychain-db"
security import "$WORK/identity.p12" -k "$KEYCHAIN" -P "$PASS" -T /usr/bin/codesign
echo "Trusting \"$NAME\" for code signing (macOS will ask for your password)…"
security add-trusted-cert -r trustRoot -p codeSign -k "$KEYCHAIN" "$WORK/cert.pem"

security find-identity -v -p codesigning | grep "$NAME"
echo "Done. Re-run scripts/macos/install_app.sh; grants you give Earthcall.app now persist across rebuilds."
