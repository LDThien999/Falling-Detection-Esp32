from flask import Flask, request

app = Flask(__name__)

@app.route("/result", methods=["POST"])
def get_result():
    data = request.get_json()
    label = data.get("label")
    confidence = data.get("confidence")
    print(f"Dự đoán: Label = {label}, Tỷ lệ = {100*confidence:.2f}%")
    return "OK", 200

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
