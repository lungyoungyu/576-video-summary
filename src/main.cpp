#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

const int ESCAPE_CHAR = 27;

using namespace cv;
namespace po = boost::program_options;

bool Parse_Arguments (  int argc, char **argv, std::string &input_directory,
                        std::string &input_prefix, std::string &input_suffix,
                        std::string &path_to_audio, size_t &start_index,
                        size_t &end_index, size_t &frame_rate );

int main ( int argc, char **argv )
{
    std::string input_directory, input_prefix, input_suffix, path_to_audio;
    size_t start_index, end_index, frame_rate;
    // Parse arguments
    if ( Parse_Arguments ( argc, argv, input_directory,
                            input_prefix, input_suffix,
                            path_to_audio, start_index,
                            end_index, frame_rate ) == false )
    {
        std::cout << "Invalid arguments ...." << std::endl;
        return -1;
    }

    int image_interval = ( 1.0 / frame_rate ) * 1000;
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



    // Replay images
    for ( size_t img_counter = start_index; img_counter < end_index; img_counter++ )
    {
        // Construct image path
        /*
        std::string img_path = input_directory + input_prefix
                                + std::to_string ( img_counter ) + "." + input_suffix;
        */
        std::string img_path = input_directory + "/frame" + std::to_string ( img_counter ) + "." + input_suffix;

        // Read image
        Mat img = imread ( img_path, IMREAD_COLOR );
        // Ensure it is not empty
        if ( img.empty ( ) )
        {
            std::cout << "Could not read image at: " << img_path << std::endl;
            return -1;
        }

        std::cout << img_path << std::endl;
        imshow ( "Video summarization", img );
        int key = waitKey ( image_interval );

        if ( key == ESCAPE_CHAR )
        {
            std::cout << "Exitting ...." << std::endl;
            return 0;
        }
    }
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
        ("input prefix,p", po::value<std::string>(&input_prefix)->default_value(""),
         "Prefix of input image files")
        ("input extension,e", po::value<std::string>(&input_suffix)->default_value("jpg"),
         "Extension of input image files")
        ("input audio file,a", po::value<std::string>(&path_to_audio)->default_value(""),
         "Path to audio file")
        ("start index,s", po::value<size_t>(&start_index)->default_value(0),
         "Starting index of image files")
        ("end index,x", po::value<size_t>(&end_index)->default_value(5000),
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
