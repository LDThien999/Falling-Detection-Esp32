from flask import Flask, request
from datetime import datetime
import os

app = Flask(__name__)
log_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), "../dataset/normal_dataset.csv")
labels = "accelX,accelY,accelZ,gyroX,gyroY,gyroZ"

if not os.path.exists(log_file):
    with open(log_file, "a") as f:
        f.write(labels + "\n")

@app.route('/data', methods=['POST'])
def receive_data():
    data = request.get_json()
    if data:
        # Ghi dữ liệu vào file
        with open(log_file, "a") as f:
            line = f"{data['accelX']}, {data['accelY']}, {data['accelZ']}, {data['gyroX']}, {data['gyroY']}, {data['gyroZ']}\n"
            f.write(line)
        return "OK", 200
    return "Bad Request", 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
