import time
import serial
from PIL import Image

PORT_NAME = 'COM10'
BAUD_RATE = 115200

try:
    ser = serial.Serial(PORT_NAME, BAUD_RATE, timeout=1)
    time.sleep(2)
    
    image = Image.open(r"C:\Repositories\pico\mcu\05-display\акула.jpg")
    width, height = image.size
    
    rgb_image = image.convert('RGB')
    
    for y in range(height):
        for x in range(width):
            r, g, b = rgb_image.getpixel((x, y))
            color_hex = (r << 16) | (g << 8) | b
            
            command = f"disp_px {x} {y} {color_hex:06X}\n"
            ser.write(command.encode('utf-8'))
            
except Exception as e:
    print(f"Error: {e}")
    
finally:
    time.sleep(0.1)
    if 'ser' in locals() and ser.is_open:
        ser.close()
