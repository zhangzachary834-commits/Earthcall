# Zenodo Pre-Release Security and Privacy Audit

**Date:** September 2, 2026  
**Auditor:** Gemini 3.8 Flash (High) / Antigravity  
**Session ID:** `e5fced8a-288a-4252-990b-b8c1c74c453f`  
**Target:** Entire Earthcall Repository (with focus on `saves/`, `scratch/`, `agent intercom/`, `logs/`, `docs/`, `src/`)  
**Purpose:** Pre-publication verification for permanent, public archival release on Zenodo.

---

## 1. Executive Summary

A comprehensive automated and manual security and privacy audit was conducted across the Earthcall repository in preparation for a permanent public release on Zenodo. The audit evaluated three primary vectors:
1. **API Keys & Secrets** (OpenAI, Anthropic, Google, AWS, tokens, private keys, passwords)
2. **Personal Details** (Emails, phone numbers, physical addresses, private personal chat logs; excluding Zach / Zachary Zhang's name and age per instructions)
3. **Serialized Person IDs & Cryptographic Signatures** (DIDs, Ed25519 keypairs, First Mover signatures, private identity ledgers)

### Verdict: **CONDITIONAL ALL CLEAR (Minor Cleanup Recommended Prior to Packaging)**
- **No live API keys, tokens, credentials, or private keys were found.**
- **No serialized cryptographic Person IDs (`did:earthcall:*`), secret keys, or private identity ledgers were found in saves or logs.**
- **Personal Detail Notice:** Personal email addresses (`zh************34@gmail.com`) exist in the `.git` commit metadata. If packaging for Zenodo, `.git` should be excluded (via `git archive` or `.git` folder removal). Minor local filesystem paths and collegiate references in `agent intercom/` should be reviewed if absolute anonymity beyond name/age is desired.

---

## 2. Category Findings

### Category 1: API Keys & Credentials
**Status:** **CLEAN / PASS**

| Scope / Pattern | Location | Line / Details | Assessment |
| :--- | :--- | :--- | :--- |
| OpenAI (`sk-*`, `sk-proj-*`) | Whole Repository | None | **CLEAN**. Only false positives from hyphenated terms (e.g. `task-bars`, `Flask-SocketIO`). |
| Anthropic (`sk-ant-*`) | Whole Repository | None | **CLEAN**. No Anthropic API keys found. |
| Google (`AIza*`) | Whole Repository | None | **CLEAN**. `src/Singularity/Foreign/py/api/ai_service.py` (L13, L23) properly reads from `os.environ.get("GOOGLE_API_KEY")`. |
| AWS (`AKIA*`, secret keys) | Whole Repository | None | **CLEAN**. No AWS access keys or secrets found. |
| GitHub Tokens (`ghp_*`, `gho_*`, `github_pat_*`) | Whole Repository | None | **CLEAN**. Only false positives in ImGui variable names (`start_pos_highp_x`). |
| Private Key Material (`BEGIN PRIVATE KEY`) | Whole Repository | None (except OpenSSL tests) | **CLEAN**. Only upstream unit test certs in `local_deps/openssl-3.0.13/test/certs/*.key`. |
| Cloud Storage Token | `src/Singularity/Storage/CloudStorage.cpp` | L91 | **CLEAN**. Safely reads from `std::getenv("EARTHCALL_CLOUD_TOKEN")`. |
| Environment File (`.env`) | `src/Singularity/Foreign/py/.env` | L1-L6 | **SAFE**. Contains only development flags (`PORT=5005`, `DEBUG=True`, `HOST=127.0.0.1`, `FLASK_APP=app.py`). No secrets. |
| Unit Test Secrets | `scratch/probes/test_app_secret_key.py` | L27, L44 | **SAFE**. Uses dummy string `custom-test-secret-key-[REDACTED]` and test constant `"earthcall-secret-key-logos"`. |
| MCP Configs | `.mcp.json`, `.codex/config.toml` | L4 | **SAFE**. Localhost stream only (`http://127.0.0.1:64362/stream`). |

---

### Category 2: Personal Details
**Status:** **ACTION ADVISED ON GIT METADATA & ARTIFACT HYGIENE**

*(Note: Name "Zach / Zachary Zhang" and age are excluded per instructions.)*

#### A. Email Addresses
1. **Git Commit History**:
   - **File / Source:** `.git/logs/HEAD`, commit history (e.g., commit `0412ed0d`)
   - **Data:** `zh************34@gmail.com`
   - **Action:** If the Zenodo release includes `.git`, this email will be exposed.
   - **Recommendation:** Package the release using `git archive` (e.g. `git archive -o earthcall-v1.0.zip HEAD`) or exclude the `.git/` folder when generating the release archive.
2. **Third-Party / Open-Source Attributions (Benign)**:
   - `docs/Reflections on Earthcall's Progression/Reflections on the Substrate/The_Legibility_Threshold.md:33`: `noreply@anthropic.com` (sample git trailer).
   - `src/json.hpp:327, 331, 16887, 18905`: Upstream author emails (`evan@nemerson.com`, `bjoern@hoehrmann.de`).
   - `third_party/miniaudio/miniaudio.h:5`: Upstream author email (`mackron@gmail.com`).
   - `third_party/glfw/`: Upstream maintainer emails (`elmindreda@glfw.org`, etc.).

#### B. Phone Numbers & Physical Addresses
- **Status:** **CLEAN**. 0 instances of telephone numbers or physical mailing/residential addresses found in the repository.

#### C. Local File Paths & Personal Logs
1. **Local Paths in Scratch/Tooling**:
   - `scratch/attic/debug_log.txt:2`: Mentions old local directory `/Users/zacharyzhang/D***************t/s********p`.
   - `scratch/scripts/refactor/move_robotics.py:4-5`: Hardcoded `/Users/zacharyzhang/Documents/GitHub/Earthcall/...`.
   - `.claude/settings.local.json:15, 28`: Contains local absolute paths.
2. **Contextual Mentions in Intercom Logs**:
   - `agent intercom/robots having fun and messing around (and Zach)/I HAD THE CRAZIEST DREAM LAST NIGHT.md:53, 85`:
     - Mentions status as a "college student" and references "midterm schedule".
   - `agent intercom/communication-threads/quota_expires.md`:
     - Personal conversational note regarding Claude and Grok quota exhaustion.
   - *Note:* All other contents of `agent intercom/` consist of technical discourse, architectural reflections, and collaborative development between Zach and LLM personas (Claude, Gemini, Grok, GPT-4o). No private personal correspondences or third-party identities were found.

---

### Category 3: Serialized Person IDs & Cryptographic Signatures
**Status:** **CLEAN / FULLY COMPLIANT**

| Substrate Element | Location | Status / Finding |
| :--- | :--- | :--- |
| **`did:earthcall:<base32>`** | `saves/` | **0 found**. No cryptographic Person IDs are serialized in any save file (`.json`, `.ecsave`, `.ecform`, `.ecmatter`). |
| **`ec1:<base32>` (Opaque IDs)** | `saves/` | **0 found**. No opaque cryptographic IDs found in world saves. |
| **Save File Authors** | `saves/` | All `authors` fields contain only symbolic labels: `"Player"`, `"Zach"`, `"System"`, `"first-mover"`, `"grok-4.6"`, `"Gemini"` / `"author.gemini-spark"`, or `"Creator"`. |
| **Save File Owners** | `saves/` | All `owner` fields contain only: `"Player"`, `"Antigravity"`, `"first-mover"`, `"Creator"`, or `""`. |
| **Inhabitants / Stakes** | `saves/` | Only `"Player"`. |
| **IdentityLedger & KeyStore** | Repository | **0 found**. The repository strictly respects the architectural invariant defined in `src/Identity/KeyStore.hpp`: private keys and the identity ledger are never written into the repository and default to `$EARTHCALL_HOME/identity` or `~/.earthcall/identity` outside the tree. |

---

## 3. Recommendations for Zenodo Packaging

1. **Use `git archive` for the Tarball/Zip**:
   Creating the distribution archive with `git archive` automatically omits `.git/`, `.env` files, `.idea/`, and untracked scratch files, eliminating the git history email exposure (`zh************34@gmail.com`).
   ```bash
   git archive --format=zip --output=earthcall-zenodo-release.zip HEAD
   ```
2. **Sanitize `scratch/attic/debug_log.txt`**:
   If `scratch/` is bundled into the release, remove or sanitize line 2 of `scratch/attic/debug_log.txt` which contains the historical local path `/Users/zacharyzhang/DimensionOfThought/sight-cpp`.
3. **Double-Check `agent intercom/`**:
   Verify whether the creative dream log (`I HAD THE CRAZIEST DREAM LAST NIGHT.md`) and quota log (`quota_expires.md`) are intended to be included in the public scholarly archive.
