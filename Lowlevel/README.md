# Modgatron-Lowlevel
custom audio handling based on https://github.com/rgrosset/pico-pwm-audio.git with many changes


## Building

### Make 
Build project using cmake. This requires Raspberry Pi Pico C/C++ SDK to be installed. 
```
mkdir build
cd build
cmake ..
make
```

Then copy pico-pwm-audio.uf2 to your Raspberry Pi Pico!


## The Circuit
TODO: recheck values used on the perfboard

## Using the Audito Converter Script. 

The conventer is a script based on the original jupyter notebook.

### Installing 
* First you need a working install of Python preferably a 3.x verson. To install python if you don't have it already go here https://www.python.org/downloads/

* Second you need *pip* which is the python package manager, install this using the following
```
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
python get-pip.py
```

* Install the python dependancies. These are the python libraries the script needs to run. 
```
pip install soundfile 
pip install samplerate
```

### Usage guide 
run the converter
```
./util/generateAudioHeader.py /path/to/audio.file
```

include the output file inside the CMAke file, everything else is done automatically by some C black magic
