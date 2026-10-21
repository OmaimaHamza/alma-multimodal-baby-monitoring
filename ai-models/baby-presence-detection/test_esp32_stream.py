from ultralytics import YOLO
import torch
import torch.serialization
from ultralytics.nn import tasks
from torch.nn.modules.container import Sequential
from torch.nn.modules.conv import Conv2d
import cv2
import numpy as np
import socket
import time
import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry
import firebase_admin
from firebase_admin import credentials, db

# Safe globals setup for PyTorch 2.6
torch.serialization.add_safe_globals([
    tasks.DetectionModel,
    Sequential,
    Conv2d
])

# Load the face detection model
print("Loading face detection model...")
model = YOLO("yolov8n-face.pt")
print("Model loaded successfully!")

# ESP32 Stream URL
ESP32_IP = "10.43.27.241"
STREAM_URL = f"http://{ESP32_IP}:81/stream"

print(f"\nConnecting to ESP32: {ESP32_IP}")
print("=" * 50)

# Initialize Firebase
if not firebase_admin._apps:
    cred = credentials.Certificate("baby-monitoring.json")
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https:url.firebaseio.com' #change this to your firebase database url
    })
print("Firebase initialized")

# First, test basic connectivity
def test_basic_connectivity():
    """Test if ESP32 is reachable"""
    try:
        # Test ping-like connection
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        result = sock.connect_ex((ESP32_IP, 81))
        sock.close()
        
        if result == 0:
            print("Port 81 is open")
            return True
        else:
            print(f"Port 81 is closed (error code: {result})")
            return False
    except Exception as e:
        print(f"Connection test failed: {e}")
        return False

# Try different connection methods
def method1_requests_with_retry():
    """Method 1: Requests with retry strategy"""
    print("\nMethod 1: Requests with retry...")
    
    session = requests.Session()
    retries = Retry(
        total=3,
        backoff_factor=1,
        status_forcelist=[500, 502, 503, 504]
    )
    session.mount('http://', HTTPAdapter(max_retries=retries))
    
    headers = {
        'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
        'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8',
        'Accept-Language': 'en-US,en;q=0.5',
        'Accept-Encoding': 'gzip, deflate',
        'Connection': 'keep-alive',
        'Upgrade-Insecure-Requests': '1',
        'Cache-Control': 'no-cache',
        'Pragma': 'no-cache'
    }
    
    try:
        print("Testing HTTP connection...")
        test_response = session.get(f"http://{ESP32_IP}:81/", timeout=5, headers=headers)
        print(f"ESP32 web server responded with status: {test_response.status_code}")
        
        print("Requesting stream...")
        response = session.get(STREAM_URL, stream=True, timeout=10, headers=headers)
        
        if response.status_code == 200:
            print("Stream connected!")
            print(f"Content-Type: {response.headers.get('Content-Type', 'unknown')}")
            return response
        else:
            print(f"HTTP {response.status_code}")
            
    except Exception as e:
        print(f"Method 1 failed: {e}")
    
    return None

def method2_simple_socket():
    """Method 2: Direct socket connection (low-level)"""
    print("\nMethod 2: Direct socket connection...")
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect((ESP32_IP, 81))
        
        request = f"GET /stream HTTP/1.1\r\nHost: {ESP32_IP}\r\nUser-Agent: Mozilla/5.0\r\nConnection: keep-alive\r\nAccept: */*\r\n\r\n"
        sock.send(request.encode())
        
        response = b""
        while b"\r\n\r\n" not in response:
            chunk = sock.recv(1024)
            if not chunk:
                break
            response += chunk
        
        print(f"Socket connected!")
        print(f"Response preview: {response[:100].decode('utf-8', errors='ignore')}")
        
        return sock, response
        
    except Exception as e:
        print(f"Socket method failed: {e}")
        return None, None

def method3_opencv_specific():
    """Method 3: OpenCV with ESP32-specific settings"""
    print("\nMethod 3: OpenCV ESP32 mode...")
    
    cap = cv2.VideoCapture(STREAM_URL, cv2.CAP_FFMPEG)
    
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    cap.set(cv2.CAP_PROP_OPEN_TIMEOUT_MSEC, 5000)
    cap.set(cv2.CAP_PROP_READ_TIMEOUT_MSEC, 5000)
    
    retries = 3
    for i in range(retries):
        print(f"Attempt {i+1} to read frame...")
        ret, frame = cap.read()
        if ret:
            print(f"OpenCV success! Frame shape: {frame.shape}")
            return cap
    
    cap.release()
    print("OpenCV method failed")
    return None

# Try all methods
if not test_basic_connectivity():
    print("\nESP32 not responding on port 81")
    print("Troubleshooting:")
    print("1. Check if ESP32 is powered on")
    print("2. Verify IP address is correct")
    print("3. Check if both devices are on same network")
    print("4. Try pinging the ESP32: ping 10.43.27.241")
    exit()

stream_source = None

response = method1_requests_with_retry()
if response:
    print("\nUsing Method 1: Requests stream")
    stream_source = "requests"

if not stream_source:
    sock, headers = method2_simple_socket()
    if sock:
        print("\nUsing Method 2: Socket stream")
        stream_source = "socket"

if not stream_source:
    cap = method3_opencv_specific()
    if cap:
        print("\nUsing Method 3: OpenCV stream")
        stream_source = "opencv"

if not stream_source:
    print("\nAll connection methods failed")
    print("\nFinal troubleshooting:")
    print("1. Restart your ESP32")
    print("2. Check ESP32 serial monitor for errors")
    print("3. Try accessing in browser: http://10.43.27.241:81/stream")
    print("4. Disable firewall temporarily")
    exit()

print("\n🎥 Starting face detection...")
print("Press 'q' to quit\n")

frame_count = 0

if stream_source == "requests":
    bytes_buffer = bytes()
    for chunk in response.iter_content(chunk_size=1024):
        bytes_buffer += chunk
        
        start = bytes_buffer.find(b'\xff\xd8')
        end = bytes_buffer.find(b'\xff\xd9')
        
        if start != -1 and end != -1 and end > start:
            jpg_frame = bytes_buffer[start:end+2]
            bytes_buffer = bytes_buffer[end+2:]
            
            frame = cv2.imdecode(np.frombuffer(jpg_frame, dtype=np.uint8), cv2.IMREAD_COLOR)
            
            if frame is not None:
                frame_count += 1
                results = model(frame, conf=0.25, verbose=False)
                annotated_frame = results[0].plot()
                
                face_count = len(results[0].boxes)
                
                # Firebase update
                ref = db.reference('realtime_video/baby_presence')
                ref.set(1 if face_count > 0 else 0)
                
                cv2.putText(annotated_frame, f"Faces: {face_count}", (10, 30),
                           cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
                
                cv2.imshow('ESP32 Face Detection', annotated_frame)
                
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    break

elif stream_source == "opencv" and cap:
    while True:
        ret, frame = cap.read()
        if not ret:
            print("Stream lost, reconnecting...")
            break
            
        frame_count += 1
        
        if frame_count % 2 == 0:
            results = model(frame, conf=0.25, verbose=False)
            annotated_frame = results[0].plot()
            
            face_count = len(results[0].boxes)
            
            # Firebase update
            ref = db.reference('realtime_video/baby_presence')
            ref.set(1 if face_count > 0 else 0)
        else:
            annotated_frame = frame
        
        cv2.imshow('ESP32 Face Detection', annotated_frame)
        
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    
    if 'cap' in locals():
        cap.release()

cv2.destroyAllWindows()
print("\nFace detection stopped")