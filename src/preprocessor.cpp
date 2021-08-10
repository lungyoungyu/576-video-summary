// OpenCV includes
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
// #include <opencv2/xfeatures2d.hpp>
// Boost includes
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
// SFML includes
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
// STL includes
#include <iostream>
#include <limits.h>
#include <unordered_map>
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <boost/filesystem.hpp>
#include <unordered_set>

const int ESCAPE_CHAR = 27;
const int MATCHING_INTERVAL = 10;
const float RATIO = 0.45;
const bool DEBUG_TO_FILE = true;
const int FRAME_RATE = 30;
const float MINIMUM_SHOT_LENGTH = FRAME_RATE * 2.5;
const float AVERAGE_THRESHOLD = 0.1;

using namespace cv;
// using namespace cv::xfeatures2d;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

// Vector of shots (shot: [start frame number, end frame number])
static std::vector<std::array<int, 3>> shots;

// Hashmap of shot pairs to normalized motion scores.
static std::unordered_map<std::string, double> shotMotion = {{"",-1}};

// Array of audio sample values for each frame.
static double frameAudio[16000];

// Hashmap of shot pairs to normalized audio scores.
static std::unordered_map<std::string, double> shotAudio = {{"",-1}};

// Hashmap of shot pairs to total weighted scores.
static std::unordered_map<std::string, double> shotWeight = {{"",-1}};

// Vector of sorted shot pairs to total weighted scores.
static std::vector<std::pair<std::string, double>> sortedWeight;

// Vector of summary shots
static std::vector<std::array<int, 3>> summary_shots;

bool Parse_Arguments (  int argc, char **argv, std::string &input_directory,
                        std::string &input_prefix, std::string &input_suffix,
                        std::string &path_to_audio, size_t &start_index,
                        size_t &end_index, size_t &frame_rate );

float Feature_Matching_Score ( Mat, Mat, float );

bool Audio_Score(std::string &input_directory, std::string &path_to_audio, size_t &start_index, size_t &end_index);

bool myComparison(const std::pair<std::string,double> &a, const std::pair<std::string,double> &b)
{
       return a.second > b.second;
}

size_t split(const std::string &txt, std::vector<std::string> &strs, char ch)
{
    size_t pos = txt.find( ch );
    size_t initialPos = 0;
    strs.clear();

    // Decompose statement
    while( pos != std::string::npos ) {
        strs.push_back( txt.substr( initialPos, pos - initialPos ) );
        initialPos = pos + 1;

        pos = txt.find( ch, initialPos );
    }

    // Add the last one
    strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos + 1 ) );

    return strs.size();
}

bool copyFile(const char *SRC, const char* DEST)
{
    std::ifstream src(SRC, std::ios::binary);
    std::ofstream dest(DEST, std::ios::binary);
    dest << src.rdbuf();
    return src && dest;
}

float Calculate_Motion ( std::vector <float> matching_scores, int start_index, int end_index )
{
    // std::cout << "Retrive values from: " << start_index << " to " << end_index << std::endl;
    // std::cout << "Size of matching_scores: " << matching_scores.size () << std::endl;
    if ( end_index > matching_scores.size () )
        end_index = matching_scores.size ();
    float sum = 0;
    for ( int counter = start_index; counter < end_index - MATCHING_INTERVAL; counter++ )
        sum += matching_scores.at ( counter );
    float average = sum / ( end_index - start_index - MATCHING_INTERVAL );
    return 100 - average;
}

bool Is_Change_In_Shot ( std::vector <float> matching_scores )
{
    float sum = 0;
    // std::cout << "Size of matching_scores = " << matching_scores.size () << std::endl;
    for ( int counter = 0; counter < MATCHING_INTERVAL; counter++ )
    {
        // std::cout << matching_scores.at ( matching_scores.size() - 1 - counter ) << std::endl;
        sum += matching_scores.at ( matching_scores.size() - 1 - counter );
    }

    float average = sum / MATCHING_INTERVAL;

    if ( average < AVERAGE_THRESHOLD )
        return true;
    else
        return false;
}

int main ( int argc, char **argv )
{
    shotMotion.clear();
    shotAudio.clear();
    shotWeight.clear();

    std::string input_directory, input_prefix, input_suffix, path_to_audio;
    size_t start_index, end_index, frame_rate;
    std::ofstream histogram_file, shot_durations_file;

    // Parse arguments
    if ( Parse_Arguments ( argc, argv, input_directory,
                            input_prefix, input_suffix,
                            path_to_audio, start_index,
                            end_index, frame_rate ) == false )
    {
        std::cout << "Invalid arguments ...." << std::endl;
        return -1;
    }

/*
    int image_interval;
    if ( frame_rate > 0  )
        image_interval = ( 1.0 / frame_rate ) * 1000;
    else
        image_interval = 1;

    std::cout << "Image interval: " << image_interval << std::endl;

    sf::Music music;
    if ( music.openFromFile ( path_to_audio ) == false )
    {
        std::cout << "Could not load audio file from: " << path_to_audio << std::endl;
        return -1;
    }
    else
    {
        std::cout << "Playing music" << std::endl;
        music.play ();
    }
*/

    // IMAGE FRAME SCORES
    shot_durations_file.open ("processed_shots.csv");
    shot_durations_file << "Starting_Frame,Ending_Frame,Motion_Percentage" << std::endl;

    if ( DEBUG_TO_FILE )
    {
        histogram_file.open ( "matching_score.csv" );
        histogram_file << "Current_Frame_Number,Previous_Frame_Number,Matching_Score" << std::endl;
    }

    std::vector <float> matching_scores;
    int shot_length = 0;
    int shot_start = 0;
    int shot_end = 0;

    // Go through all images
    for ( size_t img_counter = start_index + MATCHING_INTERVAL; img_counter < end_index - MATCHING_INTERVAL; img_counter ++)
    {
        // Construct image paths
        std::string img_path_1 = input_directory + "/frame" + std::to_string ( img_counter - MATCHING_INTERVAL ) + "." + input_suffix;
        std::string img_path_2 = input_directory + "/frame" + std::to_string ( img_counter ) + "." + input_suffix;

        // Read images
        Mat img_1 = imread ( img_path_1, IMREAD_COLOR );
        Mat img_2 = imread ( img_path_2, IMREAD_COLOR );

        // std::cout << "Image 1: " << img_path_1 << std::endl;
        // std::cout << "Image 2: " << img_path_2 << std::endl;


        // Ensure images are not empty
        if ( img_1.empty ( ) || img_2.empty ( ) )
        {
            std::cout << "Could not read image at: " << img_path_1 << std::endl;
            std::cout << "Could not read image at: " << img_path_2 << std::endl;
            return -1;
        }

        // Perform feature matching
        float matching_score = Feature_Matching_Score ( img_1, img_2, MATCHING_INTERVAL );
        // std::cout << "Matching score" << " = " << matching_score << std::endl;
        matching_scores.push_back ( matching_score );

        if ( matching_scores.size () > MATCHING_INTERVAL )
        {
            if ( !Is_Change_In_Shot ( matching_scores ) )
                shot_length++;
            else
            {
                if ( shot_length >= MINIMUM_SHOT_LENGTH )
                {
                    shot_end = img_counter;
                    // std::cout << shot_start << ", " << shot_end << ","
                        // << Calculate_Motion ( matching_scores, shot_start, shot_end ) << std::endl;
                    shot_durations_file << shot_start << "," << shot_end << ","
                        << Calculate_Motion ( matching_scores, shot_start, shot_end ) << std::endl;

                    // Add to vector of shots and hashmap of shot motion values.
                    std::array<int, 3> shot = {shot_start, shot_end, static_cast<int>(shots.size())};
                    shots.push_back(shot);
                    String shot_string = std::to_string(shot_start) + " " + std::to_string(shot_end);

                    auto it = shotAudio.find(shot_string);
                    if(it != shotAudio.end()) {
                        it->second = Calculate_Motion ( matching_scores, shot_start, shot_end );
                    } else {
                        shotAudio.insert(std::make_pair(shot_string, Calculate_Motion ( matching_scores, shot_start, shot_end )));
                    }

                    shot_length = 0;
                    shot_start = img_counter;
                }
            }
        }


        if ( DEBUG_TO_FILE )
            histogram_file << img_counter << "," << img_counter - MATCHING_INTERVAL << "," << matching_score << std::endl;

        std::cout << "Frame: " << img_counter << " / " << end_index - MATCHING_INTERVAL << " = " << matching_score << " % " << std::endl;
    }

    if ( DEBUG_TO_FILE )
        histogram_file.close ();

    shot_durations_file << shot_start << "," << end_index << ","
        << Calculate_Motion ( matching_scores, shot_start, end_index ) << std::endl;
    // Add to vector of shots and hashmap of shot motion values.
    int endIndexInt = static_cast<int>(end_index);
    std::array<int, 3> shot = {shot_start, endIndexInt, static_cast<int>(shots.size())};
    shots.push_back(shot);
    std::string shot_string = std::to_string(shot_start) + " " + std::to_string(end_index);

    auto it = shotAudio.find(shot_string);
    if(it != shotAudio.end()) {
        it->second = Calculate_Motion ( matching_scores, shot_start, shot_end );
    } else {
        shotAudio.insert(std::make_pair(shot_string, Calculate_Motion ( matching_scores, shot_start, shot_end )));
    }
    // FOR AUDIO TESTING
    /*
    shots.push_back({0,2344,0});
    shots.push_back({2344,2494,1});
    shots.push_back({2494,2644,2});
    shots.push_back({2644,3657,3});
    shots.push_back({3657,4169,4});
    shots.push_back({4169,5063,5});
    shots.push_back({5063,5719,6});
    shots.push_back({5719,6145,7});
    shots.push_back({6145,6242,8});
    shots.push_back({6242,8621,9});
    shots.push_back({8621,8863,10});
    shots.push_back({8863,9073,11});
    shots.push_back({9073,9416,12});
    shots.push_back({9416,9580,13});
    shots.push_back({9580,9881,14});
    shots.push_back({9881,10690,15});
    shots.push_back({10690,11413,16});
    shots.push_back({11413,11496,17});
    shots.push_back({11496,12249,18});
    shots.push_back({12249,13024,19});
    shots.push_back({13024,13405,20});
    shots.push_back({13405,13871,21});
    shots.push_back({13871,14140,22});
    shots.push_back({14140,14500,23});
    shots.push_back({14500,14587,24});
    shots.push_back({14587,16000,25});

    shotMotion["0 2344"] = 95.1511;
    shotMotion["2344 2494"] = 94.9667;
    shotMotion["2494 2644"] = 95.589;
    shotMotion["2644 3657"] = 95.0433;
    shotMotion["3657 4169"] = 95.0522;
    shotMotion["4169 5063"] = 86.9272;
    shotMotion["5063 5719"] = 92.9175;
    shotMotion["5719 6145"] = 92.5168;
    shotMotion["6145 6242"] = 96.0576;
    shotMotion["6242 8621"] = 93.417;
    shotMotion["8621 8863"] = 90.7868;
    shotMotion["8863 9073"] = 97.898;
    shotMotion["9073 9416"] = 79.8837;
    shotMotion["9416 9580"] = 92.741;
    shotMotion["9580 9881"] = 95.6036;
    shotMotion["9881 10690"] = 0; // nan
    shotMotion["10690 11413"] = 98.3654;
    shotMotion["11413 11496"] = 98.583;
    shotMotion["11496 12249"] = 91.7143;
    shotMotion["12249 13024"] = 81.8046;
    shotMotion["13024 13405"] = 91.641;
    shotMotion["13405 13871"] = 99.0905;
    shotMotion["13871 14140"] = 89.94;
    shotMotion["14140 14500"] = 94.4024;
    shotMotion["14500 14587"] = 79.9455;
    shotMotion["14587 16000"] = 88.87;
    */

    // AUDIO SCORES
    if ( Audio_Score(input_directory, path_to_audio, start_index, end_index) == false) {
      std::cout << "Invalid argument: path_to_audio ...." << std::endl;
      return -1;
    }

    // TOTAL WEIGHTED SCORES
    for(int i=0; i < shots.size(); i++) {
      int start = shots[i][0];
      int end = shots[i][1];

      std::string shot_string (std::to_string(start) + " " + std::to_string(end));

      double motion_score = 0.0;
      auto it = shotMotion.find(shot_string);
      if(it != shotMotion.end()) {
          motion_score = shotMotion[shot_string];
      }

      double audio_score = 0.0;
      it = shotAudio.find(shot_string);
      if(it != shotAudio.end()) {
          audio_score = shotAudio[shot_string];
      }

      it = shotWeight.find(shot_string);
      if(it != shotWeight.end()) {
          it->second = motion_score + audio_score;
      } else {
          shotWeight.insert(std::make_pair(shot_string, motion_score + audio_score));
      }
    }

    for(auto i = shotWeight.begin(); i != shotWeight.end(); i++) {
      sortedWeight.push_back(std::make_pair(i->first, i->second));
    }

    // SORT SHOTS BY HIGHEST SCORE
    std::sort(sortedWeight.begin(),sortedWeight.end(),myComparison);

    int total_sec = 0;
    int index = 0;
    while (total_sec < 90) {
      std::vector<std::string> v;
      v.clear();
      split(sortedWeight[index].first, v, ' ');
      int start = std::stoi(v[0]);
      int end = std::stoi(v[1]);
      summary_shots.push_back({start, end,-1});
      index += 1;
      total_sec += (end - start + 1) / 30;
    }

    // SORT SHOTS BY ORDER IN VID
    std::sort(summary_shots.begin(), summary_shots.end());

    for(int i=0; i < summary_shots.size(); i++) {
      summary_shots[i][2] = i;
      // std::cout << std::to_string(summary_shots[i][0]) << " " << std::to_string(summary_shots[i][1]) << " " << std::to_string(summary_shots[i][2]) << std::endl;
    }

    // MOVE FILES TO NEW DIRECTORY
    /*
    fs::path dir = "";
    fs::remove_all(dir / "video");
    fs::create_directories("video");
    int copy_index = 0;
    std::unordered_set<int> repeats;

    for(int i = 0; i < summary_shots.size(); i++) {
      int start = summary_shots[i][0];
      int end = summary_shots[i][1];

      for(int j = start; j <= end; j++) {
        fs::path sourceFile = input_directory + "/frame" + std::to_string(j) + ".jpg";
        fs::path targetParent = "video";
        // auto target = targetParent / sourceFile.filename(); // sourceFile.filename() returns "sourceFile.ext".
        auto target = targetParent / ("frame" + std::to_string(copy_index) + ".jpg");

        std::unordered_set<int>::const_iterator got = repeats.find (copy_index);
        if ( got == repeats.end() )
          copy_index++;
        else
          repeats.insert(copy_index);

        try // If you want to avoid exception handling, then use the error code overload of the following functions.
        {
            fs::create_directories(targetParent); // Recursively create target directory if not existing.
            fs::copy_file(sourceFile, target, fs::copy_options::overwrite_existing);
        }
        catch (std::exception& e) // Not using fs::filesystem_error since std::bad_alloc can throw too.
        {
            std::cout << e.what();
        }
      }
    }
    */

    return 0;
}

bool Parse_Arguments ( int argc, char **argv, std::string &input_directory,
                        std::string &input_prefix, std::string &input_suffix,
                        std::string &path_to_audio, size_t &start_index,
                        size_t &end_index, size_t &frame_rate )
{
   po::options_description desc("Options");
   desc.add_options()
        ("help", "Print help message")
        ("input,i", po::value<std::string>(&input_directory)->default_value(""),
         "Input directory containing jpg files")
        ("input prefix,p", po::value<std::string>(&input_prefix)->default_value("frame"),
         "Prefix of input image files")
        ("input extension,e", po::value<std::string>(&input_suffix)->default_value("jpg"),
         "Extension of input image files")
        ("input audio file,a", po::value<std::string>(&path_to_audio)->default_value(""),
         "Path to audio file")
        ("start index,s", po::value<size_t>(&start_index)->default_value(0),
         "Starting index of image files")
        ("end index,x", po::value<size_t>(&end_index)->default_value(16000),
         "Ending index of image files")
        ("Frame rate,f", po::value<size_t>(&frame_rate)->default_value(30),
         "Frame rate to play video at");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if ( vm.count ("help") )
    {
        std::cout << desc << std::endl;
        return false;
    }

    if ( input_directory.empty () )
    {
        std::cerr << desc << std::endl;
        std::cerr << "Must specify the input image directory" << std::endl;
        return false;
    }

    boost::filesystem::path inpath ( input_directory );
    if ( !boost::filesystem::exists ( inpath ) )
    {
        std::stringstream ss;
        ss << "Unable to open " << inpath;
        throw std::runtime_error(ss.str());
    }

    if ( path_to_audio == "" )
    {
        std::cout << "Must specify path to audio file using -a" << std::endl;
        return false;
    }

    std::cout << "Input directory: " << input_directory << std::endl;
    std::cout << "Image file prefix: " << input_prefix << std::endl;
    std::cout << "Image file extension: " << input_suffix << std::endl;
    std::cout << "Path to audio file: " << path_to_audio << std::endl;
    std::cout << "Starting index: " << start_index << std::endl;
    std::cout << "Ending index: " << end_index << std::endl;
    std::cout << "Frame rate: " << frame_rate << " fps" << std::endl;

    return true;
}


float Feature_Matching_Score ( Mat img_1, Mat img_2, float image_interval )
{
    std::vector<KeyPoint> keypoints_1, keypoints_2;
    Mat descriptors_1, descriptors_2;
    // Feature detector
    Ptr<FeatureDetector> detector = ORB::create();
    // Descriptor extractor
    Ptr<DescriptorExtractor> descriptor = ORB::create();
    // Descriptor matcher
    Ptr<DescriptorMatcher> matcher  = DescriptorMatcher::create ( DescriptorMatcher::FLANNBASED );

    // Detect features
    detector->detect ( img_1,keypoints_1 );
    detector->detect ( img_2,keypoints_2 );

    // Compute descriptors
    descriptor->compute ( img_1, keypoints_1, descriptors_1 );
    descriptor->compute ( img_2, keypoints_2, descriptors_2 );

    // Convert to FLANN-compatible descriptors
    descriptors_1.convertTo ( descriptors_1, CV_32F );
    descriptors_2.convertTo ( descriptors_2, CV_32F );

    // Feature matching
    std::vector<DMatch> matches;
    // std::cout << "Matching ...";
    try
    {
        matcher->match ( descriptors_1, descriptors_2, matches);
    }
    catch ( ...)
    {
        // std::cout << "Caught an error ..." << std::endl;
        return 0;
    }
    // std::cout << "Matched" << std::endl;

    //-- Filter matches using the Lowe's ratio test

    float maxdist = 0;
    for ( unsigned int i = 0; i < matches.size(); ++i)
        maxdist = max(maxdist, matches[i].distance);

    std::vector<DMatch> good_matches;
    for (size_t i = 0; i < matches.size(); i++)
    {
        if ( matches[i].distance < maxdist*RATIO )
        {
            good_matches.push_back( matches [i] );
        }
    }

    // // Get the matching percentage
    // std::cout << good_matches.size () << std::endl;
    float matching_percentage = good_matches.size() * 100.0f / keypoints_1.size ();

    // Mat img_match;
    Mat img_goodmatch;
    // drawMatches ( img_1, keypoints_1, img_2, keypoints_2, matches, img_match );
    // drawMatches ( img_1, keypoints_1, img_2, keypoints_2, good_matches, img_goodmatch );
    // imshow ( "ORB-Feature Matching", img_match );
    // imshow ( "ORB-Feature Good Matches", img_goodmatch );
    // waitKey( image_interval );
    return matching_percentage;

}

bool Audio_Score(std::string &input_directory, std::string &path_to_audio, size_t &start_index, size_t &end_index) {
  fs::path dir = "";
  fs::remove_all(dir / "audio");
  fs::create_directories("audio");

  sf::SoundBuffer originalBuffer;
  sf::SoundBuffer newAudioBuffer;
  if ( originalBuffer.loadFromFile( path_to_audio ) == false )
  {
      std::cout << "Could not load audio file from: " << path_to_audio << std::endl;
      return false;
  }
  else
  {
      // Sound: 32 bits/frame
      // Total Frames: 16200 jpg frames
      // Frame Rate: 30 fps
      // Sample Count: 51855360 audio samples
      // Sample Rate: 48000 samples/sec

      // Number of total seconds: 16200 / 30 = 540 sec = 9 min
      // Number of total seconds: 51855360 / 48000 = 1080 sec = 18 min
      // 2 because 2 channels (stereo)
      // Channels interleaved (first sample is left, second is right, third is left, fourth is right, etc.)
      // 48000 / 30 = 1600 samples/frame

      // set up original buffer here - stereo
      const sf::Int16* originalSamples{ originalBuffer.getSamples() };
      const sf::Uint64 sizeOfSingleChannel{ originalBuffer.getSampleCount() / 2u };
      sf::Int16* leftSamples{ new sf::Int16[sizeOfSingleChannel] };
      sf::Int16* rightSamples{ new sf::Int16[sizeOfSingleChannel] };
      for (sf::Uint64 i{ 0u }; i < sizeOfSingleChannel; ++i)
      {
          leftSamples[i] = originalSamples[i * 2u];
          rightSamples[i] = originalSamples[i * 2u + 1u];
      }
      sf::SoundBuffer leftBuffer;
      sf::SoundBuffer rightBuffer;
      leftBuffer.loadFromSamples(leftSamples, sizeOfSingleChannel, 1u, originalBuffer.getSampleRate());
      rightBuffer.loadFromSamples(rightSamples, sizeOfSingleChannel, 1u, originalBuffer.getSampleRate());

      const sf::Int16* left_samples = leftBuffer.getSamples();
      const sf::Int16* right_samples = rightBuffer.getSamples();



      /*
      //TEST BLOCK-------------------------------

      sf::SoundBuffer bufferTest;
      bufferTest.loadFromSamples(left_samples, sizeOfSingleChannel, 1u, originalBuffer.getSampleRate());
      bufferTest.saveToFile(input_directory + "/test.wav");

      sf::Int16* newAudioSamplesTest {new sf::Int16[originalBuffer.getSampleCount()]};
      for(int i = 0; i < 16000; i++){
          newAudioSamplesTest[i] = left_samples[i];
      }
      sf::SoundBuffer newAudioBufferTest;
      newAudioBufferTest.loadFromSamples(newAudioSamplesTest, leftBuffer.getSampleCount(), 1u, originalBuffer.getSampleRate());
      newAudioBufferTest.saveToFile(input_directory + "/test.wav");
      //TEST BLOCK-------------------------------
      */


      sf::Uint64 numSamples = leftBuffer.getSampleCount();
      //std::cout << numSamples << std::endl;

      for(int i=0; i <= end_index; i++) {
        frameAudio[i] = 0;
      }

      int curr_index = 0;

      int frame_num_samples = leftBuffer.getSampleRate() / FRAME_RATE;
      std::cout << frame_num_samples << std::endl;

      // Get total sample values for each frame.
      for(int i=0; i <= end_index; i++) {
        int count = 0;
        while(count < frame_num_samples) {
          double temp = abs(left_samples[curr_index]) + abs(right_samples[curr_index]);
          frameAudio[i] += temp;
          curr_index += 10;
          count += 10;
        }
      }

      // Get average total sample values for each shot.
      double max_audio = INT_MIN;
      double min_audio = INT_MAX;

      for(int i=0; i < shots.size(); i++) {
        int start = shots[i][0];
        int end = shots[i][1];
        int numFrames = end-start;
        // std::cout << numFrames << std::endl;
        sf::Int16* newAudioSamples {new sf::Int16[(end-start) * 1600 * 2]};
        //std::cout << originalSamples << std::endl;
        //std::cout << originalSamples+start << std::endl;
        memcpy(newAudioSamples, originalSamples+(start*1600*2), ((end-start) * 1600 * 4));

        double total_sample_val = 0;
        //sf::Uint64 audioSampleIndex = 0;
        for(int j=start; j <= end; j++) {
          total_sample_val += frameAudio[j];
          /*
          for(int k = 1; k <= 1600; k++){
              newAudioSamples[audioSampleIndex] = left_samples[k * j];
              newAudioSamples[audioSampleIndex + 1] = right_samples[k * j];
              audioSampleIndex+=2;
          }
          */
        }
        newAudioBuffer.loadFromSamples(newAudioSamples, ((end-start) * 1600 * 2), 2u, originalBuffer.getSampleRate());
        //newAudioBuffer.loadFromSamples(newAudioSamples, audioSampleIndex, 1u, originalBuffer.getSampleRate());
        newAudioBuffer.saveToFile("audio/shot" + std::to_string(i) + ".wav");
        std::string shot_string (std::to_string(start) + " " + std::to_string(end));

        auto it = shotAudio.find(shot_string);
        if(it != shotAudio.end()) {
            it->second = total_sample_val / (end - start + 1);
        } else {
            shotAudio.insert(std::make_pair(shot_string, total_sample_val / (end - start + 1)));
        }

        if(total_sample_val / (end - start + 1) > max_audio) {
          max_audio = total_sample_val / (end - start + 1);
        }

        if(total_sample_val / (end - start + 1) < min_audio) {
          min_audio = total_sample_val / (end - start + 1);
        }

        delete[] (newAudioSamples);
      }

      // Normalize audio scores.
      for(int i=0; i < shots.size(); i++) {
        //std::cout << i << " " << shots.size() << std::endl;
        int start = shots[i][0];
        int end = shots[i][1];

        std::string shot_string = std::to_string(start) + " " + std::to_string(end);

        double audio_score = shotAudio.at(shot_string);

        double normalized_audio_score = (audio_score - min_audio) / (max_audio - min_audio) * 100;

        auto it = shotAudio.find(shot_string);
        if(it != shotAudio.end())
            it->second = normalized_audio_score;
      }
      //delete[] leftSamples;
      //delete[] rightSamples;
      return true;
  }
}
