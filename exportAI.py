import tensorflow as tf

# Load your model (h5 file)
model = tf.keras.models.load_model('fall_detection_model.h5')

# Convert to TensorFlow Lite
converter = tf.lite.TFLiteConverter.from_keras_model(model)
tflite_model = converter.convert()

# Save the TensorFlow Lite model
with open('fall_detection_model.tflite', 'wb') as f:
    f.write(tflite_model)

# Save model as .h file for embedding into ESP32
with open('Arduino/run/fall_detection_model.h', 'w') as f:
    f.write('const uint8_t model_data[] = {')
    f.write(', '.join([f'0x{byte:02x}' for byte in tflite_model]))
    f.write('};')
    print("convert complete")
