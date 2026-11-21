# Thiết kế Parallel Brute Force (TV2)

## 1. Mô hình song song và cách ánh xạ iteration cho thread

Vòng lặp tuần tự duyệt `index = 0 .. N-1` được chuyển thành vòng lặp OpenMP
`#pragma omp parallel for`. OpenMP tự chia dải `[0, N-1]` cho các thread theo
kiểu scheduling đã chọn — mỗi `index` được xử lý đúng một lần, không trùng
không sót, vì bản chất của `parallel for` là phân vùng một vòng lặp chỉ số
nguyên liên tục.

## 2. Các chỉ thị OpenMP sử dụng

- `#pragma omp parallel for schedule(runtime) num_threads(N) reduction(+:attempts) reduction(min:found_index)`
- `omp_set_schedule()` được gọi trước vùng song song để thiết lập cụ thể
  `static` hoặc `dynamic` (kèm `chunk_size`) cho `schedule(runtime)` — đây là
  cách chuẩn để chọn kiểu lập lịch tại thời điểm chạy thay vì biên dịch cứng.
- `omp_get_wtime()` đo thời gian, chỉ bao quanh vùng thực thi thuật toán.

## 3. Phân loại dữ liệu shared/private

| Dữ liệu | Loại | Lý do |
|---|---|---|
| `charset`, `target`, `password_length`, `total` | shared (chỉ đọc) | Mọi thread đọc cùng cấu hình, không ai ghi |
| `candidate` | private | Khai báo bên trong thân vòng lặp → mỗi iteration/thread có vùng nhớ riêng |
| `attempts` | reduction(+) | Tránh nhiều thread cùng tăng một biến shared không đồng bộ |
| `found_index` | reduction(min) | Lấy chỉ số nhỏ nhất khớp mà không cần critical/atomic thủ công |
| `index` | private (ngầm định bởi OpenMP) | Mỗi iteration có chỉ số riêng |

## 4. Race condition có thể xảy ra và cách khắc phục

Nếu `attempts++` hoặc so sánh/ghi `found_index` được thực hiện trực tiếp trên
biến shared mà không có `reduction`/`atomic`/`critical`, nhiều thread có thể
đọc-sửa-ghi cùng lúc, dẫn đến mất lượt đếm (`attempts` nhỏ hơn N thực tế) hoặc
ghi đè `found_index` không xác định. Giải pháp đã dùng: `reduction(+:attempts)`
và `reduction(min:found_index)` — OpenMP tự cấp bản sao cục bộ cho mỗi thread
rồi gộp lại an toàn ở cuối vùng song song.

## 5. So sánh static/dynamic (nguyên lý — điền số liệu thực đo khi có benchmark từ TV3)

| Chế độ | Cách hoạt động | Overhead | Phù hợp khi |
|---|---|---|---|
| static | Chia trước thành các phần gần bằng nhau | Thấp | Mỗi iteration có chi phí gần đều (đúng với bài toán này vì mọi candidate có cùng độ dài, cùng chi phí so sánh) |
| dynamic | Thread lấy từng chunk khi xong phần trước | Cao hơn (điều phối liên tục) | Workload không đều giữa các iteration |

Vì workload ở bài toán này gần như đồng đều, dự kiến `static` sẽ có lợi thế
nhẹ về overhead — nhưng kết luận cuối cùng phải dựa trên số liệu đo thực tế
từ TV3 (không kết luận trước khi có số liệu, theo đúng mục 5.5 tài liệu).

## 6. Hạn chế

- Overhead tạo/điều phối team OpenMP được tính vào thời gian đo (team được
  tạo bên trong `run_parallel`), theo đúng quy ước mục 3.7.
- Speedup không tăng tuyến tính theo số thread do: giới hạn số core vật lý,
  chi phí đồng bộ của `reduction`, và băng thông bộ nhớ khi nhiều thread
  cùng ghi vào buffer `candidate` riêng nhưng cạnh tranh cache.
- Không triển khai early-stopping trong bản chính (đúng mục 5.6 — không bắt
  buộc, và nếu làm phải tách thí nghiệm riêng).
