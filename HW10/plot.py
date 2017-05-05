import matplotlib.pyplot as plt
import serial

ser = serial.Serial('/dev/ttyACM0', 9600)
ser.write("r")

raw = []
maf = []
iir = []
fir = []

for i in range(100):
    line = ser.readline()
    data = line.split()
    raw.append(data[1])
    maf.append(data[2])
    iir.append(data[3])
    fir.append(data[4])

plt.plot(raw, label = 'raw')
plt.plot(maf, label = 'maf')
plt.plot(iir, label = 'iir')
plt.plot(fir, label = 'fir')
plt.legend()
plt.show()
