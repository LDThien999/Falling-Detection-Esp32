- file Arduino để nạp vào arduino: 
	+ dataCollection -> thu thập data
	+ run_sendSMTP huấn luyện model, gửi các dự đoán đến server python và gửi mail khi ngã
	+ run_pythonServer: gửi các dự đoán đến server python (file này chỉ để tạm thời không cần quan tâm)

-file dataset: 
	+ gồm 3 file chứa dữ liệu thu từ mpu6050 tương ứng với 3 nhãn: walking, falling, normal

- RequestEsp32:
	+ AIrespone.py: bật lên sau khi nhúng file huấn luyện model vào mạch(run_pythonServer) mở server nhận các dự đoán của AI.
	+ dataCollector.py: bật lên sau khi nhúng file thu thập dữ liệu vào mạch(dataCollection), nhận các dữ liệu bằng HTTP và lưu vào file csv.

- trainingAI.py: file huấn luyện AI, dùng thuật toán CNN 1D. kết quả tạo thành model đuôi h5.

- exportAI.py: chuyển đổi file model thành file đuôi h để nhúng vào arduino.

- các file model là các file model thôi.

-traing.ipynb (cho vui kệ nó đi)