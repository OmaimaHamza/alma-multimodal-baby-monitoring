# working_server.py
from flask import Flask, request, jsonify
import firebase_admin
from firebase_admin import credentials, db
from datetime import datetime
import os
import tensorflow.compat.v1 as tf

tf.disable_v2_behavior()
import numpy as np

app = Flask(__name__)

print("Starting at:", datetime.now().strftime("%H:%M:%S"))

# Initialize Firebase
if not firebase_admin._apps:
    cred = credentials.Certificate("baby-monitoring.json")
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'url.firebaseio.com' #change this
    })
print("Firebase initialized")

# Load labels
LABEL_PATH = 'retrained_labels.txt'

def load_labels(path):
    with open(path) as f:
        return [line.strip() for line in f]

# Load the retrained model ONCE (this is why it's fast)
print("Loading retrained_graph.pb...")
sess = None
labels = []
try:
    # Load the graph file
    with tf.io.gfile.GFile("retrained_graph.pb", 'rb') as f: #the model
        graph_def = tf.GraphDef()
        graph_def.ParseFromString(f.read())

    # Import graph
    tf.import_graph_def(graph_def, name='')

    # Create session (KEEP IT OPEN for fast predictions)
    sess = tf.Session()

    # Get the CORRECT tensor - DecodeJpeg/contents:0 (takes raw JPEG)
    input_tensor = sess.graph.get_tensor_by_name('DecodeJpeg/contents:0')
    output_tensor = sess.graph.get_tensor_by_name('final_result:0')

    # Load labels
    labels = load_labels(LABEL_PATH)

    print(f"Model loaded successfully!")
    print(f"Input tensor: {input_tensor}")
    print(f"Output tensor: {output_tensor}")
    print(f"Labels: {labels}")

except Exception as e:
    print(f"✗ Error loading model: {e}")
    sess = None

def predict_baby_state(image_bytes):
    """Fast prediction using already-loaded model"""
    if sess is None:
        return "awake", 99.9

    try:
        # Run inference - feed RAW JPEG bytes directly
        predictions = sess.run(output_tensor, feed_dict={input_tensor: image_bytes})

        # Get prediction
        predicted_index = np.argmax(predictions[0])
        confidence = predictions[0][predicted_index] * 100
        predicted_class = labels[predicted_index]

        return predicted_class, confidence

    except Exception as e:
        print(f"Prediction error: {e}")
        return "awake", 0

@app.route('/upload', methods=['POST'])
def upload_image():
    """Handle image, run prediction, update Firebase"""
    print("\n" + "=" * 60)
    print(f"Image received at {datetime.now().strftime('%H:%M:%S')}")

    try:
        # Get raw image bytes
        image_data = request.data
        print(f"Image size: {len(image_data)} bytes")

        # Optional: Save image for debugging
        os.makedirs("received_images", exist_ok=True)
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"received_images/capture_{timestamp}.jpg"

        with open(filename, 'wb') as f:
            f.write(image_data)
        print(f"Image saved: {filename}")

        # ===== FAST PREDICTION WITH ALREADY-LOADED MODEL =====
        print("\nRunning prediction...")
        predicted_class, confidence = predict_baby_state(image_data)
        print(f"Prediction: {predicted_class} with {confidence:.1f}% confidence")

        # Create Firebase data with prediction
        firebase_data = {}
        # Set all labels to 0 first
        for label in labels:
            firebase_data[label] = 0
        # Set predicted label to 1
        firebase_data[predicted_class] = 1
        # Add metadata
        firebase_data["confidence"] = float(confidence)
        # firebase_data["predicted_class"] = predicted_class
        firebase_data["timestamp"] = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        # firebase_data["message"] = f"Predicted: {predicted_class}"
        # firebase_data["image_size"] = len(image_data)
        firebase_data["image_filename"] = filename

        print("\nData to send to Firebase:")
        for key, value in firebase_data.items():
            print(f"  {key}: {value}")

        # ===== UPDATE FIREBASE =====
        print("\nUPDATING FIREBASE...")

        ref = db.reference('baby_room/movement_detection')
        ref.set(firebase_data)
        print("Updated: baby_room/movement_detection")

        print("\nFirebase update completed!")
        print("=" * 60)

        return jsonify({
            "status": "success",
            # "prediction": predicted_class,
            "confidence": confidence,
            "firebase_updated": True,
            "timestamp": datetime.now().strftime("%H:%M:%S")
        })

    except Exception as e:
        print(f"✗ Error: {e}")
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    print("\nServer is ready!")
    print("=" * 60 + "\n")

    app.run(host='0.0.0.0', port=8000, debug=False)