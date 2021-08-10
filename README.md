# 576-video-summary

## Dependencies

### OpenCV
This package uses [OpenCV (>= 4.2)](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html) to display images and for CV algorithms. Please first build and install OpenCV on your machine.

### Boost Libraries
This package also uses [Boost C++ Libraries](https://www.boost.org/) for reading images from disk. You can install this on your machine using:</br>
```sudo apt-get install libboost-all-dev```

### SFML
This [package](https://www.sfml-dev.org/tutorials/2.5/start-linux.php) provides functionality to play wav files.</br>
```sudo apt-get install libsfml-dev```

## Build instructions
To build:</br>
```
mkdir build && cd build
cmake ..
make
```

## Usage instructions
Run the package using:
```
Include jpg files folder and .wav file in build folder.

./video_summarization
  --help                              Print help message
  -i [ --input ] arg                  Input directory containing jpeg files
  -p [ --input prefix ] arg           Prefix of input image files
  -e [ --input extension ] arg (=jpg) Extension of input image files
  -s [ --start index ] arg (=0)       Starting index of image files
  -x [ --end index ] arg (=5000)      Ending index of image files
  -f [ --Frame rate ] arg (=30)       Frame rate to play video at

```

# Shot segmentation

To run the video pre-processor using the following:
```
Include jpg files folder and .wav file in build folder.

./video_preprocessor
  --help                              Print help message
  -i [ --input ] arg                  Input directory containing jpeg files
  -p [ --input prefix ] arg           Prefix of input image files
  -e [ --input extension ] arg (=jpg) Extension of input image files
  -s [ --start index ] arg (=0)       Starting index of image files
  -x [ --end index ] arg (=5000)      Ending index of image files
  -f [ --Frame rate ] arg (=30)       Frame rate to play video at
```

This will add a ```processed_shots.csv``` in the build directory. This contains, for each shot, a starting and ending index along with a percentage of motion in that shot.

Run with: ./video_preprocessor -i JPG_FRAME_DIRECTORY/ -a AUDIO.WAV

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
