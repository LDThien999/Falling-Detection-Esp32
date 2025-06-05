- thư mục Arduino Arduino để nạp vào arduino: 
	+ dataCollection -> thu thập data
	+ run_sendSMTP huấn luyện model, gửi các dự đoán đến server python và gửi mail khi ngã

- thư mục Dataset: 
	+ gồm 3 file chứa dữ liệu thu từ mpu6050 tương ứng với 3 nhãn: walking, falling, normal

- thư mục RequestEsp32:
	+ AIrespone.py: bật lên sau khi nhúng file huấn luyện model vào mạch(run_pythonServer) mở server nhận các dự đoán của AI.
	+ dataCollector.py: bật lên sau khi nhúng file thu thập dữ liệu vào mạch(dataCollection), nhận các dữ liệu bằng HTTP và lưu vào file csv.

- thư mục frontend:
	+ chứa giao diện nhận tín hiệu từ esp32.

- thư mục new_env: chứa các biến enviroments (bỏ qua)

- trainingAI.py: file huấn luyện AI, dùng thuật toán CNN 1D. kết quả tạo thành model đuôi h5.
- exportAI.py: chuyển đổi file model thành file đuôi h để nhúng vào arduino.
- các file model là các file model.
-traing.ipynb (file test, bỏ qua)
