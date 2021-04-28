import numpy as np

rssi = np.array([-80, -70, -50, -10, -20, -30, -40, -75, -25, -35])

windows = [
    rssi[:4],
    rssi[1:4],
    rssi[4:8],
    rssi[4:8],
    np.array([rssi[8]]),
    rssi[8:10],
    np.array([rssi[9]]),
]

for data in windows:
    print(10*np.log10(sum(10.0**(data/10))/len(data)))
