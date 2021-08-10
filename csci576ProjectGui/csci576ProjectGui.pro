#-------------------------------------------------
#
# Project created by QtCreator 2017-03-05T12:30:06
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = csci576ProjectGui
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
win32{
    INCLUDEPATH += C:\opencv\build\include
    INCLUDEPATH += C:\boost_1_76_0
    INCLUDEPATH += C:\SFML-2.5.1-sources\include
    INCLUDEPATH += C:\FFmpeg\include

    LIBS += C:\opencv-build\bin\libopencv_core320.dll
    LIBS += C:\opencv-build\bin\libopencv_highgui320.dll
    LIBS += C:\opencv-build\bin\libopencv_imgcodecs320.dll
    LIBS += C:\opencv-build\bin\libopencv_imgproc320.dll
    LIBS += C:\opencv-build\bin\libopencv_features2d320.dll
    LIBS += C:\opencv-build\bin\libopencv_calib3d320.dll
    # https://forum.qt.io/topic/92848/opencv-error-undefined-reference-to-cv-videocapture-videocapture
    LIBS += C:\opencv-build\bin\libopencv_videoio320.dll
    LIBS += "-LC:\boost_1_76_0\libs"

    LIBS += -LC:\SFML\lib
    LIBS += -LC:\SFML-2.5.1-sources\extlibs\libs-mingw\x86

    #Release Configuration
    CONFIG(release, debug|release):
    {
    #Audio Related Libs
    LIBS += -lsfml-audio-d          #SFML Static Module
    LIBS += -lopenal32              #Dependency
    LIBS += -lFLAC                  #Dependency
    LIBS += -lvorbisenc             #Dependency
    LIBS += -lvorbisfile            #Dependency
    LIBS += -lvorbis                #Dependency
    LIBS += -logg                   #Dependency

    #SFML-Graphics Libs
    LIBS += -lsfml-graphics-d       #SFML Static Module
    LIBS += -lfreetype              #Dependency
    #SFML-Network Libs
    LIBS += -lsfml-network-d        #SFML Static Module
    LIBS += -lws2_32                #Dependency

    #SFML-Window Libs
    LIBS += -lsfml-window-d         #SFML Static Module
    LIBS += -lopengl32              #Dependency
    LIBS += -lgdi32                 #Dependency

    #SFML-System Libs
    LIBS += -lsfml-system-d         #SFML Static Module
    LIBS += -lwinmm                 #Dependency
    }

    #Debug Configuration
    CONFIG(debug, debug|release):
    {
    #Audio Related Libs
    LIBS += -LC:\SFML\lib\sfml-audio-d-2.dll       #SFML Static Module
    LIBS += -lopenal32              #Dependency
    LIBS += -lFLAC                  #Dependency
    LIBS += -lvorbisenc             #Dependency
    LIBS += -lvorbisfile            #Dependency
    LIBS += -lvorbis                #Dependency
    LIBS += -logg                   #Dependency

    #SFML-Graphics Libs
    LIBS += -LC:\SFML\lib\sfml-graphics-d-2     #SFML Static Module
    LIBS += -lfreetype              #Dependency

    #SFML-Network Libs
    LIBS += -LC:\SFML\lib\sfml-network-d-2      #SFML Static Module
    LIBS += -lws2_32                #Dependency

    #SFML-Window Libs
    LIBS += -LC:\SFML\lib\sfml-window-d-2       #SFML Static Module
    LIBS += -lopengl32              #Dependency
    LIBS += -lgdi32                 #Dependency

    #SFML-System Libs
    LIBS += -LC:\SFML\lib\sfml-system-d-2       #SFML Static Module
    LIBS += -lwinmm                 #Dependency
    }

    LIBS += "-LC:\FFmpeg\lib"
}

# more correct variant, how set includepath and libs for mingw
# add system variable: OPENCV_SDK_DIR=C:/opencv/opencv-build/install
# read http://doc.qt.io/qt-5/qmake-variable-reference.html#libs

#INCLUDEPATH += $$(OPENCV_SDK_DIR)/include

#LIBS += -L$$(OPENCV_SDK_DIR)/x86/mingw/lib \
#        -lopencv_core320        \
#        -lopencv_highgui320     \
#        -lopencv_imgcodecs320   \
#        -lopencv_imgproc320     \
#        -lopencv_features2d320  \
#        -lopencv_calib3d320

RESOURCES += \
    resources.qrc
