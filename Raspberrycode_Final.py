import cv2
import time
import serial
import threading
import RPi.GPIO as GPIO
from ultralytics import YOLO

# ---------------- GPIO ----------------
RED_LED = 17
ORANGE_LED = 27

GPIO.setmode(GPIO.BCM)
GPIO.setup([RED_LED, ORANGE_LED], GPIO.OUT)

# ---------------- UART ----------------
esp_uart = serial.Serial('/dev/serial0', 115200, timeout=1)

# ---------------- ANIMALS ----------------
WILD = {"bird", "bear", "elephant", "zebra", "giraffe"}
DOM = {"cat", "dog", "horse", "sheep", "cow"}

# ---------------- CAMERA ----------------
class Cam:
    def __init__(self):
        self.cap = cv2.VideoCapture(0)
        self.ret, self.frame = self.cap.read()
        self.running = True

    def start(self):
        threading.Thread(target=self.update, daemon=True).start()
        return self

    def update(self):
        while self.running:
            self.ret, self.frame = self.cap.read()

    def read(self):
        return self.frame

# ---------------- LED ----------------
def blink(pin):
    GPIO.output(pin, True)
    time.sleep(0.2)
    GPIO.output(pin, False)

# ---------------- UART SEND ----------------
def send(msg):
    esp_uart.write((msg + "\n").encode())
    print("TX:", msg)

# ---------------- MAIN ----------------
model = YOLO("yolov8n.pt").to("cpu")
cam = Cam().start()

last_time = time.time()

print("PI STARTED")

while True:
    frame = cam.read()
    if frame is None:
        continue

    results = model(frame, imgsz=160, verbose=False)

    detected = False

    for r in results:
        for b in r.boxes:
            label = model.names[int(b.cls[0])]
            conf = float(b.conf[0])

            if conf > 0.4 and (label in WILD or label in DOM):
                detected = True
                last_time = time.time()

                # --- DRAWING BOX AND ACCURACY ---
                # Get coordinates (x1, y1, x2, y2)
                x1, y1, x2, y2 = map(int, b.xyxy[0])
                
                # Pick color: Red for Wild, Orange (using yellow here) for Dom
                color = (0, 0, 255) if label in WILD else (0, 165, 255)
                
                # Draw the bounding box
                cv2.rectangle(frame, (x1, y1), (x2, y2), color, 2)
                
                # Add text (Label + Accuracy)
                text = f"{label} {conf:.2f}"
                cv2.putText(frame, text, (x1, y1 - 10), 
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)
                # -------------------------------

                cat = "WILD" if label in WILD else "DOM"
                msg = f"DETECT|{cat}|{label}|{conf:.2f}"
                send(msg)

                blink(RED_LED if cat == "WILD" else ORANGE_LED)

    # heartbeat
    if time.time() - last_time > 3:
        send("NORMAL|None|None|0")
        last_time = time.time()

    cv2.imshow("PI", frame)
    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

GPIO.cleanup() 
cv2.destroyAllWindows()