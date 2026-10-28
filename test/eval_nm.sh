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

normalize_stderr() {
    src=$1
    dst=$2
    sed 's/^[^:][^:]*:[[:space:]]/CMD: /' "$src" > "$dst"
}

run_compare_case() {
    case_name=$1
    shift

    ft_out="$TMP_DIR/${case_name}.ft.out"
    ft_err="$TMP_DIR/${case_name}.ft.err"
    ft_err_norm="$TMP_DIR/${case_name}.ft.err.norm"
    nm_out="$TMP_DIR/${case_name}.nm.out"
    nm_err="$TMP_DIR/${case_name}.nm.err"
    nm_err_norm="$TMP_DIR/${case_name}.nm.err.norm"

    LC_ALL=C ./ft_nm "$@" >"$ft_out" 2>"$ft_err"
    ft_status=$?
    LC_ALL=C nm "$@" >"$nm_out" 2>"$nm_err"
    nm_status=$?

    normalize_stderr "$ft_err" "$ft_err_norm"
    normalize_stderr "$nm_err" "$nm_err_norm"

    if [ "$ft_status" -ne "$nm_status" ] || ! cmp -s "$ft_out" "$nm_out" || ! cmp -s "$ft_err_norm" "$nm_err_norm"; then
        fail "$case_name"
        printf '  args: %s\n' "$*"
        printf '  status ft_nm=%s nm=%s\n' "$ft_status" "$nm_status"
        if ! cmp -s "$ft_out" "$nm_out"; then
            printf '  stdout diff:\n'
            diff -u "$nm_out" "$ft_out" || true
        fi
        if ! cmp -s "$ft_err_norm" "$nm_err_norm"; then
            printf '  stderr diff (normalized command prefix):\n'
            diff -u "$nm_err_norm" "$ft_err_norm" || true
        fi
        return
    fi

    pass "$case_name"
}

cd "$REPO_ROOT" || exit 1

if ! command -v nm >/dev/null 2>&1; then
    skip "system nm unavailable (all comparison tests skipped)"
    printf '\nSummary: %s passed, %s failed, %s skipped\n' "$PASS_COUNT" "$FAIL_COUNT" "$SKIP_COUNT"
    exit 0
fi

if [ ! -x ./ft_nm ]; then
    if ! command -v make >/dev/null 2>&1; then
        skip "make unavailable and ./ft_nm missing"
        printf '\nSummary: %s passed, %s failed, %s skipped\n' "$PASS_COUNT" "$FAIL_COUNT" "$SKIP_COUNT"
        exit 0
    fi
    if ! make >/dev/null 2>&1; then
        if make CFLAGS="-Wall -Wextra -Werror -std=c2x -Isrc -Isrc/elf_stream -Isrc/symbol_table -Isrc/ar_table -Isrc/nm_display -Isrc/file_type -g3" >/dev/null 2>&1; then
            pass "build ./ft_nm with make (c2x fallback)"
        else
            fail "build ./ft_nm with make"
            printf '\nSummary: %s passed, %s failed, %s skipped\n' "$PASS_COUNT" "$FAIL_COUNT" "$SKIP_COUNT"
            exit 1
        fi
    fi
fi

run_compare_case "valid-existing-elf" test/test_nm
run_compare_case "valid-existing-elf-2" test/test_nm_2
run_compare_case "valid-existing-archive" test/libft.a
run_compare_case "multiple-inputs" test/test_nm test/test_nm_2

run_compare_case "missing-file" does_not_exist_for_ft_nm

for invalid in test/header test/header_copy test/header_and_prog test/header_and_prog_copy test/header_offset_error test/error_header test/unterminated_string test/wrong_arch test/test_corrupt_strtab; do
    if [ -f "$invalid" ]; then
        run_compare_case "invalid-$(basename "$invalid")" "$invalid"
    else
        skip "missing fixture $invalid"
    fi
done

CC_BIN=${CC:-cc}
if command -v "$CC_BIN" >/dev/null 2>&1 && command -v ar >/dev/null 2>&1; then
    cat > "$TMP_DIR/fixture.c" <<'SRC'
int global_data = 7;
static int local_data = 3;
int exported_fn(void) { return global_data + local_data; }
int main(void) { return exported_fn(); }
SRC

    if "$CC_BIN" -c "$TMP_DIR/fixture.c" -o "$TMP_DIR/fixture.o" >/dev/null 2>&1; then
        run_compare_case "generated-object" "$TMP_DIR/fixture.o"

        if "$CC_BIN" "$TMP_DIR/fixture.c" -o "$TMP_DIR/fixture.elf" >/dev/null 2>&1; then
            run_compare_case "generated-executable" "$TMP_DIR/fixture.elf"
            run_compare_case "generated-multiple" "$TMP_DIR/fixture.o" "$TMP_DIR/fixture.elf"
        else
            skip "compile generated executable fixture"
        fi

        if ar rcs "$TMP_DIR/libfixture.a" "$TMP_DIR/fixture.o" >/dev/null 2>&1; then
            run_compare_case "generated-archive" "$TMP_DIR/libfixture.a"
        else
            skip "create generated archive fixture"
        fi

        if [ -e ./a.out ]; then
            skip "default-aout (./a.out already exists)"
        elif cp "$TMP_DIR/fixture.elf" ./a.out >/dev/null 2>&1; then
            run_compare_case "default-aout"
            rm -f ./a.out
        else
            skip "prepare ./a.out default input"
        fi

        for opt in -a -g -u -r -p; do
            run_compare_case "option-${opt#-}" "$opt" "$TMP_DIR/fixture.o"
        done

        if "$CC_BIN" -m32 -c "$TMP_DIR/fixture.c" -o "$TMP_DIR/fixture32.o" >/dev/null 2>&1; then
            run_compare_case "generated-32bit-object" "$TMP_DIR/fixture32.o"
        else
            skip "32-bit toolchain unavailable"
        fi
    else
        skip "compile generated object fixture"
    fi
else
    skip "compiler or ar unavailable (generated fixture tests skipped)"
fi

printf '\nSummary: %s passed, %s failed, %s skipped\n' "$PASS_COUNT" "$FAIL_COUNT" "$SKIP_COUNT"

if [ "$FAIL_COUNT" -ne 0 ]; then
    exit 1
fi

exit 0
