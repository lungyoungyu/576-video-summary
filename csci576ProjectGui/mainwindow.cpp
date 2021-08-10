#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QProcess>
#include <QMainWindow>
#include <QFileDialog>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>

#include <iostream>
#include <limits.h>
#include <unordered_map>
#include <string>
#include <stdio.h>      /* printf */
#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */


std::string input_directory, path_to_audio;
QString folderName, audioPath;
size_t start_index, end_index, frame_rate;

const bool DEBUG_TO_FILE = false;
const int MATCHING_INTERVAL = 10;
const std::string input_suffix = "jpg";
const int FRAME_RATE = 30;
const float MINIMUM_SHOT_LENGTH = FRAME_RATE * 2.5;
const float AVERAGE_THRESHOLD = 0.05;
const float RATIO = 0.30;

// Vector of shots (shot: [start frame number, end frame number])
static std::vector<std::array<int, 3>> shots;
// Hashmap of shot pairs to normalized audio scores.
static std::unordered_map<std::string, double> shotAudio = {{"",-1}};
// Array of audio sample values for each frame.
static double frameAudio[20000];
// Hashmap of shot pairs to normalized motion scores.
static std::unordered_map<std::string, double> shotMotion = {{"",-1}};
// Hashmap of shot pairs to total weighted scores.
static std::unordered_map<std::string, double> shotWeight = {{"",-1}};
// Vector of sorted shot pairs to total weighted scores.
static std::vector<std::pair<std::string, double>> sortedWeight;
// Vector of summary shots
static std::vector<std::array<int, 3>> summary_shots;

using namespace cv;
namespace po = boost::program_options;

float Feature_Matching_Score ( Mat, Mat, float );
bool Audio_Score(std::string &path_to_audio, size_t &start_index, size_t &end_index);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Sets the Main Window's central widget as the video player
    player = new QMediaPlayer(this);
    vw = new QVideoWidget(this);
    player->setVideoOutput(vw);
    this->setCentralWidget(vw);

    //Initialize the slider and progress bar
    slider = new QSlider(this);
    bar = new QProgressBar(this);
    slider->setOrientation(Qt::Horizontal);
    ui->statusbar->addPermanentWidget(slider);
    ui->statusbar->addPermanentWidget(bar);

    //Attach the slider to match the progress of the video being played and allow for seeking as user adjusts the position of slider
    connect(player, &QMediaPlayer::durationChanged, slider, &QSlider::setMaximum);
    connect(player, &QMediaPlayer::positionChanged, slider, &QSlider::setValue);
    connect(slider, &QSlider::sliderMoved, player, &QMediaPlayer::setPosition);

    //connect(player, &QMediaPlayer::durationChanged, bar, &QProgressBar::setMaximum);
    //connect(player, &QMediaPlayer::positionChanged, bar, &QProgressBar::setValue);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    //Open a dialog window to open a .avi to play in the video player
    QString filename = QFileDialog::getOpenFileName(this, "Open a File", "", "Video File(*.avi)");

    //qDebug(filename.toLatin1());

    //If user cancels the file open dialog it doesn't close out the current media and cause
    //DirectShowPlayerService::doSetUrlSource: Unresolved error code 0x80040216 ()
    if(!filename.isNull())
    {
        on_actionStop_triggered();
        player->setMedia(QUrl::fromLocalFile(filename));

        on_actionPlay_triggered();
    }
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
    //strs.push_back( txt.substr( initialPos, std::min( pos, txt.size() ) - initialPos ) );
    auto index = txt.find_last_of(" ");
    strs.push_back(txt.substr(++index));


    return strs.size();
}

void MainWindow::on_actionSummarize_triggered()
{
    shotMotion.clear();
    shotAudio.clear();
    shotWeight.clear();

    ui->statusbar->showMessage("Getting Video Scores...");
    //Opens a dialog box to select a directory of jpg files with the naming convention frame0.jpg, frame1.jpg, ..., framen.jpg
    folderName = QFileDialog::getExistingDirectory(this, "Open a Directory", "", QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
    input_directory = folderName.toUtf8().constData();
    //Open dialog box to get the path of desired audio input
    QString audioPath = QFileDialog::getOpenFileName(this, "Open an Audio File", "", "Audio File(*.wav)");
    path_to_audio = audioPath.toUtf8().constData();
    //std::cout << path_to_audio << std::endl;
    //Get the number of images in the directory to process images
    //https://stackoverflow.com/questions/6890757/counting-file-in-a-directory
    start_index = 0;
    QDir dir(folderName);
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    //Filter for only files starting with frame since output will be in same directory
    //https://forum.qt.io/topic/38216/qdir-filter-by-file-name-not-extension/4
    QStringList fileFilter("frame*");
    end_index = dir.entryList(fileFilter).count();





    //--------------------------VIDEO SCORING START--------------------------------------------------------------------------------
    /*
    std::ofstream histogram_file, shot_durations_file;
    // IMAGE FRAME SCORES OUTPUT FILE
    if(QFile::exists(folderName + "/processed_shots.csv"))
    {
        QFile::remove(folderName + "/processed_shots.csv");
    }
    shot_durations_file.open (input_directory + "/processed_shots.csv");
    shot_durations_file << "Starting_Frame,Ending_Frame,Motion_Percentage" << std::endl;

    if ( DEBUG_TO_FILE )
    {
        if(QFile::exists(folderName + "/matching_score.csv"))
        {
            QFile::remove(folderName + "/matching_score.csv");
        }
        histogram_file.open (input_directory + "/matching_score.csv");
        histogram_file << "Current_Frame_Number,Previous_Frame_Number,Matching_Score" << std::endl;
    }
    */
    std::vector <float> matching_scores;
    int shot_length = 0;
    int shot_start = 0;
    int shot_end = 0;

    // Go through all images
    bar->setMinimum(0);
    bar->setMaximum(end_index - MATCHING_INTERVAL);
    qDebug("Start Video Scoring");
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
            return;
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
                    /*
                    shot_durations_file << shot_start << "," << shot_end << ","
                        << Calculate_Motion ( matching_scores, shot_start, shot_end ) << std::endl;
                    */
                    // Add to vector of shots and hashmap of shot motion values.
                    std::array<int, 3> shot = {shot_start, shot_end, static_cast<int>(shots.size())};
                    shots.push_back(shot);
                    std::string shot_string = std::to_string(shot_start) + " " + std::to_string(shot_end);

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

        /*
        if ( DEBUG_TO_FILE )
        {
            histogram_file << img_counter << "," << img_counter - MATCHING_INTERVAL << "," << matching_score << std::endl;
        }
        */
        if(img_counter % 100 == 0)
        {
            std::cout << "Frame: " << img_counter << " / " << end_index - MATCHING_INTERVAL << " = " << matching_score << " % " << std::endl;
        }
        bar->setValue(img_counter);
    }

    /*
    if ( DEBUG_TO_FILE )
    {
        histogram_file.close ();
    }

    shot_durations_file << shot_start << "," << end_index << ","
        << Calculate_Motion ( matching_scores, shot_start, end_index ) << std::endl;
    */
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
    ui->statusbar->showMessage("Finished Video Scoring");
    bar->setValue(0);
    qDebug("Finished Video Scoring");
    //--------------------------VIDEO SCORING END--------------------------------------------------------------------------------
    /*
     * For debugging
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
    */


    //--------------------------AUDIO SCORING START--------------------------------------------------------------------------------
    // AUDIO SCORES

    qDebug("Start Audio Scoring");
    ui->statusbar->showMessage("Finished Video Scoring");
    if ( Audio_Score(path_to_audio, start_index, end_index) == false) {
       std::cout << "Invalid argument: path_to_audio ...." << std::endl;
       return;
    }
    qDebug("Finished Audio Scoring");

    //--------------------------AUDIO SCORING END--------------------------------------------------------------------------------


    //--------------------------TOTAL SCORING START--------------------------------------------------------------------------------
    // TOTAL WEIGHTED SCORES
    qDebug("Start Total Scoring");
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
      //std::cout << i->first<< " " << i->second << std::endl;
      sortedWeight.push_back(std::make_pair(i->first, i->second));
    }
    qDebug("Finished Total Scoring");

    //--------------------------TOTAL SCORING END--------------------------------------------------------------------------------


    //--------------------------SORT SCORING START--------------------------------------------------------------------------------
    // SORT SHOTS BY HIGHEST SCORE

    qDebug("Start Score Sorting");
    std::sort(sortedWeight.begin(),sortedWeight.end(),myComparison);

    int total_sec = 0;
    int index = 0;
    std::vector<std::string> v;
    while (total_sec < 90) {
      v.clear();
      if(index == sortedWeight.size())
      {
          break;
      }
      split(sortedWeight[index].first, v, ' ');
      //std::cout << sortedWeight[index].first << std::endl;
      //std::cout << v[0]<< std::endl << v[1] << std::endl;

      int start = std::stoi(v[0]);
      int end = std::stoi(v[1]);
      summary_shots.push_back({start, end,-1});
      index += 1;
      total_sec += (end - start + 1) / 30;
    }

    // SORT SHOTS BY ORDER IN VID
    std::sort(summary_shots.begin(), summary_shots.end());
    for(int i=0; i < summary_shots.size(); i++) {
          for(int j=0; j < shots.size(); j++) {
            if(summary_shots[i][0] == shots[j][0] && summary_shots[i][1] == shots[j][1]) {
              summary_shots[i][2] = shots[j][2];
            }
          }
          // std::cout << std::to_string(summary_shots[i][0]) << " " << std::to_string(summary_shots[i][1]) << " " << std::to_string(summary_shots[i][2]) << std::endl;
        }
    qDebug("Finished Score Sorting");
    //--------------------------SORT SCORING END--------------------------------------------------------------------------------

    //--------------------------MOVE FILES TO NEW DIRECTORY---------------------------------------------------------------------
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
    //--------------------------MOVE FILES TO NEW DIRECTORY---------------------------------------------------------------------
    std::ofstream audioSummaryList;
    // IMAGE FRAME SCORES OUTPUT FILE
    if(QFile::exists(folderName + "/audio_summary.txt"))
    {
        QFile::remove(folderName + "/audio_summary.txt");
    }
    audioSummaryList.open (input_directory + "/audio_summary.txt");
    //Delete any existing file with same output name to avoid conflicts essentially overwriting
    if(QFile::exists(folderName + "/summaryVideo.avi"))
    {
        QFile::remove(folderName + "/summaryVideo.avi");
    }
    if(QFile::exists(folderName + "/summaryAudio.wav"))
    {
        QFile::remove(folderName + "/summaryAudio.wav");
    }
    if(QFile::exists(folderName + "/summaryOutputAV.avi"))
    {
        QFile::remove(folderName + "/summaryOutputAV.avi");
    }
    //Get frame size to create the video, all frames should have same resolution
    //https://stackoverflow.com/questions/22704236/create-a-video-from-image-sequence-in-open-cv
    std::string img_path;
    Size S = imread(input_directory + "/frame0.jpg", IMREAD_COLOR).size();
    //Create the output file to save into the same directory as the frame images
    //https://docs.opencv.org/3.4/dd/d9e/classcv_1_1VideoWriter.html#ad59c61d8881ba2b2da22cff5487465b5
    cv::VideoWriter outputVideo;
    outputVideo.open(input_directory + "/summaryVideo.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), FRAME_RATE, S, true);

    //if it fails to create the output file
    if(!outputVideo.isOpened())
    {
        qDebug("Could not open the output video for writing");
    }

    qDebug("Start Audio/Video Compilation");
    //For loop to add each of the frames into the video to be created
    //Also set values for the progress bar to give user visual feedback that there is something happening
    //https://stackoverflow.com/questions/29794192/increment-progress-bar-inside-for-loop-in-c-qt
    bar->setMinimum(0);
    bar->setMaximum(summary_shots.size());
    ui->statusbar->showMessage("Compiling Summary Video...");

    for(int i=0; i < summary_shots.size(); i++) {
      int start = summary_shots[i][0];
      int end = summary_shots[i][1];
      int shotInd = summary_shots[i][2];
      for(int j = start; j < end; j++){
          img_path = input_directory + "/frame" + std::to_string ( j ) + ".jpg";
          outputVideo << imread(img_path, IMREAD_COLOR);
          bar->setValue(i);
      }
      audioSummaryList << "file 'shot" + std::to_string(shotInd) + ".wav'" << std::endl;
    }
    bar->setValue(0);
    ui->statusbar->showMessage("Processing Complete");
    //https://cects.com/concatenating-windows-ffmpeg/
    std::string ffmpegCmdAudioCat =  "cmd /C C:/FFmpeg/bin/ffmpeg -f concat -i " + input_directory + "/audio_summary.txt -c copy " + input_directory + "/summaryAudio.wav";
    system(ffmpegCmdAudioCat.c_str());

    //https://stackoverflow.com/questions/49410123/showing-cmd-terminal-in-qt-widgets-application
    //https://stackoverflow.com/questions/42166306/a-command-is-working-in-cmd-but-not-working-in-c-program-using-system-functi
    //https://datatofish.com/command-prompt-python/

    //https://superuser.com/questions/277642/how-to-merge-audio-and-video-file-in-ffmpeg
    std::string ffmpegCmdAV =  "cmd /C C:/FFmpeg/bin/ffmpeg -i " + input_directory + "/summaryVideo.avi -i " + input_directory + "/summaryAudio.wav -shortest " + input_directory + "/summaryOutputAV.avi";
    system(ffmpegCmdAV.c_str());
    //system("cmd /C C:/FFmpeg/bin/ffmpeg -i C:/Users/achan/Documents/CS572/hw4/project_dataset/frames/concert/output.avi -i C:/Users/achan/Documents/CS572/hw4/project_dataset/audio/concert.wav -shortest C:/Users/achan/Documents/CS572/hw4/project_dataset/frames/concert/test.avi");
    qDebug("Finished Audio/Video Compilation");

    qDebug("Finished Summarizing");
}

void MainWindow::on_actionPlay_triggered()
{
    //If DirectShowPlayerService::doRender: Unresolved error code 0x80040266 () comes up
    //May need to download K-Lite_Codec_Pack_1612_Basic.exe to play video
    //https://forum.qt.io/topic/86246/directshowplayerservice-dorender-unresolved-error-code-0x80040266
    player->play();
    ui->statusbar->showMessage("Playing");

    //Original logic to play frames sequentially in a loop
    /*
    qDebug(QString::fromStdString(input_directory).toLatin1());
    start_index = 0;
    QDir dir(folderName);
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    end_index = dir.count();

    for(size_t img_counter = start_index; img_counter < end_index; img_counter++)
    {
        std::string img_path = input_directory + "/frame" + std::to_string ( img_counter ) + ".jpg";
        if(imread ( img_path, IMREAD_COLOR ).empty())
        {
            std::cout << "Could not read image at: " << img_path << std::endl;
            return;
        }
        QPixmap img(QString::fromStdString(img_path));
        ui->imgLabel->setPixmap(img);
    }
    */

}

void MainWindow::on_actionPause_triggered()
{
    player->pause();
    ui->statusbar->showMessage("Paused");

}

void MainWindow::on_actionStop_triggered()
{
    player->stop();
    ui->statusbar->showMessage("Stopped");

}

void MainWindow::on_actionCreate_Video_triggered()
{
    //Opens a dialog box to select a directory of jpg files with the naming convention frame0.jpg, frame1.jpg, ..., framen.jpg
    //Turns all the frames into a single.avi file at 30FPS
    folderName = QFileDialog::getExistingDirectory(this, "Open a Directory", "", QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
    //qDebug(folderName.toLatin1());//Open dialog box to get the path of desired audio input
    QString audioPath = QFileDialog::getOpenFileName(this, "Open an Audio File", "", "Audio File(*.wav)");
    path_to_audio = audioPath.toUtf8().constData();

    //Delete any existing file with same output name to avoid conflicts essentially overwriting
    if(QFile::exists(folderName + "/output.avi"))
    {
        QFile::remove(folderName + "/output.avi");
    }

    //Get the number of images in the directory to process images
    //https://stackoverflow.com/questions/6890757/counting-file-in-a-directory
    input_directory = folderName.toUtf8().constData();
    QDir dir(folderName);
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    //Filter for only files starting with frame since output will be in same directory
    //https://forum.qt.io/topic/38216/qdir-filter-by-file-name-not-extension/4
    QStringList fileFilter("frame*");
    end_index = dir.entryList(fileFilter).count();

    //Get frame size to create the video, all frames should have same resolution
    //https://stackoverflow.com/questions/22704236/create-a-video-from-image-sequence-in-open-cv
    std::string img_path;
    Size S = imread(input_directory + "/frame0.jpg", IMREAD_COLOR).size();
    //Create the output file to save into the same directory as the frame images
    //https://docs.opencv.org/3.4/dd/d9e/classcv_1_1VideoWriter.html#ad59c61d8881ba2b2da22cff5487465b5
    cv::VideoWriter outputVideo;
    outputVideo.open(input_directory + "/output.avi", VideoWriter::fourcc('M', 'J', 'P', 'G'), FRAME_RATE, S, true);

    //if it fails to create the output file
    if(!outputVideo.isOpened())
    {
        qDebug("Could not open the output video for writing");
    }

    //For loop to add each of the frames into the video to be created
    //Also set values for the progress bar to give user visual feedback that there is something happening
    //https://stackoverflow.com/questions/29794192/increment-progress-bar-inside-for-loop-in-c-qt
    bar->setMinimum(0);
    bar->setMaximum(end_index);
    ui->statusbar->showMessage("Processing...");
    for(size_t i = 0; i < end_index; i++)
    {
        img_path = input_directory + "/frame" + std::to_string ( i ) + ".jpg";
        outputVideo << imread(img_path, IMREAD_COLOR);
        bar->setValue(i);
    }
    bar->setValue(0);
    ui->statusbar->showMessage("Processing Complete");//https://superuser.com/questions/277642/how-to-merge-audio-and-video-file-in-ffmpeg
    std::string ffmpegCmd =  "cmd /C C:/FFmpeg/bin/ffmpeg -i " + input_directory + "/output.avi -i " + path_to_audio + " -shortest " + input_directory + "/outputAV.avi";
    system(ffmpegCmd.c_str());
    qDebug("Finished writing");
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
bool Audio_Score(std::string &path_to_audio, size_t &start_index, size_t &end_index) {
  sf::SoundBuffer originalBuffer;
  sf::SoundBuffer newAudioBuffer;
  if ( originalBuffer.loadFromFile( path_to_audio ) == false )
  {
      std::cout << "Could not load audio file from: " << path_to_audio << std::endl;
      return false;
  }
  else
  {
      if(QFile::exists(folderName + "/shot*"))
      {
          QFile::remove(folderName + "/shot*");
      }
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
      //std::cout << frame_num_samples << std::endl;

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
        //int numFrames = end-start;
        //std::cout << numFrames << std::endl;
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
        newAudioBuffer.saveToFile(input_directory + "/shot" + std::to_string(i) + ".wav");
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
