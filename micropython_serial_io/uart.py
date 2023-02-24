from machine import UART
import time

uart = UART(2, 15200, bits=8, parity=None, stop=1)
uart.init(15200, bits=8, parity=None, stop=1)

while True:
    print("Sending data")
    uart.write("Hello World")
    received = uart.read()
    if (received):
        print(f"Data received: {received}")
    else:
        print("No data received")
    time.sleep(10)