# Minimum cmake version
cmake_minimum_required ( VERSION 3.2 )
# Project name
project ( video_summarization )
# External packages
find_package ( OpenCV REQUIRED )
find_package ( Boost REQUIRED COMPONENTS program_options filesystem )
find_package ( SFML 2.5 COMPONENTS graphics audio REQUIRED )
# Include directories
include_directories ( 
			${OpenCV_INCLUDE_DIRS}
            ${Boost_INCLUDE_DIRS} # Boost libraries			
			${PROJECT_SOURCE_DIR}/src # Source files
        	${PROJECT_SOURCE_DIR}/include # Header files
        	)
# Link directories
link_directories ( ${Boost_LIBRARY_DIRS} )
############################################################
add_executable( video_player src/main.cpp )
target_link_libraries( video_player ${OpenCV_LIBS} ${Boost_LIBRARIES} sfml-graphics sfml-audio )

add_executable( video_preprocessor src/preprocessor.cpp )
target_link_libraries( video_preprocessor ${OpenCV_LIBS} ${Boost_LIBRARIES} sfml-graphics sfml-audio )

