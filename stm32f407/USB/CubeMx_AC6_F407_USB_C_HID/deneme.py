import usb.core
import usb.util

# find our device
dev = usb.core.find(idVendor=0x0483, idProduct=0x5760)
if not dev:
    print ("Cihaz Bulunamadı")
    exit(1)
#print (dev)
print ("="*50)
dev.set_configuration()
cfg = dev.get_active_configuration()
intf = cfg[(0,0)]

ep = usb.util.find_descriptor(
    intf,
    # match the first OUT endpoint
    custom_match = \
    lambda e: \
        usb.util.endpoint_direction(e.bEndpointAddress) == \
        usb.util.ENDPOINT_OUT)
if not ep:
    print ("no ep")
    exit(1)

dev.set_configuration()
print(ep.bEndpointAddress)
data = [ord('1'), ord('2'), 0, 0, 0, 0, 0, 0]
#ep.adres,[Val0,Val1,Val2,Val3,Val4,Val5,Val6,Val7], timeout
dev.write(ep.bEndpointAddress, [0x01,0x01,0x06,0x05,0x04,0x03,0x02, 0x01], 10000)

print ("Print EP")
print (ep)
print ("Print cfg")
print (cfg)
print ("Print INTF")
print (intf)
while True:
        test = dev.read(0x81, 8, 10000)
        print (test)

