# The Fake Hackers - Music Hack
By Genesis Argueta, Manuela Roca, and Emmanuel De La Paz

## Project Details
For our first project, we decided to build a music box program for DOSBox that can generate and play music in two ways:
1. Interactive Mode – Users can play music live using their computer keyboard. Multiple notes can be held at once, allowing for simple polyphony. An ASCII “oscilloscope” provides real-time visual feedback of the sound.

2. Sheet Playback Mode – The program can also read in a text file and interpret it as a score. The sheet format supports notes, rests, chords, tempo, instrument selection, sustain pedal, and overlaps, which the program translates into MIDI playback with ASCII visualization.

To simulate working in a resource-constrained environment, the project runs inside a virtual machine (DOSBox). MIDI (a digital instruction set for music) is used to control sound output, recreating the feel of early computer music experiments.

## Computing History
 We were inspired by Peter Samson’s Samson Box and Steven Dompier’s Music Machine. Samson and Dompier are considered pioneers of computer music because they applied algorithmic and generative techniques to create digital music. Their innovations were crucial in shifting public perception of computers by showing that these machines could be used not only for business or scientific calculations, but also for creative expression. Our project aims to emulate this spirit.

 After finishing Hackers, we wanted to continue that spirit of playful creativity. All of us have musical backgrounds: Manuela plays piano and violin, Genesis played violin, and Emmanuel plays trumpet, percussion, and steel drums, so combining music and programming felt natural.

## Importance of Historical Artifact
The Samson Box and Music Machine were important because they introduced the idea of using computers as tools for art. At the time, computers were reserved for “serious computations,” and creating music with them was seen as trivial or wasteful. Today, however, computers are central to creative work and they are used in music production, digital art, animation, film editing, and more. By revisiting these early experiments, we recognize how they laid the foundation for the creative uses of computing that we now take for granted.

## Features
- MIDI playback via MPU-401 UART (DOSBox must be configured with mpu401=uart).
- Two modes: interactive keyboard input or text-based sheet playback.
- Sheet notation support for:
    Notes, chords, rests
    Beat units (quarter, eighth, half, whole, sixteenth, dotted quarter)
    Tempo (T=###), instrument (I=###), sustain (SUS=ON|OFF), overlap (OVL=ms)
    ASCII oscilloscope that displays averaged note frequencies.

## How to Set Up

#### 1. Local setup
Set up directory and music program in that directory.

    `mkdir dosproj`

    `cd dosproj`

    `vim music.c`

compile your music program using watcom: https://www.openwatcom.org/ 

    `wcl music.c -fe=music.exe`

### 2. Setting up DOSBox (scary)
1.  First make sure to download DOSBox from this scary website: https://www.dosbox.com/download.php?main=1
2. Once DOSBox is downloaded, you must mount the directory where the executable is located. Below is example from using student machine.

    `MOUNT C /Users/<your-username>/path_to_dir`

3.  Move executable from local to DOSBox. 
4. For MIDI playback, ensure DOSBox’s configuration file includes:

    `mpu401=uart`

    `mididevice=default`

5. Then you run the executable on the DOSBox machine. 
- Interactive mode: run music.exe with no arguments and use your keyboard to play notes.
- Sheet mode: provide a text file as input: 

    `music.exe song.txt`
