import time
import serial
import matplotlib.pyplot as plt

def read_values(ser):
	while True:
		try:
			line = ser.readline().decode('ascii').strip()
			if not line:
				continue
			t, p, h = map(float, line.split())
			return t, p, h
		except ValueError:
			continue

def main():
	ser = serial.Serial(port='COM10', baudrate=115200, timeout=1.0)

	if not ser.is_open:
		print("Port closed")
		return

	measure_t = []
	measure_p = []
	measure_h = []
	measure_ts = []
	start_ts = time.time()

	try:
		while True:
			ts = time.time() - start_ts

			ser.write("get_all\n".encode('ascii'))
			t, p, h = read_values(ser)

			measure_ts.append(ts)
			measure_t.append(t)
			measure_p.append(p)
			measure_h.append(h)

			print(f'{ts:.1f}s | T: {t:.2f}°C | P: {p:.1f} Pa | H: {h:.1f}%')
			time.sleep(0.2)

	finally:
		ser.close()

		if measure_ts:
			plt.figure(figsize=(10, 8))
			
			plt.subplot(3, 1, 1)
			plt.plot(measure_ts, measure_t, color='red')
			plt.title('График зависимости температуры от времени')
			plt.ylabel('Температура, °C')
			plt.grid(True)

			plt.subplot(3, 1, 2)
			plt.plot(measure_ts, measure_p, color='blue')
			plt.title('График зависимости давления от времени')
			plt.ylabel('Давление, Па')
			plt.grid(True)

			plt.subplot(3, 1, 3)
			plt.plot(measure_ts, measure_h, color='green')
			plt.title('График зависимости влажности от времени')
			plt.xlabel('Время, с')
			plt.ylabel('Влажность, %')
			plt.grid(True)

			plt.tight_layout()
			plt.show()

if __name__ == "__main__":
	main()
