/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 5.9.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionPlay;
    QAction *actionPause;
    QAction *actionStop;
    QAction *actionSummarize;
    QAction *actionOpen;
    QAction *actionCreate_Video;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuVideo;
    QStatusBar *statusbar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QStringLiteral("MainWindow"));
        MainWindow->setEnabled(true);
        MainWindow->resize(320, 341);
        MainWindow->setMinimumSize(QSize(320, 300));
        MainWindow->setMaximumSize(QSize(320, 341));
        actionPlay = new QAction(MainWindow);
        actionPlay->setObjectName(QStringLiteral("actionPlay"));
        QIcon icon;
        icon.addFile(QStringLiteral(":/images/icons/play-button.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionPlay->setIcon(icon);
        actionPause = new QAction(MainWindow);
        actionPause->setObjectName(QStringLiteral("actionPause"));
        QIcon icon1;
        icon1.addFile(QStringLiteral(":/images/icons/pause.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionPause->setIcon(icon1);
        actionStop = new QAction(MainWindow);
        actionStop->setObjectName(QStringLiteral("actionStop"));
        QIcon icon2;
        icon2.addFile(QStringLiteral(":/images/icons/stop-button.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStop->setIcon(icon2);
        actionSummarize = new QAction(MainWindow);
        actionSummarize->setObjectName(QStringLiteral("actionSummarize"));
        QIcon icon3;
        icon3.addFile(QStringLiteral(":/images/icons/papers-outlines-stack.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSummarize->setIcon(icon3);
        actionOpen = new QAction(MainWindow);
        actionOpen->setObjectName(QStringLiteral("actionOpen"));
        QIcon icon4;
        icon4.addFile(QStringLiteral(":/images/icons/folder.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionOpen->setIcon(icon4);
        actionCreate_Video = new QAction(MainWindow);
        actionCreate_Video->setObjectName(QStringLiteral("actionCreate_Video"));
        QIcon icon5;
        icon5.addFile(QStringLiteral(":/images/icons/pen.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionCreate_Video->setIcon(icon5);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QStringLiteral("centralwidget"));
        centralwidget->setMinimumSize(QSize(320, 300));
        centralwidget->setMaximumSize(QSize(320, 180));
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QStringLiteral("menubar"));
        menubar->setGeometry(QRect(0, 0, 320, 21));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QStringLiteral("menuFile"));
        menuVideo = new QMenu(menubar);
        menuVideo->setObjectName(QStringLiteral("menuVideo"));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName(QStringLiteral("statusbar"));
        MainWindow->setStatusBar(statusbar);
        toolBar = new QToolBar(MainWindow);
        toolBar->setObjectName(QStringLiteral("toolBar"));
        MainWindow->addToolBar(Qt::TopToolBarArea, toolBar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuVideo->menuAction());
        menuFile->addAction(actionCreate_Video);
        menuFile->addAction(actionOpen);
        menuFile->addAction(actionSummarize);
        menuVideo->addAction(actionPlay);
        menuVideo->addAction(actionPause);
        menuVideo->addAction(actionStop);
        toolBar->addAction(actionCreate_Video);
        toolBar->addSeparator();
        toolBar->addAction(actionOpen);
        toolBar->addSeparator();
        toolBar->addAction(actionSummarize);
        toolBar->addSeparator();
        toolBar->addAction(actionPlay);
        toolBar->addAction(actionPause);
        toolBar->addAction(actionStop);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", Q_NULLPTR));
        actionPlay->setText(QApplication::translate("MainWindow", "Play", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        actionPlay->setToolTip(QApplication::translate("MainWindow", "Play the video", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        actionPause->setText(QApplication::translate("MainWindow", "Pause", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        actionPause->setToolTip(QApplication::translate("MainWindow", "Pause the video", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        actionStop->setText(QApplication::translate("MainWindow", "Stop", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        actionStop->setToolTip(QApplication::translate("MainWindow", "Stop the Video", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        actionSummarize->setText(QApplication::translate("MainWindow", "Summarize", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        actionSummarize->setToolTip(QApplication::translate("MainWindow", "Summarize a set of folers", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        actionOpen->setText(QApplication::translate("MainWindow", "Open", Q_NULLPTR));
#ifndef QT_NO_TOOLTIP
        actionOpen->setToolTip(QApplication::translate("MainWindow", "Open a folder", Q_NULLPTR));
#endif // QT_NO_TOOLTIP
        actionCreate_Video->setText(QApplication::translate("MainWindow", "Create Video", Q_NULLPTR));
        menuFile->setTitle(QApplication::translate("MainWindow", "File", Q_NULLPTR));
        menuVideo->setTitle(QApplication::translate("MainWindow", "Video", Q_NULLPTR));
        toolBar->setWindowTitle(QApplication::translate("MainWindow", "toolBar", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
