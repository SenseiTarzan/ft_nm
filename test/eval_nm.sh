#!/bin/sh

PASS_COUNT=0
FAIL_COUNT=0
SKIP_COUNT=0

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/.." && pwd)
TMP_DIR=$(mktemp -d "${TMPDIR:-/tmp}/ft_nm_eval.XXXXXX") || exit 1

cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT INT TERM HUP

pass() {
    PASS_COUNT=$((PASS_COUNT + 1))
    printf 'PASS: %s\n' "$1"
}

fail() {
    FAIL_COUNT=$((FAIL_COUNT + 1))
    printf 'FAIL: %s\n' "$1"
}

skip() {
    SKIP_COUNT=$((SKIP_COUNT + 1))
    printf 'SKIP: %s\n' "$1"
}

section() {
    printf '\n== %s ==\n' "$1"
}

normalize_stderr() {
    src=$1
    dst=$2
    sed 's/^[^:][^:]*:[[:space:]]/CMD: /' "$src" > "$dst"
}

extract_symbols() {
    src=$1
    dst=$2
    awk '
        /^[[:space:]]*[0-9A-Fa-f]+[[:space:]]+[[:alpha:]?][[:space:]]+/ { print; next }
        /^[[:space:]]+[[:alpha:]?][[:space:]]+/ { print; next }
    ' "$src" > "$dst"
}

run_raw() {
    case_key=$1
    shift

    FT_OUT="$TMP_DIR/${case_key}.ft.out"
    FT_ERR="$TMP_DIR/${case_key}.ft.err"
    FT_ERR_NORM="$TMP_DIR/${case_key}.ft.err.norm"
    NM_OUT="$TMP_DIR/${case_key}.nm.out"
    NM_ERR="$TMP_DIR/${case_key}.nm.err"
    NM_ERR_NORM="$TMP_DIR/${case_key}.nm.err.norm"

    LC_ALL=C ./ft_nm "$@" >"$FT_OUT" 2>"$FT_ERR"
    FT_STATUS=$?
    LC_ALL=C nm "$@" >"$NM_OUT" 2>"$NM_ERR"
    NM_STATUS=$?

    normalize_stderr "$FT_ERR" "$FT_ERR_NORM"
    normalize_stderr "$NM_ERR" "$NM_ERR_NORM"
}

report_diff() {
    if [ "$FT_STATUS" -ne "$NM_STATUS" ]; then
        printf '  status ft_nm=%s nm=%s\n' "$FT_STATUS" "$NM_STATUS"
    fi

    if [ -f "$TMP_DIR/current_expected" ] && [ -f "$TMP_DIR/current_actual" ] && ! cmp -s "$TMP_DIR/current_expected" "$TMP_DIR/current_actual"; then
        printf '  stdout diff:\n'
        diff -u "$TMP_DIR/current_expected" "$TMP_DIR/current_actual" || true
    fi

    if [ -f "$NM_ERR_NORM" ] && [ -f "$FT_ERR_NORM" ] && ! cmp -s "$NM_ERR_NORM" "$FT_ERR_NORM"; then
        printf '  stderr diff (normalized command prefix):\n'
        diff -u "$NM_ERR_NORM" "$FT_ERR_NORM" || true
    fi
}

run_strict_case() {
    case_name=$1
    case_key=$2
    shift 2

    run_raw "$case_key" "$@"
    cp "$NM_OUT" "$TMP_DIR/current_expected"
    cp "$FT_OUT" "$TMP_DIR/current_actual"

    if [ "$FT_STATUS" -eq "$NM_STATUS" ] && cmp -s "$FT_OUT" "$NM_OUT" && cmp -s "$FT_ERR_NORM" "$NM_ERR_NORM"; then
        pass "$case_name"
        return
    fi

    fail "$case_name"
    printf '  args: %s\n' "$*"
    report_diff
}

run_symbols_case() {
    case_name=$1
    case_key=$2
    order_mode=$3
    shift 3

    run_raw "$case_key" "$@"

    ft_sym="$TMP_DIR/${case_key}.ft.sym"
    nm_sym="$TMP_DIR/${case_key}.nm.sym"
    ft_sym_cmp="$TMP_DIR/${case_key}.ft.sym.cmp"
    nm_sym_cmp="$TMP_DIR/${case_key}.nm.sym.cmp"

    extract_symbols "$FT_OUT" "$ft_sym"
    extract_symbols "$NM_OUT" "$nm_sym"

    if [ "$order_mode" = "unordered" ]; then
        LC_ALL=C sort "$ft_sym" > "$ft_sym_cmp"
        LC_ALL=C sort "$nm_sym" > "$nm_sym_cmp"
    else
        cp "$ft_sym" "$ft_sym_cmp"
        cp "$nm_sym" "$nm_sym_cmp"
    fi

    cp "$nm_sym_cmp" "$TMP_DIR/current_expected"
    cp "$ft_sym_cmp" "$TMP_DIR/current_actual"

    if [ "$FT_STATUS" -eq "$NM_STATUS" ] && cmp -s "$ft_sym_cmp" "$nm_sym_cmp"; then
        pass "$case_name"
        return
    fi

    fail "$case_name"
    printf '  args: %s\n' "$*"
    if [ "$FT_STATUS" -ne "$NM_STATUS" ]; then
        printf '  status ft_nm=%s nm=%s\n' "$FT_STATUS" "$NM_STATUS"
    fi
    if ! cmp -s "$ft_sym_cmp" "$nm_sym_cmp"; then
        printf '  symbol diff (%s):\n' "$order_mode"
        diff -u "$nm_sym_cmp" "$ft_sym_cmp" || true
    fi
}

build_ft_nm() {
    if [ -x ./ft_nm ]; then
        return 0
    fi

    if ! command -v make >/dev/null 2>&1; then
        skip "make unavailable and ./ft_nm missing"
        return 1
    fi

    if make >/dev/null 2>&1; then
        pass "build ./ft_nm with make"
        return 0
    fi

    if make CFLAGS="-Wall -Wextra -Werror -std=c2x -Isrc -Isrc/elf_stream -Isrc/symbol_table -Isrc/ar_table -Isrc/nm_display -Isrc/file_type -g3" >/dev/null 2>&1; then
        pass "build ./ft_nm with make (c2x fallback)"
        return 0
    fi

    fail "build ./ft_nm with make"
    return 1
}

find_dynamic_library() {
    if [ -f /usr/lib/x86_64-linux-gnu/libc.so.6 ]; then
        printf '%s\n' /usr/lib/x86_64-linux-gnu/libc.so.6
        return
    fi

    found=$(find /usr/lib -type f \( -name '*.so' -o -name '*.dylib' \) 2>/dev/null | head -n 1)
    if [ -n "$found" ]; then
        printf '%s\n' "$found"
    fi
}

prepare_fixtures() {
    CC_BIN=${CC:-cc}

    if ! command -v "$CC_BIN" >/dev/null 2>&1; then
        skip "compiler unavailable (fixture compilation skipped)"
        return 1
    fi

    cat > "$TMP_DIR/easy_test.c" <<'SRC'
int easy_global = 42;
int easy_fn(void) { return easy_global; }
int main(void) { return easy_fn(); }
SRC

    cat > "$TMP_DIR/not_so_easy_test.c" <<'SRC'
int global_data = 7;
static int local_data = 3;
__attribute__((weak)) int weak_symbol = 1;
int exported_fn(void) { return global_data + local_data + weak_symbol; }
int main(void) { return exported_fn(); }
SRC

    if ! "$CC_BIN" "$TMP_DIR/easy_test.c" -o "$TMP_DIR/easy_test" >/dev/null 2>&1; then
        skip "compile easy_test.c"
        return 1
    fi
    if ! "$CC_BIN" "$TMP_DIR/not_so_easy_test.c" -o "$TMP_DIR/not_so_easy_test" >/dev/null 2>&1; then
        skip "compile not_so_easy_test.c"
        return 1
    fi

    if "$CC_BIN" -m32 "$TMP_DIR/not_so_easy_test.c" -o "$TMP_DIR/not_so_easy_test_32-bit" >/dev/null 2>&1; then
        HAVE_32BIT=1
    else
        HAVE_32BIT=0
        skip "32-bit toolchain unavailable for not_so_easy_test_32-bit"
    fi

    if "$CC_BIN" -c "$TMP_DIR/easy_test.c" -o "$TMP_DIR/easy_test_64.o" >/dev/null 2>&1; then
        HAVE_OBJ64=1
    else
        HAVE_OBJ64=0
        skip "compile 64-bit object fixture"
    fi

    if "$CC_BIN" -m32 -c "$TMP_DIR/easy_test.c" -o "$TMP_DIR/easy_test_32.o" >/dev/null 2>&1; then
        HAVE_OBJ32=1
    else
        HAVE_OBJ32=0
        skip "32-bit toolchain unavailable for object fixture"
    fi

    if "$CC_BIN" -shared -fPIC "$TMP_DIR/easy_test.c" -o "$TMP_DIR/libeasy_test.so" >/dev/null 2>&1; then
        HAVE_LOCAL_SO=1
    else
        HAVE_LOCAL_SO=0
        skip "compile local dynamic library fixture"
    fi

    return 0
}

check_no_cheat() {
    if grep -R -E 'system\(|popen\(|execl\(|execv\(|execvp\(|objdump|readelf|[[:<:]]nm[[:>:]]' src >/dev/null 2>&1; then
        fail "preliminary/no-cheat (suspicious external tool usage in src/)"
    else
        pass "preliminary/no-cheat"
    fi
}

cd "$REPO_ROOT" || exit 1

if ! command -v nm >/dev/null 2>&1; then
    skip "system nm unavailable (all comparison tests skipped)"
    printf '\nSummary: %s passed, %s failed, %s skipped\n' "$PASS_COUNT" "$FAIL_COUNT" "$SKIP_COUNT"
    exit 0
fi

section "Preliminary tests"
check_no_cheat
if ! build_ft_nm; then
    printf '\nSummary: %s passed, %s failed, %s skipped\n' "$PASS_COUNT" "$FAIL_COUNT" "$SKIP_COUNT"
    exit 1
fi

if [ ! -d ./src ] || [ ! -d ./test ]; then
    fail "preliminary/repository-layout"
else
    pass "preliminary/repository-layout"
fi

prepare_fixtures

section "nm error tests"
run_strict_case "error/missing-file" "error_missing" does_not_exist_for_ft_nm
for invalid in test/header test/header_copy test/header_and_prog test/header_and_prog_copy test/header_offset_error test/error_header test/unterminated_string test/wrong_arch test/test_corrupt_strtab; do
    if [ -f "$invalid" ]; then
        run_strict_case "error/$(basename "$invalid")" "error_$(basename "$invalid")" "$invalid"
    else
        skip "error fixture missing: $invalid"
    fi
done

section "Nm easy test"
if [ -f "$TMP_DIR/easy_test" ]; then
    run_strict_case "easy/easy_test" "easy_test" "$TMP_DIR/easy_test"
else
    skip "easy/easy_test (fixture unavailable)"
fi

section "Test less easy"
if [ -f "$TMP_DIR/not_so_easy_test" ]; then
    run_symbols_case "less-easy/not_so_easy_test (64-bit)" "less_easy_64" "ordered" "$TMP_DIR/not_so_easy_test"
else
    skip "less-easy/64-bit fixture unavailable"
fi
if [ "${HAVE_32BIT:-0}" -eq 1 ] && [ -f "$TMP_DIR/not_so_easy_test_32-bit" ]; then
    run_symbols_case "less-easy/not_so_easy_test_32-bit" "less_easy_32" "ordered" "$TMP_DIR/not_so_easy_test_32-bit"
else
    skip "less-easy/not_so_easy_test_32-bit"
fi

section "Other Test"
if [ -f test/test_nm ] && [ -f test/test_nm_2 ]; then
    run_symbols_case "other/test_nm" "other_test_nm" "ordered" test/test_nm
    run_symbols_case "other/test_nm_2" "other_test_nm_2" "ordered" test/test_nm_2
else
    skip "other/existing binaries unavailable"
fi

section "Multiples arguments"
if [ -f test/test_nm ] && [ -f test/test_nm_2 ]; then
    run_strict_case "multi/test_nm+test_nm_2" "multi_existing" test/test_nm test/test_nm_2
elif [ -f "$TMP_DIR/easy_test" ] && [ -f "$TMP_DIR/not_so_easy_test" ]; then
    run_strict_case "multi/easy+not_so_easy" "multi_generated" "$TMP_DIR/easy_test" "$TMP_DIR/not_so_easy_test"
else
    skip "multiple-arguments fixtures unavailable"
fi

section "Object files"
if [ "${HAVE_OBJ64:-0}" -eq 1 ]; then
    run_symbols_case "object/easy_test_64.o" "obj64" "unordered" "$TMP_DIR/easy_test_64.o"
else
    skip "object/64-bit fixture unavailable"
fi
if [ "${HAVE_OBJ32:-0}" -eq 1 ]; then
    run_symbols_case "object/easy_test_32.o" "obj32" "unordered" "$TMP_DIR/easy_test_32.o"
else
    skip "object/32-bit fixture unavailable"
fi

section "Dynamic library"
DYNAMIC_LIB=$(find_dynamic_library)
if [ -n "$DYNAMIC_LIB" ] && [ -f "$DYNAMIC_LIB" ]; then
    run_symbols_case "dynamic/system-library" "dynamic_system" "unordered" "$DYNAMIC_LIB"
elif [ "${HAVE_LOCAL_SO:-0}" -eq 1 ] && [ -f "$TMP_DIR/libeasy_test.so" ]; then
    run_symbols_case "dynamic/local-fixture" "dynamic_local" "unordered" "$TMP_DIR/libeasy_test.so"
else
    skip "dynamic-library unavailable"
fi

section "Universal binary"
UNIVERSAL_BIN=""
if command -v file >/dev/null 2>&1; then
    for candidate in /usr/bin/python /usr/bin/python3 /bin/ls; do
        if [ -f "$candidate" ] && file "$candidate" | grep -iq 'universal'; then
            UNIVERSAL_BIN=$candidate
            break
        fi
    done
fi

if [ -n "$UNIVERSAL_BIN" ]; then
    run_symbols_case "universal/$UNIVERSAL_BIN" "universal_bin" "unordered" "$UNIVERSAL_BIN"
else
    skip "universal binary not found on this environment"
fi

printf '\nSummary: %s passed, %s failed, %s skipped\n' "$PASS_COUNT" "$FAIL_COUNT" "$SKIP_COUNT"

if [ "$FAIL_COUNT" -ne 0 ]; then
    exit 1
fi

exit 0
