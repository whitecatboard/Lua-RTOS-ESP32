# VL53L0X Python interface on Raspberry Pi

This project provides a simplified python interface on Raspberry Pi to the ST VL53L0X API (ST Microelectronics).

Patterned after the cassou/VL53L0X_rasp repository (https://github.com/cassou/VL53L0X_rasp.git)

In order to be able to share the i2c bus with other python code that uses the i2c bus, this library implements the VL53L0X platform specific i2c functions through callbacks to the python smbus interface. 

Version 1.0.2:
- Add support for TCA9548A I2C Multiplexer. Tested with https://www.adafruit.com/products/2717 breakout. (johnbryanmoore)
- Add python example using TCA9548A Multiplexer support (johnbryanmoore)

Version 1.0.1:
- Simplify build process (svanimisetti)
- Add python example that graphs the sensor output (svanimisetti)
- Update the build instructions (svanimisetti, johnbryanmoore)

Version 1.0.0:
- Add support for multiple sensors on the same bus utilizing the ST API call to change the address of the device.
- Add support for improved error checking such as I/O error detection on I2C access.

Version 0.0.9:
- initial version and only supports 1 sensor with limited error checking.

Notes on Multiple sensor support:
- In order to have multiple sensors on the same bus, you must have the shutdown pin of each sensor tied to individual GPIO's so that they can be individually enabled and the addresses set.
- Both the Adafruit and Pololu boards for VL53L0X have I2C pull ups on the board. Because of this, the number of boards that can be added will be limited to only about 5 or 6 before the pull-up becomes too strong.
- Changes to the platform and python_lib c code allow for up to 16 sensors.
- Address changes are volatile so setting the shutdown pin low or removing power will change the address back to the default 0x29.

Notes on using TCA9548A I2C Multiplexer:
- If limited on GPIO's that would be needed to set a new addresses for each sensor, using a TCA9548A I2C Multiplexer is a good option since it allows using up to 8 sensors without using GPIO's.
- The TCA9548A is also a good option if using multiple boards on the same I2C bus and the total of all the combined I2C pullups would cause the bus not to function. 
- Theoretically you can connect mutltiple TCA9548A Multiplexers, each with up to 8 sensors as long each TCA9548A has a different address. This has not been tested but should work in theory.

(Please note that while the author is an embedded software engineer, this is a first attempt at extending python and the author is by no means a python expert so any improvement suggestions are appreciated).


### Installation


### Compilation

* To build on raspberry pi, first make sure you have the right tools and development libraries:
```bash
sudo apt-get install build-essential python-dev
```

Then use following commands to clone the repository and compile:
```bash
cd your_git_directory
git clone https://github.com/johnbryanmoore/VL53L0X_rasp_python.git
cd VL53L0X_rasp_python
make
```

* In the Python directory are the following python files:

VL53L0X.py - This contains the python ctypes interface to the ST Library

VL53L0X_example.py - This example accesses a single sensor with the default address.

VL53L0X_example_livegraph.py - This example plots the distance data from a single sensor in a live graph. This example requires matplotlib. Use `sudo pip install matplotlib` to install matplotlib.

VL53L0X_multi_example.py - This example accesses 2 sensors, setting the first to address 0x2B and the second to address 0x2D. It uses GPIOs 20 and 16 connected to the shutdown pins on the 2 sensors to control sensor activation.

![VL53L0X_multi_example.py Diagram](https://raw.githubusercontent.com/johnbryanmoore/VL53L0X_rasp_python/master/VL53L0X_Mutli_Rpi3_bb.jpg "Fritzing Diagram for VL53L0X_multi_example.py")

VL53L0X_TCA9548A_example.py - This example accesses 2 sensors through a TCA9548A I2C Multiplexer with the first connected to bus 1 and the second on bus 2 on the TCA9548A.

![VL53L0X_TCA9548A_example.py Diagram](https://raw.githubusercontent.com/johnbryanmoore/VL53L0X_rasp_python/master/VL53L0X_TCA9548A_Rpi3_bb.jpg "Fritzing Diagram for VL53L0X_TCA9548A_example.py")

