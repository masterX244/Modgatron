#!/usr/bin/python3
import soundfile as sf
import samplerate
import sys
import random
soundfile = sys.argv[1]
data_in, datasamplerate = sf.read(soundfile)
# This means stereo so extract one channel 0
if len(data_in.shape)>1:
    data_in = data_in[:,0]

ran = random.randrange(10 ** 80)
prefix = "%064x" % ran
prefix = "S_"+prefix[:32]

converter = 'sinc_best'  # or 'sinc_fastest', ...
desired_sample_rate = 11000.0
ratio = desired_sample_rate/datasamplerate
data_out = samplerate.resample(data_in, ratio, converter)
print(data_out)
maxValue = max(data_out)
minValue = min(data_out)
print("length", len(data_out))
print("max value", max(data_out))
print("min value", min(data_out))
vrange = (maxValue - minValue) 
print("value range", vrange)


m68code = "/*    File "+soundfile+ "\r\n *    Sample rate "+str(int(desired_sample_rate)) +" Hz\r\n */\r\n"
m68code += "#define WAV_DATA_LENGTH "+str(len(data_out))+" \r\n\r\n"
m68code += "uint8_t WAV_DATA[] = {\r\n    "

m68code="""
#include <pico.h> //was pico/platform.h
extern "C"{
    #include "pico-pwm-audio.h"
}
#define """+prefix+"""_WAV_DATA_LENGTH """+str(len(data_out))+"""

const char __in_flash() """+prefix+"""_LABEL[] = " """+soundfile+"""";

const uint8_t __in_flash() """+prefix+"""_WAV_DATA[] = {

"""
maxitemsperline = 16
itemsonline = maxitemsperline
firstvalue = 0
lastvalue = 0
for v in data_out:
    # scale v to between 0 and 1
    isin = (v-minValue)/vrange   
    v =  int((isin * 255))
    vstr = str(v)
    if (firstvalue==0):
        firstvalue= v
    lastvalue = v
    m68code+=vstr
    itemsonline-=1
    if (itemsonline>0):
        m68code+=','
    else:
        itemsonline = maxitemsperline
        m68code+=',\r\n    '
        
# keep track of first and last values to avoid
# blip when the loop restarts.. make the end value
# the average of the first and last. 
end_value = int( (firstvalue + lastvalue) / 2)
m68code+=str(end_value)+'    \r\n};'
m68code += """


void """+prefix+"""_magic() __attribute__ ((constructor));
 
void """+prefix+"""_magic() {
    registerSample((uint8_t*)&"""+prefix+"""_WAV_DATA,"""+prefix+"""_WAV_DATA_LENGTH,(char*)&"""+prefix+"""_LABEL,false);
}
"""
#print(m68code)
with open(soundfile+".cpp", "w") as text_file:
    text_file.write(m68code)    