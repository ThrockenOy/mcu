import time
import serial
import matplotlib.pyplot as plt

def read_value(ser):
    while True:
        try:
            line = ser.readline().decode('ascii').strip()
            if not line:
                continue
            value = float(line)
            return value
        except ValueError:
            continue

def main():
    ser = serial.Serial(port='COM10', baudrate=115200, timeout=1.0)
    
    if ser.is_open:
        print(f"Port {ser.name} opened")
    else:
        print(f"Port {ser.name} closed")
        return

    measure_temperature_C = []
    measure_voltage_V = []
    measure_ts = []
    start_ts = time.time()

    try:
        while True:
            ts = time.time() - start_ts

            # Теперь 'ser' виден, так как мы внутри функции main
            ser.write("get_adc\n".encode('ascii'))
            voltage_V = read_value(ser)
            
            ser.write("get_temp\n".encode('ascii'))
            temp_C = read_value(ser)
            
            measure_ts.append(ts)
            measure_voltage_V.append(voltage_V)
            measure_temperature_C.append(temp_C)
            
            print(f'{voltage_V:.3f} V - {temp_C:.1f}C - {ts:.2f}s')
            time.sleep(0.1)

    finally:
        ser.close()
        
        if measure_ts:
            plt.figure(figsize=(10, 8))
            plt.subplot(2, 1, 1)
            plt.plot(measure_ts, measure_voltage_V, color='blue')
            plt.title('Зависимость напряжения от времени')
            plt.ylabel('Напряжение, В')
            plt.grid(True)

            plt.subplot(2, 1, 2)
            plt.plot(measure_ts, measure_temperature_C, color='red')
            plt.title('Зависимость температуры от времени')
            plt.xlabel('Время, с')
            plt.ylabel('Температура, C')
            plt.grid(True)

            plt.tight_layout()
            plt.show()


if __name__ == "__main__":
    main()
