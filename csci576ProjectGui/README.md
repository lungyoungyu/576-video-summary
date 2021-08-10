# 576-video-summary GUI

## Dependencies

### QT Creator and OpenCV
This package uses [QT Creator and OpenCV](https://wiki.qt.io/How_to_setup_Qt_and_openCV_on_Windows) to display images and for CV algorithms. Please first build and install both QT Creator OpenCV on your machine to view the GUI.

## Build instructions

When first opening the project (which the .pro file should open the project, if not select the directory within the QT Creator IDE) it may prompt to select a build directory, which has been included here. Otherwise select your preferred location. Make sure the dependency paths are resolved by editing the .pro file. Most important is the SFML dependency.

### SFML
This [guide](https://github.com/SFML/SFML/wiki/Tutorial:-Compile-and-Link-SFML-with-Qt-Creator) shows how to install SFML for use within QT Creator to provide functionality to play wav files. IT IS IMPORTANT TO USE THE SAME COMPILER TO COMPILE SFML AS THE ONE USED TO COMPILE THE PROJECT/APPLICATION (VERSION AND BIT), so if following the above guide for QT Creator, USE MinGW 5.3.0 32-bit to compile the SFML source code. The SFML files used for this project is also provided in the repo</br>

## FFMPEG
This package uses [FFmpeg](https://ffmpeg.org/download.html) to concat wav files and add them to video. This [post](https://stackoverflow.com/questions/49410123/showing-cmd-terminal-in-qt-widgets-application) shows how to execute cmd commands from a QT project.

## Usage instructions

### Create Video
Opens a directory select dialog box to select a directory full of .jpg frames to compile into a .avi, then a file select dialog box to select a .wav. The program will then compile the frames into a full .avi video and add the .wav audio on top of it.

### Open File
Opens a .avi file to play in the video player.

### Summarize
Opens a directory select dialog box to select a directory full of .jpg frames to compile into a .avi, then a file select dialog box to select a .wav like create video but summarizes based on scene motion and scene audio into a 90-120 second video. 

Implementation Notes
- Audio Scores:
  - Set up audio buffer - split to left and right buffer (stereo)
  - Calculate Audio Sample Values for each frame by summing absolute values of sample values.
    - Number of audio samples per frame = sample rate of original buffer / frame rate (30 fps)
  - Normalize Audio Sample Values for each frame by dividing by total number of frames in corresponding shot.
  - Normalize Audio Sample Values again for each shot to between 0 and 100 via normalized value = (audio value - min audio) / (max audio - min audio) * 100 to get audio score for each shot
- Motion Scores + Audio Scores = Total weighted score for each shot
- Sort shots by total weighted score
- Starting from shot with highest weighted score, add shots as long as not exceeded 90 sec (if last shot, results in longer than 90 sec, that is fine)
- Sort shots by order they appear in original video.
- Resulting order of jpg frames in 90 sec summary video.
