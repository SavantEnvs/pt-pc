#!/usr/bin/env bash
#
# mayhem/test.sh — RUN this repo's OWN functional test suite (already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
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

# Runs upstream's own unit tests for the parsers we fuzz (built by mayhem/build.sh with normal flags):
#   tests/mods_test.cpp   — mod keys, mod.json parsing, priorities, mod folder discovery/overrides,
#                           fpk entry overrides and the sandboxed Lua host; prints one PASS/FAIL line per check
#   tests/test_strcode.cpp — StrCode64 known-answer vectors (20 hashes); prints "<n> failures"
T="$SRC/mayhem/build/test"
for b in pt_mods_test pt_test_strcode; do
  [ -x "$T/$b" ] || { echo "test.sh: $T/$b missing — build.sh did not build it" >&2; emit_ctrf "pt-unit-tests" 0 1 0; exit 1; }
done
passed=0; failed=0
work=$(mktemp -d "${TMPDIR:-/tmp}/pt_mods_test.XXXXXX")
out=$("$T/pt_mods_test" "$work/root" 2>&1); rc=$?
rm -rf "$work"
echo "$out"
p=$(grep -c '^PASS ' <<<"$out" || true); f=$(grep -c '^FAIL ' <<<"$out" || true)
passed=$((passed + p)); failed=$((failed + f))
# the runner must have reached its summary line, with every check passing
if [ "$rc" -ne 0 ] || ! grep -qx 'PASS: 0 failure(s)' <<<"$out" || [ "$p" -lt 30 ]; then
  echo "test.sh: pt_mods_test did not complete cleanly (rc=$rc, $p checks passed)" >&2
  failed=$((failed + 1))
fi
out=$("$T/pt_test_strcode" 2>&1); rc=$?
echo "$out"
n=$(sed -n 's/^\([0-9][0-9]*\) failures$/\1/p' <<<"$out")
if [ "$rc" -eq 0 ] && [ "$n" = "0" ]; then passed=$((passed + 20)); elif [ -n "$n" ] && [ "$n" -gt 0 ]; then failed=$((failed + n)); else failed=$((failed + 20)); fi
emit_ctrf "pt-unit-tests" "$passed" "$failed" 0
