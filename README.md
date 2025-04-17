# ESP32-CAM YOLO Object Detection with Vercel

This project implements a real-time object detection system using an ESP32-CAM, a YOLO model, and Vercel serverless functions.

## System Architecture

1. ESP32-CAM captures images and sends them via HTTP POST to a Vercel-hosted API
2. The API processes the image with a YOLOv8 model for object detection
3. Detection results are returned to the ESP32-CAM for further processing or display

## Setup Instructions

### 1. Deploy the API to Vercel

1. Fork or clone this repository
2. Connect your GitHub repository to Vercel
3. Deploy the project - Vercel will automatically detect the Python FastAPI application

### 2. Program the ESP32-CAM

1. Install the Arduino IDE and ESP32 board support
2. Install required libraries:
   - ESP32 Arduino Core
   - ArduinoJson
3. Open the `esp32_code/esp32_cam_yolo.ino` file
4. Update the WiFi credentials and API endpoint URL with your Vercel deployment URL
5. Upload the code to your ESP32-CAM

### 3. Hardware Setup

Connect the ESP32-CAM to a power source and ensure it has a clear view of the area you want to monitor.

## Configuration Options

### Camera Settings

You can modify the camera resolution in the ESP32 code by changing the `config.frame_size` parameter:
- `FRAMESIZE_QQVGA` (160x120) - Faster processing, less detail
- `FRAMESIZE_QVGA` (320x240) - Balanced option
- `FRAMESIZE_VGA` (640x480) - More detail, slower processing

### YOLO Model

You can change the YOLO model in `api/main.py` by modifying:
```python
model = YOLO("yolov8n.pt")  # nano model
```

Options include:
- `yolov8n.pt` - Smallest and fastest (recommended for Vercel)
- `yolov8s.pt` - Small model, better accuracy
- `yolov8m.pt` - Medium model, may exceed Vercel limits

## Troubleshooting

- **HTTP Error 413**: Reduce image resolution or quality in ESP32 code
- **Server Timeout**: Check Vercel function execution limits
- **ESP32 Connection Issues**: Ensure WiFi credentials are correct and signal is strong

## License

[MIT License](LICENSE)