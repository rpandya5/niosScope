# FPGA-Based Oscilloscope on the DE1-SoC

We've created a complete oscilloscope implementation on the **DE1-SoC FPGA board (Revision F)** running **Nios II**. Our oscilloscope captures analog inputs through the onboard **LTC2308 ADC** and processes them to provide real-time waveform display and measurements, including **frequency, amplitude, and DC offset**. The system features **trigger functionality** for stable waveform capture and uses **advanced signal processing techniques** to ensure accurate measurements.

Demo on CPULator:
<div style="display: flex; justify-content: center; gap: 10px;">
    <img src=images/"With_trigger.png" alt="Static Image" width="45%">
    <img src=images/"withouttrigger.gif" alt="GIF Animation" width="45%">
</div>
Left image (without trigger) | Right image (with trigger)
Full demonstration on De1-Soc Board - https://youtu.be/HqvTaowTBj0



---

## System Architecture

Our oscilloscope system consists of several interconnected subsystems that handle different aspects of signal acquisition, processing, and display:

### Signal Acquisition System

- The analog signal connects to **Channel 0** of the **LTC2308 ADC**, which provides **12-bit resolution (0-4095 values)** with a **5V reference voltage**.
- We implemented an **interrupt-driven sampling system** using the **Nios II timer peripheral** to generate precise sampling intervals.
- The sampling system operates at a **configurable frequency** (adjustable via pushbuttons) and stores samples in a **circular buffer of 80 samples**.
- The **memory-mapped address for the ADC peripheral** is `0xFF204000`, which we access directly for fast sampling.
- The system can theoretically sample at rates up to **500kHz** (LTC2308 max rate), though we limit this in practice to ensure stability.

### Signal Processing

#### Goertzel Algorithm for Frequency Detection
- We chose **Goertzel** over FFT because it is more **efficient for targeted frequency detection**, ideal for our limited computational resources.
- The implementation divides the **frequency spectrum into 256 bins** and identifies the dominant frequency.

#### Hamming Window for Accuracy
- We apply a **Hamming window** to reduce spectral leakage, improving frequency measurement accuracy.

#### Measurement Calculations
Using the frequency from Goertzel, we derive:
- **Period** (inverse of frequency)
- **Amplitude** (half of peak-to-peak voltage)
- **DC offset** (waveform's center point)
- **Peak-to-peak voltage** (max-min values)

### Trigger System

- Synchronizes waveform display to a **stable reference point**.
- The **trigger level is adjustable** via DE1-SoC pushbuttons.
- A **visual indicator (red line)** shows the trigger level on screen.
- Supports **rising-edge and falling-edge trigger detection** for versatile signal capture.

### Display System (VGA Output: 320×240)

- **Background Grid:** Major/minor divisions for signal visualization.
- **Waveform Rendering:** Uses **sinc interpolation** instead of simple point-to-point connections for smoother waveform representation.
- **Real-time Measurements Displayed:**
  - Frequency (Hz/kHz)
  - Amplitude (V)
  - Period (s/ms/μs)
  - DC offset (V)

### User Interface (Pushbuttons)

| Button  | Function |
|---------|----------|
| `KEY0`  | Increase sampling rate |
| `KEY1`  | Decrease sampling rate |
| `KEY2`  | Increase trigger level |
| `KEY3`  | Decrease trigger level |

---

## File Structure

```
old_arduino_builds/
├── fully_integrated_main.c  # Entire implementation in one file
├── simulated_main.c  # Optimized version for CPUlator simulation  
│
├── main.c  # Main program loop & initialization  
├── measurements.c  # Signal analysis & measurement calculations  
├── graphics.c  # Display rendering & VGA interface  
├── logic.c  # Trigger system & control logic 
└── testing_files/  
   ├── measurements_test.c  # Test functions for measurement accuracy  
   └── goertzel_test.c  # Isolated Goertzel algorithm test  
```

### File Structure
**1. **Run `fully_integrated_main.c` on the FPGA**  
   - This version contains everything in one file for easy deployment.  
   - Just compile and load it onto the DE1-SoC board.
   - Use pushbuttons to adjust the **sampling rate & trigger level**.
     
2. **Run `simulated_main.c` in [CPUlator (De1-SoC Nios II Simulator)](https://cpulator.01xz.net/?sys=nios2-de1soc)**  
   - This version is optimized for the **CPUlator Nios II simulator**.  
   - It uses **predefined points** to simulate different waveforms.
   - Use simulated pushbuttons to control the oscilloscope.

3. **Compile and link individual modules**  
   - Download and compile the following files together:  
     - `main.c`
     - `measurements.c`
     - `graphics.c`
     - `logic.c`

4. **Use the testing files to validate algorithms**  
   - `measurements_test.c` is used to verify measurement calculations.  
   - `goertzel_test.c` isolates and tests the Goertzel frequency detection algorithm.  
   - These can be compiled and run separately to debug specific components.  
---

## Technical Deep Dive

### Sinc Interpolation (Waveform Display)
- **Why?** Traditional point-to-point rendering causes artifacts; sinc interpolation reconstructs a **smooth, accurate** waveform.
- **How?** Each sample point contributes a **shifted sinc function** to the display buffer.
- **Optimization:** We pre-calculate sinc values in a lookup table for real-time performance.

### Goertzel Algorithm (Frequency Analysis)
- **Why?** FFT is computationally expensive; Goertzel **efficiently** computes individual frequency components.
- **Implementation Steps:**
  1. **Divide spectrum into 10 bins**
  2. **Apply Hamming window**
  3. **Process each bin using Goertzel recursion**
  4. **Identify the bin with max power**
  5. **Extract dominant frequency**

### Trigger System Logic
- Continuously scans buffer for **threshold crossings**.
- Stores crossing point as **waveform start** for stable display.
- Supports **adjustable trigger levels** with **on-screen visualization**.

---

## Additional Resources
- [DE1-SoC Computer System with Nios II Documentation](https://www.intel.com/content/www/us/en/programmable/products/boards_and_kits/altera/kit-dk-de1-soc.html)
- [LTC2308 ADC Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/2308fa.pdf)
- [Implementing the Goertzel Algorithm on Embedded Devices](https://www.embedded.com/the-goertzel-algorithm/)
- [More on Goertzel Filter in DSP](https://www.mstarlabs.com/dsp/goertzel/goertzel.html)

---

## Team

This project was developed by [Richa Pandya](richapandya.com) and [Robert Saab](https://www.linkedin.com/in/robert-saab/) as part of our final project for ECE243 at the University of Toronto.

