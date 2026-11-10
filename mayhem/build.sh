#!/usr/bin/env bash
#
# mayhem/build.sh — builds the OpenMV imlib fuzz harnesses, their standalone reproducers and the
# known-answer oracle used by mayhem/test.sh.
#
# OpenMV is ARM MicroPython firmware with no host build, so the image-processing library
# (lib/imlib) is compiled for the host against stand-ins for the firmware services it calls
# (mayhem/shim: CMSIS intrinsics, the MicroPython runtime, the uma_* allocator, file I/O).
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

: "${SRC:=/mayhem}"
cd "$SRC"

OUT="$SRC/mayhem/build"
rm -rf "$OUT"
mkdir -p "$OUT/fuzz" "$OUT/test"

# imlib sources the harnesses reach (draw/blob/font/etc. are pulled in by the QR/barcode/datamatrix
# detectors and the image format conversions).
LIB_SOURCES="
  lib/imlib/imlib.c lib/imlib/jpegd.c lib/imlib/jpege.c lib/imlib/png.c lib/imlib/lodepng.c
  lib/imlib/qrcode.c lib/imlib/zbar.c lib/imlib/dmtx.c lib/imlib/draw.c lib/imlib/font.c
  lib/imlib/blob.c lib/imlib/rectangle.c lib/imlib/collections.c lib/imlib/bayer.c lib/imlib/yuv.c
  lib/imlib/ppm.c lib/imlib/bmp.c lib/imlib/fmath.c lib/imlib/sincos_tab.c lib/imlib/xyz_tab.c
  lib/imlib/lab_tab.c lib/imlib/rainbow_tab.c common/array.c common/memcpy.c mayhem/shim/shim.c
"

INCLUDES="-Imayhem/shim -Icommon -Ilib/imlib -Imayhem/harnesses"
DEFS='-DCMSIS_MCU_H="shim_mcu.h" -DIMLIB_ENABLE_IMAGE_FILE_IO'
# The firmware targets Cortex-M, where unaligned loads are legal and imlib's decoders and image
# kernels rely on them, so the alignment check would fire on every valid input. Likewise
# float-cast-overflow: fmath.h's fast float->int round is a single `vcvtr` on ARMv7 (NaN -> 0,
# out-of-range saturates), and only the host fallback (`roundf` converted to int) is UB there.
# Everything else in UBSan stays on and halting.
FUZZ_CFLAGS="$SANITIZER_FLAGS -fno-sanitize=alignment,float-cast-overflow -fsanitize=fuzzer-no-link $DEBUG_FLAGS -O1 -w -ffunction-sections -fdata-sections $INCLUDES $DEFS"
# The JPEG codec builds Huffman tables with `cc >> (bits - 5)` (negative exponent for short codes)
# and shifts negative fixed-point values left, on every valid JPEG; the shift checks only flood there.
JPEG_RELAX="-fno-sanitize=shift"
# jpegd.c (and the harnesses that #include it) also drops signed-integer-overflow: JPEGIDCT's
# fixed-point products overflow `int` on garbage tables/coefficients, and on Cortex-M (GCC, no
# -ftrapv) they wrap and are narrowed to `short`, so they are not decoder bugs, but they were 9 of
# the 28 jpeg-decode defects and crowded out the table/component-validation bugs behind them.
# Every signed-overflow defect in jpegd.c was in JPEGIDCT. jpege.c keeps the check halting.
# (A per-function ignorelist would be narrower but is not allowed, SPEC 6.2 item 17.)
JPEGD_RELAX="-fno-sanitize=shift,signed-integer-overflow"
# zbar's scan loop steps back through the image with `p += (uintptr_t)(-1) * w`, which wraps to
# p - w (well-defined on the 32-bit target), and its fixed-point edge math shifts negative values
# left; both checks only flood there. Out-of-bounds accesses are still caught by ASan.
ZBAR_RELAX="-fno-sanitize=pointer-overflow,shift-base"
TEST_CFLAGS="-O2 -w $COVERAGE_FLAGS $INCLUDES $DEFS"

obj_name() { echo "$1" | tr / _; }

compile_one() {
  # compile_one <fuzz|test> <source>
  local kind="$1" src="$2" flags extra=""
  case "$src" in
    *jpegd.c|*jpeg_decode.c|*jpeg_encode.c) [ "$kind" = fuzz ] && extra="$JPEGD_RELAX" ;;
    *jpege.c) [ "$kind" = fuzz ] && extra="$JPEG_RELAX" ;;
    *zbar.c) [ "$kind" = fuzz ] && extra="$ZBAR_RELAX" ;;
  esac
  if [ "$kind" = fuzz ]; then flags="$FUZZ_CFLAGS $extra"; else flags="$TEST_CFLAGS"; fi
  # shellcheck disable=SC2086
  $CC $flags -c "$src" -o "$OUT/$kind/$(obj_name "$src").o"
}
export -f compile_one obj_name
export OUT FUZZ_CFLAGS TEST_CFLAGS JPEG_RELAX JPEGD_RELAX ZBAR_RELAX

HARNESSES="jpeg_decode jpeg_encode png_decode png_encode qrcode_find barcode_find datamatrix_find"
# Harnesses #include the source of the file they drive (to reach its statics), so that file's own
# object is left out of their link.
declare -A OWN_OBJECT=(
  [jpeg_decode]=lib/imlib/jpegd.c [jpeg_encode]=lib/imlib/jpegd.c
  [png_decode]=lib/imlib/png.c [png_encode]=lib/imlib/png.c
)

# 1) The project (imlib subset) twice: sanitized + coverage-instrumented for fuzzing, and with
#    normal flags for the oracle.
{
  for s in $LIB_SOURCES; do echo "fuzz $s"; echo "test $s"; done
  for h in $HARNESSES; do echo "fuzz mayhem/harnesses/$h.c"; done
  echo "test mayhem/oracle/oracle.c"
} | xargs -P "$MAYHEM_JOBS" -L1 bash -c 'compile_one "$0" "$1"'

# Disable LeakSanitizer at build time (leaks are not what this fleet fuzzes for).
# shellcheck disable=SC2086
$CXX $SANITIZER_FLAGS $DEBUG_FLAGS -c mayhem/lsan_off.cc -o "$OUT/lsan_off.o"
# The run-once driver is C; compile it as C so the harness symbol keeps C linkage.
# shellcheck disable=SC2086
$CC $SANITIZER_FLAGS $DEBUG_FLAGS -c "$STANDALONE_FUZZ_MAIN" -o "$OUT/standalone_main.o"

# 2) Link every harness against the fuzzing engine and, separately, the standalone driver.
for h in $HARNESSES; do
  objs=""
  for s in $LIB_SOURCES; do
    [ "$s" = "${OWN_OBJECT[$h]:-}" ] && continue
    objs="$objs $OUT/fuzz/$(obj_name "$s").o"
  done
  harness="$OUT/fuzz/$(obj_name "mayhem/harnesses/$h.c").o"
  # shellcheck disable=SC2086
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE -Wl,--gc-sections \
      "$harness" $objs "$OUT/lsan_off.o" -lm -o "/mayhem/$h"
  # shellcheck disable=SC2086
  $CC $SANITIZER_FLAGS $DEBUG_FLAGS -Wl,--gc-sections \
      "$harness" $objs "$OUT/lsan_off.o" "$OUT/standalone_main.o" -lm -o "/mayhem/$h-standalone"
done

# 3) The known-answer oracle with the project's normal flags.
objs=""
for s in $LIB_SOURCES; do objs="$objs $OUT/test/$(obj_name "$s").o"; done
# shellcheck disable=SC2086
$CC -O2 $COVERAGE_FLAGS $objs "$OUT/test/$(obj_name mayhem/oracle/oracle.c).o" -lm -o "$OUT/oracle"
