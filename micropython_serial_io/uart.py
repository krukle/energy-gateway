from machine import UART
import random
import time

uart = UART(2, 15200, bits=8, parity=None, stop=1)
uart.init(15200, bits=8, parity=None, stop=1)

while True:
    data = random.randint(0, 255)
    print(f"Sending data: {data}")
    uart.write(data.to_bytes(1, "big"))
    received = uart.read()
    if (received):
        print(f"Data received: {received}")
    else:
        print("No data received")
    time.sleep(10)