#/usr/bin/python
import pigpio
address=0x08

pi = pigpio.pi()
h = pi.i2c_open(1, address)

pi.i2c_write_device(h, [0x01])
(count, data) = pi.i2c_read_device(h, 2)

if count >= 2:
    raw_temperature = data[1] << 8
    raw_temperature |= data[0]

    print str(raw_temperature/10.0)
else:
    print "Error, not enough bytes read"

