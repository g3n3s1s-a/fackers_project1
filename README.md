# The Fake Hackers - Music Hack
By Genesis Argueta, Manuela Roca, and Emmanuel De La Paz

## Project Details
For our first project, we decided to build a music box program that can generate and play music in real time. Users will be able to choose a scale and the duration of the piece. To simulate working in a resource-constrained environment, we are running the program inside a virtual machine (DOSBox), which limits the resources available. Our project uses MIDI (a set of digital instructions for music) to recreate the core ideas of early computer music experiments.

## Computing History
 We were inspired by Peter Samson’s Samson Box and Steven Dompier’s Music Machine. Samson and Dompier are considered pioneers of computer music because they applied algorithmic and generative techniques to create digital music. Their innovations were crucial in shifting public perception of computers by showing that these machines could be used not only for business or scientific calculations, but also for creative expression. Our project aims to emulate this spirit.

## Importance of Historical Artifact
The Samson Box and Music Machine were important because they introduced the idea of using computers as tools for art. At the time, computers were reserved for “serious computations,” and creating music with them was seen as trivial or wasteful. Today, however, computers are central to creative work and they are used in music production, digital art, animation, film editing, and more. By revisiting these early experiments, we recognize how they laid the foundation for the creative uses of computing that we now take for granted.

## How to Set Up

#### 1. Local setup
Set up directory and music program in that directory.

`mkdir dosproj`

`cd dosproj`

`vim music.c`

compile your music program using watcom: https://www.openwatcom.org/ 

### 2. Setting up DOSBox (scary)
- First make sure to download DOSBox from this scary website: https://www.dosbox.com/download.php?main=1
- Once DOSBox is downloaded, you must mount the directory where the executable is located. Below is example from using student machine.
 `MOUNT C /Users/<your-username>/path_to_dir`
- Move executable from local to DOSBox. 
- Then you run the executable on the DOSBox machine. 

