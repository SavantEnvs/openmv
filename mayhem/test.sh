#!/usr/bin/env bash
#
# mayhem/test.sh — RUN the known-answer checks mayhem/build.sh built (mayhem/oracle/oracle.c).
# exit 0 = pass. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

ORACLE="$SRC/mayhem/build/oracle"
if [ ! -x "$ORACLE" ]; then
  echo "missing $ORACLE — mayhem/build.sh did not produce it" >&2
  emit_ctrf "openmv-imlib-oracle" 0 1 0
  exit 1
fi

# Golden values: the QR/DataMatrix/barcode ones are the expected results of upstream's own tests
# (scripts/unittest/tests/{qrcodes,datamatrices,barcodes}.py) on the images in scripts/unittest/data.
# The JPEG/PNG ones pin the encoder's byte output and the decoder's pixels.
GOLDEN='qr.count=1
qr.rect=76,36,168,168
qr.payload=https://openmv.io
qr.meta=1,1,3,4,0
dm.count=1
dm.rect=34,15,90,89
dm.payload=https://openmv.io/
dm.meta=0,18,18,18,0
bc.count=1
bc.rect=11,12,514,39
bc.payload=https://openmv.io/
bc.meta=15,0,40
jpeg.valid=1
jpeg.decoded_fnv=d3cc503f
jpeg.gray_fnv=19ec51fc
jpeg.encode_rc=0
jpeg.encode_size=969
jpeg.encode_fnv=5ee5e269
jpeg.encode_valid=1
jpeg.roundtrip_fnv=557bd874
png.encode_rc=0
png.signature=1
png.encode_size=289
png.roundtrip_equal=1'

actual="$("$ORACLE" "$SRC/scripts/unittest/data" 2>/dev/null || true)"
passed=0 ; failed=0
while IFS= read -r want; do
  key="${want%%=*}"
  if printf '%s\n' "$actual" | grep -qxF -- "$want"; then
    passed=$((passed + 1))
  else
    failed=$((failed + 1))
    echo "FAIL $key: want '${want#*=}', got '$(printf '%s\n' "$actual" | sed -n "s/^$key=//p")'" >&2
  fi
done <<< "$GOLDEN"

emit_ctrf "openmv-imlib-oracle" "$passed" "$failed" 0
