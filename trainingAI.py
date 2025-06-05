import pandas as pd
import numpy as np
from sklearn.preprocessing import LabelEncoder
from sklearn.model_selection import train_test_split
import tensorflow as tf
from tensorflow.keras import layers, models

# Load dataset
def load_dataset(file_path, label):
    data = pd.read_csv(file_path)
    # Thêm nhãn vào dữ liệu
    data['label'] = label
    return data

falling_data = load_dataset('dataset/falling_dataset.csv', 'falling')
walking_data = load_dataset('dataset/walking_dataset.csv', 'walking')
normal_data = load_dataset('dataset/normal_dataset.csv', 'normal')

# Gộp dữ liệu lại thành 1 DataFrame
all_data = pd.concat([falling_data, walking_data, normal_data], ignore_index=True)
all_data.columns = all_data.columns.str.strip()

# Chia dữ liệu thành X (features) và y (labels)
X = all_data[['accelX', 'accelY', 'accelZ', 'gyroX', 'gyroY', 'gyroZ']].values
y = all_data['label'].values

# Chuyển nhãn thành số (encoding)
label_encoder = LabelEncoder()
y = label_encoder.fit_transform(y)

# Chia dữ liệu thành tập huấn luyện và kiểm tra
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Chuyển đổi dữ liệu thành dạng 3D (số mẫu, số đặc trưng, 1)
X_train = X_train.reshape((X_train.shape[0], X_train.shape[1], 1))
X_test = X_test.reshape((X_test.shape[0], X_test.shape[1], 1))

# Mô hình CNN 1D
model = models.Sequential([
    layers.Conv1D(64, 3, activation='relu', input_shape=(X_train.shape[1], 1), padding='same'),
    layers.MaxPooling1D(2),
    
    layers.Conv1D(128, 3, activation='relu', padding='same'),
    layers.MaxPooling1D(2),
    
    layers.Flatten(),
    layers.Dense(64, activation='relu'),
    layers.Dense(3, activation='softmax')  # 3 nhãn: falling, walking, normal
])


# Biên dịch mô hình
model.compile(optimizer='adam', loss='sparse_categorical_crossentropy', metrics=['accuracy'])

# Huấn luyện mô hình
model.fit(X_train, y_train, epochs=50, batch_size=32, validation_data=(X_test, y_test))

# Lưu mô hình
model.save('fall_detection_model.h5')
print("Saved model")
