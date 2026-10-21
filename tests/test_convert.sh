#!/bin/sh
# Test công cụ chuyển mã vilikey-convert.
CONV="${1:?đường dẫn vilikey-convert}"
fail=0; total=0
check() {  # check "mô tả" "kết quả" "mong đợi"
    total=$((total+1))
    if [ "$2" = "$3" ]; then echo "  OK   $1"; else echo "  FAIL $1: '$2' (mong đợi '$3')"; fail=$((fail+1)); fi
}
S="Trường Đại học Cần Thơ — chuyển đổi số"

for cs in tcvn3 vni viqr cp1258 ncr-dec ncr-hex unicode-decomposed; do
    r=$(printf '%s' "$S" | "$CONV" -f unicode -t $cs | "$CONV" -f $cs -t unicode)
    check "khứ hồi Unicode -> $cs -> Unicode" "$r" "$S"
done
# VISCII và VPS không có ký tự "—", chỉ thử với chữ tiếng Việt
S2="Trường Đại học Cần Thơ, chuyển đổi số"
for cs in viscii vps; do
    r=$(printf '%s' "$S2" | "$CONV" -f unicode -t $cs | "$CONV" -f $cs -t unicode)
    check "khứ hồi Unicode -> $cs -> Unicode" "$r" "$S2"
done

# Byte TCVN3 đã biết: "đ" = 0xAE, "Đ" = 0xA7
check "TCVN3 byte của 'đĐ'" "$(printf 'đĐ' | "$CONV" -f unicode -t tcvn3 | od -An -tx1 | tr -d ' ')" "aea7"
check "VNI: 'Vieät Nam'" "$(printf 'Vie\344t Nam' | "$CONV" -f vni -t unicode)" "Việt Nam"

# Văn bản mã cũ dán từ Word/LibreOffice (ký tự Windows-1252)
legacy=$(printf '%s' "$S" | "$CONV" -f unicode -t tcvn3 --to-text)
check "TCVN3 dạng văn bản -> Unicode" "$(printf '%s' "$legacy" | "$CONV" -f tcvn3 --from-text)" "$S"
legacy=$(printf '%s' "$S" | "$CONV" -f unicode -t vni --to-text)
check "VNI dạng văn bản -> Unicode" "$(printf '%s' "$legacy" | "$CONV" -f vni --from-text)" "$S"

check "bỏ dấu" "$(printf 'Trường Đại học Cần Thơ' | "$CONV" -f unicode --strip)" "Truong Dai hoc Can Tho"
check "chữ hoa" "$(printf 'Đại học Cần Thơ' | "$CONV" -f unicode --upper)" "ĐẠI HỌC CẦN THƠ"
check "chữ thường" "$(printf 'ĐẠI HỌC' | "$CONV" -f unicode --lower)" "đại học"
check "TCVN3 -> Unicode, bỏ dấu" "$(printf 'đĐ' | "$CONV" -f unicode -t tcvn3 | "$CONV" -f tcvn3 --strip)" "dD"

echo "$((total-fail))/$total trường hợp chuyển mã đạt"
[ $fail -eq 0 ]
