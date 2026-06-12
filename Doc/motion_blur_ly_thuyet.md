# Motion Blur (Nhòe chuyển động) — Cơ sở lý thuyết

> Tài liệu giải thích vì sao ta thêm `time` vào `Ray`, và cơ sở vật lý/toán học
> đằng sau kỹ thuật motion blur trong path tracer này.
> Bám sát chương "Motion Blur" của sách *Ray Tracing: The Next Week*.

---

## 1. Bắt nguồn từ máy ảnh thật

Máy ảnh thật **không** chụp ảnh trong 0 giây. Khi bấm nút, màn trập (shutter)
**mở ra trong một khoảng thời gian** `[t0, t1]` (ví dụ 1/60 giây) rồi mới đóng.

Trong khoảng thời gian đó:

- Cảm biến **tích lũy ánh sáng liên tục**.
- Nếu vật thể **di chuyển**, ánh sáng từ nó rơi vào **nhiều vị trí khác nhau**
  trên cảm biến.
- Kết quả: vật chuyển động bị **nhòe**, vật đứng yên thì vẫn sắc nét.

```
Vật ở t0          Vật ở t1
   ●  ────────────►  ●
   └──── vệt nhòe ────┘   (cảm biến ghi lại CẢ quá trình, không phải 1 khoảnh khắc)
```

Mục tiêu của chúng ta: mô phỏng lại đúng hiện tượng vật lý này trong renderer.

---

## 2. Bài toán toán học: tích phân theo thời gian

Màu của một pixel thực chất là **trung bình ánh sáng trong suốt thời gian màn trập mở**:

$$
\text{Color}(pixel) = \frac{1}{t_1 - t_0}\int_{t_0}^{t_1} L(pixel, t)\, dt
$$

Trong đó:

- `L(pixel, t)` = lượng ánh sáng (radiance) đến pixel **tại thời điểm `t`**.
- Tích phân lấy trên toàn bộ khoảng thời gian màn trập mở `[t0, t1]`.

Đây là một tích phân **không tính được dạng đóng** (closed form) cho cảnh phức tạp,
nên ta phải **xấp xỉ** nó. Công cụ dùng để xấp xỉ chính là **Monte Carlo**.

---

## 3. Monte Carlo: thay tích phân bằng lấy mẫu ngẫu nhiên

### 3.1. Ý tưởng cốt lõi

Path tracer vốn đã bắn **rất nhiều ray cho mỗi pixel** (để chống răng cưa và
khử nhiễu — anti-aliasing). Ta tận dụng luôn điều đó:

> **Mỗi ray, ngoài việc chọn ngẫu nhiên vị trí trong pixel, còn chọn ngẫu nhiên
> một thời điểm `t` trong khoảng `[t0, t1]`.**

Đó chính xác là dòng code trong `src/core/camera.h`:

```cpp
float time = time0 + random_float(local_rand_state) * (time1 - time0);
```

Ý nghĩa: *"ray này đại diện cho ánh sáng tại một thời điểm ngẫu nhiên trong lúc
màn trập mở."*

### 3.2. Vì sao trung bình lại hội tụ về tích phân?

Định lý nền tảng của Monte Carlo: nếu lấy `N` mẫu `t_i` **phân bố đều** trên
`[t0, t1]`, thì:

$$
\int_{t_0}^{t_1} L(t)\,dt \;\approx\; \frac{t_1-t_0}{N}\sum_{i=1}^{N} L(t_i)
$$

Khi `N → ∞`, vế phải hội tụ về vế trái (luật số lớn). Sai số giảm theo tốc độ
`O(1/√N)` — đây là lý do càng nhiều samples/pixel thì ảnh càng mịn.

Điểm hay: ta **không cần code thêm vòng lặp tích phân** nào cả. Việc lấy trung bình
hàng trăm ray (mà path tracer đã làm sẵn) **tự động** thực hiện phép xấp xỉ này.
Chỉ cần mỗi ray mang một `time` ngẫu nhiên là đủ.

---

## 4. Vì sao Ray phải MANG theo `time`?

Khi ray đi vào scene, scene cần biết **vật thể đang ở đâu tại thời điểm đó**.
Xem `src/geometry/moving_sphere.h`:

```cpp
__device__ Point3 MovingSphere::center(float time) const {
    return center0 + ((time - time0) / (time1 - time0)) * (center1 - center0);
}
```

Đây là **nội suy tuyến tính (linear interpolation / lerp)**: quả cầu di chuyển
từ `center0` (tại `t0`) đến `center1` (tại `t1`) theo **đường thẳng, tốc độ đều**.

| `time` của ray | Vị trí quả cầu       |
|----------------|----------------------|
| `0.0`          | `center0` (điểm đầu) |
| `0.5`          | chính giữa           |
| `1.0`          | `center1` (điểm cuối)|

Mỗi ray "nhìn thấy" quả cầu ở một chỗ khác nhau → khi cộng dồn (trung bình) lại
trên cảm biến, ta thu được **vệt nhòe** đúng như máy ảnh thật.

> **Lưu ý:** Nội suy tuyến tính chỉ là mô hình chuyển động đơn giản nhất
> (thẳng + đều). Có thể thay bằng đường cong, gia tốc... nhưng nguyên lý không đổi:
> `time` quyết định trạng thái hình học của scene.

---

## 5. Vì sao scatter phải GIỮ NGUYÊN `time`?

Trong `src/render_kernel.cu`, mỗi khi ray phản xạ/khúc xạ, ray mới giữ nguyên
`time` của ray cũ:

```cpp
scattered = Ray(rec.p, scatter_direction, r_in.time());  // truyền lại time gốc
```

(Áp dụng cho cả 3 vật liệu: Lambertian, Metal, Dielectric.)

**Lý do vật lý:** một photon đi vào scene tại thời điểm `t`, rồi nảy qua nhiều bề
mặt. Nhưng tất cả các lần nảy đó xảy ra **gần như cùng một thời điểm**, vì ánh sáng
đi **cực nhanh** so với tốc độ vật chuyển động. Do đó toàn bộ đường đi (path) của
một ray phải dùng **cùng một `time`** thì scene mới nhất quán.

Nếu mỗi lần bounce lại random một `time` mới → tại bounce 1 quả cầu ở vị trí A,
bounce 2 nó "nhảy" sang vị trí B → cảnh bị **rách, phi vật lý**.

---

## 6. Tổng kết — bản đồ liên hệ Vật lý ↔ Code

| Khái niệm vật lý                          | Code tương ứng                          |
|-------------------------------------------|-----------------------------------------|
| Màn trập mở trong `[t0, t1]`              | `time0`, `time1` trong `Camera`         |
| Cảm biến tích lũy ánh sáng theo thời gian | Tích phân Monte Carlo (mục 2, 3)        |
| Mỗi ray = 1 mẫu thời điểm                 | `random_float()` trong `get_ray()`      |
| Ray nhớ thời điểm của mình                | trường `tm` + `Ray::time()` trong `ray.h` |
| Vật di chuyển trong lúc chụp              | `MovingSphere::center(time)`            |
| 1 photon = 1 thời điểm cố định            | truyền `r_in.time()` khi scatter        |
| Càng nhiều mẫu càng mịn (`O(1/√N)`)       | samples-per-pixel                       |

---

## 7. Luồng dữ liệu của `time` (end-to-end)

```
Camera::get_ray()
   │  sinh time ngẫu nhiên ∈ [time0, time1]
   ▼
Ray(orig, dir, time)            ← trường tm trong ray.h
   │
   ▼
MovingSphere::hit(r, ...)
   │  current_center = center(r.time())   ← nội suy vị trí
   ▼
scatter(r_in, ...)
   │  scattered = Ray(rec.p, dir, r_in.time())   ← GIỮ NGUYÊN time qua mỗi bounce
   ▼
(lặp lại đến khi ray kết thúc)
   │
   ▼
Cộng dồn màu của tất cả ray → trung bình Monte Carlo → màu pixel cuối cùng
```

---

## 8. Một lưu ý nhỏ về code

Trong `ray.h`, constructor mặc định `Ray() {}` để `tm` **chưa khởi tạo** (giá trị rác).
Constructor có tham số đã mặc định `time = 0.0f` nên hiện tại an toàn, nhưng nên
phòng ngừa bằng default member initializer:

```cpp
float tm = 0.0f;   // đảm bảo Ray() rỗng vẫn có time hợp lệ
```

---

*Tham khảo: Peter Shirley, "Ray Tracing: The Next Week" — chương Motion Blur.*
*Xem thêm file `Doc/Ray Tracing_ The Next Week.html` trong repo.*
