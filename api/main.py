from fastapi import FastAPI, File, UploadFile, HTTPException
from fastapi.responses import JSONResponse
from fastapi.middleware.cors import CORSMiddleware
import io
import os
import time
import json
from PIL import Image
import numpy as np
import torch
from ultralytics import YOLO

app = FastAPI(title="ESP32-CAM YOLO Object Detection")

# Add CORS middleware
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # Allow all origins
    allow_credentials=True,
    allow_methods=["*"],  # Allow all methods
    allow_headers=["*"],  # Allow all headers
)

# Global variable to store our model
model = None

def get_model():
    """Load or download the YOLO model."""
    global model
    if model is None:
        # Path to weights file or use pre-trained model
        model = YOLO("yolov8n.pt")  # Using YOLOv8 nano model
    return model

@app.get("/")
async def root():
    return {"message": "ESP32-CAM YOLO Object Detection API is running!"}

@app.post("/detect")
async def detect_objects(file: UploadFile = File(...)):
    """
    Endpoint for object detection.
    Accepts an image file and returns detection results.
    """
    start_time = time.time()
    
    # Input validation
    if not file.content_type.startswith("image/"):
        raise HTTPException(status_code=400, detail="File must be an image")
    
    try:
        # Read image data
        image_data = await file.read()
        image = Image.open(io.BytesIO(image_data))
        
        # Get model
        model = get_model()
        
        # Run inference
        results = model(image)
        
        # Parse results
        detections = []
        result = results[0]  # First image result
        
        boxes = result.boxes
        for box in boxes:
            # Get box coordinates
            x1, y1, x2, y2 = box.xyxy[0].tolist()
            
            # Get confidence
            confidence = float(box.conf[0])
            
            # Get class
            class_id = int(box.cls[0])
            class_name = result.names[class_id]
            
            detections.append({
                "class": class_name,
                "confidence": round(confidence, 3),
                "box": {
                    "x1": round(x1),
                    "y1": round(y1),
                    "x2": round(x2),
                    "y2": round(y2)
                }
            })
        
        # Calculate processing time
        process_time = time.time() - start_time
        
        # Return results
        return JSONResponse(content={
            "success": True,
            "processing_time": round(process_time, 3),
            "detections": detections
        })
            
    except Exception as e:
        raise HTTPException(status_code=500, detail=f"Error processing image: {str(e)}")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)