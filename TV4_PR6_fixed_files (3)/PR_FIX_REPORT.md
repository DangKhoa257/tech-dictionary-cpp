# Báo cáo sửa TV4 – PR #6

Ngày kiểm tra: 2026-11-11

## Đã sửa

- `src/dictionary_edit.cpp`: chỉ còn `updateWordMeaning()` và `deleteWord()`. Không định nghĩa lại `hashWord`, `clearDictionary`, `addWord`, `findWord`, `listBucket`, `listAllWords`.
- Kiểm tra khóa tiếng Anh rỗng/không bắt đầu bằng ASCII A-Z; kiểm tra kết quả `hashWord()` nằm trong `[0, 25]` trước khi truy cập bucket.
- So khớp từ tiếng Anh không phân biệt hoa/thường ASCII, không thay đổi key đã lưu.
- Từ chối nghĩa rỗng hoặc chỉ có khoảng trắng; giữ nguyên byte UTF-8 của nghĩa tiếng Việt.
- Xóa đúng node khớp, nối lại danh sách đúng cho node đầu/giữa/cuối/duy nhất.
- `ui/word_dialog.*`: validate input, khóa English key ở Edit mode, phát `saveRequested` nhưng không tự đóng dialog. Caller/controller phải xác nhận thao tác thành công rồi mới gọi `accept()`.
- `ui/delete_confirm.*`: thông báo tiếng Việt và mặc định chọn No.
- `tests/dictionary_edit_test.cpp`: bổ sung test các trường hợp update/delete và integrity.
- `CMakeLists.txt`: thêm target TV4 riêng để build UI, dictionary edit và test; không gắn các module này vào app/controller hiện tại.

## Kết quả kiểm tra thực tế trong môi trường này

- Biên dịch test C++17 với `-Wall -Wextra -Wpedantic`: PASS.
- Chạy 39 assertions: PASS (`All 39 checks passed.`).
- Biên dịch và chạy UBSan: PASS (`All 39 checks passed.`).
- ASan: không chạy được do môi trường báo ASan runtime không được nạp đầu tiên; đây là lỗi runtime của môi trường, không phải kết quả test thất bại.
- Qt6 Widgets không có trong môi trường (`pkg-config Qt6Widgets` không tìm thấy), nên chưa thể xác nhận biên dịch UI Qt.
- Chưa chạy được full CMake/CTest trên toàn repository do không có bản checkout đầy đủ và Qt6 Widgets trong môi trường.
- Chưa tạo commit hoặc push PR. Không có phiên GitHub được kết nối để ghi thay đổi vào `feature/tv4`.

## Còn cần tích hợp

- TV1: nối dialog với AppController, xử lý kết quả và chỉ đóng dialog sau khi thành công.
- TV3: xác nhận quy tắc duplicate/case-insensitive cho add/search thống nhất với update/delete.
- TV5: nối tín hiệu/luồng hiển thị và refresh UI sau khi thao tác thành công.
- TV2: giữ nguyên `hashWord()`/`clearDictionary()`; TV4 chỉ gọi API hiện có.


## Additional change: Undo/Redo

Added optional `DictionaryHistory` snapshot-based undo/redo support:
- records successful add/update/delete mutations when callers capture before/after snapshots;
- supports multiple undo/redo operations;
- clears redo history after a new mutation following undo;
- preserves all buckets, linked-list order, keys, and Vietnamese meanings;
- added unit tests for add/update/delete replay, empty history, and branch invalidation.

Integration is not automatic: TV1/TV5 must wire UI actions and refresh the view,
and TV3 must record successful additions. The supplied TV4 files do not include
the controller/main-window sources, so this integration cannot be completed or
verified in this package alone.
