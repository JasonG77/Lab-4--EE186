# Lab-4--EE186

Written Answers:
Part 2 ADC: 

(1 Point) What are the different bit resolutions can you set the ADC? What is the maximum sampling
rate at each of these resolutions? Where did you find this information in the datasheet?

The different bit resolutions are 6, 8, 10, and 12 bit.

6-bit maximum sampling rate: 5.33 Msps

8-bit maximum sampling rate: 6.15 Msps

10-bit maximum sampling rate: 7.27 Msps

12-bit maximum sampling rate: 8.88 Msps

I found this information on page 214 of the datasheet - Section 6.3.21 (Table 81 : ADC Characteristics)

• (1 Point) Why do you need a voltage divider circuit? What would happen if the photodiode is directly
connected to the board?

We use a voltage divider because the photodiode changes resistance, not voltage. The divider turns that resistance change into a voltage the ADC can read. Without it, the ADC would just see a constant 0 or 3.3V and wouldn’t be able to measure different amounts of light.

Part 3 DAC:

(1 Point) If B = 9, what is Vout? Express your answer in terms of Vref . Use the DAC mode that
provides the best resolution available on your MCU (refer to the data sheet).

(3 Point) How does the sampling rate of the DAC affect the quality of the output waveform? What
are the differences in sound perception between sine, square, and sawtooth waves? Why do we need a
capacitor in the signal path when sending an audio signal to a speaker or earphones?
