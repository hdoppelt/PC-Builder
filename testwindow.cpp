/**
 * @file testwindow.cpp
 *
 * @brief Implementation of the TestWindow class.
 *
 * This file defines the interactive test screen where users assemble
 * a PC by dragging and dropping components to the correct locations.
 *
 * The logic helps simulate building a PC while providing real-time
 * feedback to reinforce learning through interaction.
 *
 * @date 04/22/2025
 */

#include "testwindow.h"
#include "infobox.h"
#include "learningwindow.h"
#include "testchecker.h"
#include "ui_testwindow.h"
#include "winwindow.h"

TestWindow::TestWindow(LearningWindow* learningWindow, QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::TestWindow),
    learningWindow(learningWindow),
    lastSize(QSize(0, 0)),
    lastName("none"),
    location(QPoint(0, 0)),
    dontMove({"caseLabel", "centralwidget"}),
    reset(false)
{
    ui->setupUi(this);
    this->setWindowTitle("Test Window");
    setMouseTracking(true);

    ui->backButton->setIcon(QIcon::fromTheme("go-previous")); // uses system theme arrow

    // Initialize audio players
    goodAudio = new QMediaPlayer(this);
    badAudio = new QMediaPlayer(this);
    goodAudioOutput = new QAudioOutput(this);
    badAudioOutput = new QAudioOutput(this);
    winAudio = new QMediaPlayer(this);
    winAudioOutput = new QAudioOutput(this);

    // Set audio output for each sound effect
    goodAudio->setAudioOutput(goodAudioOutput);
    badAudio->setAudioOutput(badAudioOutput);
    winAudio->setAudioOutput(winAudioOutput);

    // Set the volume of the audio (range is 0 to 100)
    goodAudioOutput->setVolume(50);
    badAudioOutput->setVolume(50);
    winAudioOutput->setVolume(40);

    // Load button sounds from qrc file
    goodAudio->setSource(QUrl("qrc:/sounds/Good-Sound.wav"));
    badAudio->setSource(QUrl("qrc:/sounds/Bad-Sound.wav"));
    winAudio->setSource(QUrl("qrc:/sounds/Win-Sound.wav"));

    // PC Case Image
    QPixmap casePixmap(":/images/case.png");
    ui->caseLabel->setPixmap(casePixmap);
    ui->caseLabel->setScaledContents(true);
    ui->caseLabel->setToolTip("Computer Case");

    // Motherboard Image
    QPixmap motherboardPixmap(":/images/motherboard.png");
    ui->motherboardLabel->setPixmap(motherboardPixmap);
    ui->motherboardLabel->setScaledContents(true);
    ui->motherboardLabel->setToolTip("Motherboard");

    // Graphics Card Image
    QPixmap gpuPixmap(":/images/gpu.png");
    ui->gpuLabel->setPixmap(gpuPixmap);
    ui->gpuLabel->setScaledContents(true);
    ui->gpuLabel->setToolTip("Graphics Processing Unit (GPU)");

    // CPU Image
    QPixmap cpuPixmap(":/images/cpu.png");
    ui->cpuLabel->setPixmap(cpuPixmap);
    ui->cpuLabel->setScaledContents(true);
    ui->cpuLabel->setToolTip("Central Processing Unit (CPU)");

    // Memory Image
    QPixmap memoryPixmap(":/images/memory.png");
    ui->memoryLabel->setPixmap(memoryPixmap);
    ui->memoryLabel->setScaledContents(true);
    ui->memoryLabel->setToolTip("Solid State Drive (SSD)");

    // Ram Images
    QPixmap ramPixmap(":/images/ram.png");
    ui->ramLabel1->setPixmap(ramPixmap);
    ui->ramLabel1->setScaledContents(true);
    ui->ramLabel1->setToolTip("Random Access Memory (RAM)");

    ui->ramLabel2->setPixmap(ramPixmap);
    ui->ramLabel2->setScaledContents(true);
    ui->ramLabel2->setToolTip("Random Access Memory (RAM)");

    TestChecker* testChecker = new TestChecker();

    ui->progressLabel->hide();

    // Set up rainbow colors for progress label
    progressLabelColors = {"red", "orange", "yellow", "green", "blue", "indigo", "violet"};
    progressLabelIndex = 0;

    // Make a timer for the color text
    progressLabelTimer = new QTimer(this);
    progressLabelTimer->start(200);

    connect(progressLabelTimer,
            &QTimer::timeout,
            this,
            &TestWindow::updateProgressLabel
    );

    connect(this,
            &TestWindow::checkAnswer,
            testChecker,
            &TestChecker::checkPlacement
    );

    connect(this,
            &TestWindow::getCurrentStep,
            testChecker,
            &TestChecker::sendCurrentStep
    );

    connect(testChecker,
            &TestChecker::sendAnswer,
            this,
            &TestWindow::receiveAnswer
    );

    connect(ui->backButton,
            &QPushButton::clicked,
            this,
            &TestWindow::onBackButtonClicked
    );

    connect(this,
            &TestWindow::completedTesting,
            this,
            &TestWindow::openWinWindow
    );

}

TestWindow::~TestWindow()
{
    delete ui;
}

// SLOT
void TestWindow::onBackButtonClicked()
{
    this->hide();
    TestWindow* newTestWindow = new TestWindow(nullptr);
    LearningWindow* newLearningWindow = new LearningWindow(newTestWindow);
    newTestWindow->setLearningWindow(newLearningWindow);
    newLearningWindow->show();
}

void TestWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }

        else {
            event->acceptProposedAction();
        }

    }

    else {
        event->ignore();
    }
}

void TestWindow::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }

        else {
            event->acceptProposedAction();
        }

    }

    else {
        event->ignore();
    }
}

void TestWindow::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-dnditemdata")) {
        QByteArray itemData = event->mimeData()->data("application/x-dnditemdata");
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        QPixmap pixmap;
        QPoint offset;
        dataStream >> pixmap >> offset;

        QLabel* newIcon = new QLabel(this);
        QPoint cursor = event->position().toPoint();
        QPoint newLocal = snapLocation(cursor);
        newIcon->setPixmap(pixmap);
        newIcon->move(newLocal);
        newIcon->resize(lastSize);
        newIcon->setObjectName(lastName);
        newIcon->setScaledContents(true);
        newIcon->show();
        newIcon->setAttribute(Qt::WA_DeleteOnClose);

        if (event->source() == this) {
            event->setDropAction(Qt::MoveAction);
            event->accept();
        }

        else {
            event->acceptProposedAction();
        }

        emit checkAnswer(lastName, newLocal);

        if (reset) {
            newIcon->move(location);
            reset = false;
        }
    }

    else {
        event->ignore();
    }
}

void TestWindow::mousePressEvent(QMouseEvent *event)
{
    QLabel* child = static_cast<QLabel *>(childAt(event->pos()));

    if (!child || dontMove.contains(child->objectName())) {
        return;
    }

    lastSize = child->size();
    lastName = child->objectName();
    location = child->pos();
    QPixmap pixmap = child->pixmap();

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << pixmap << QPoint(event->pos() - child->pos());

    QMimeData* mimeData = new QMimeData;
    mimeData->setData("application/x-dnditemdata", itemData);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(pixmap.scaled(lastSize.width(), lastSize.height()));
    drag->setHotSpot(event->pos() - child->pos());

    child->setPixmap(pixmap);
    child->resize(lastSize);
    child->setScaledContents(true);

    if (drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction) == Qt::MoveAction) {
        child->close();
    }

    else {
        child->show();
        child->setPixmap(pixmap);
    }
}

QPoint TestWindow::snapLocation(QPoint cursor)
{
    int step = emit getCurrentStep();
    QPoint newDrop = QPoint(cursor.x() - .5 * lastSize.width(), cursor.y() - .5 * lastSize.height());

    switch (step) {

    case 1:

        // Motherboard location
        if (150 <= cursor.x() && cursor.x() <= 450 && 290 <= cursor.y() && cursor.y() <= 590) {
            newDrop = QPoint(200, 245);
        }

        break;

    case 2:
    case 3:
    case 4:
    case 5:
    case 6:

        // CPU location
        if (315 <= cursor.x() && cursor.x() <= 395 && 295 <= cursor.y() && cursor.y() <= 375) {
            newDrop = QPoint(315, 295);
        }

        // Memory location
        else if (260 <= cursor.x() && cursor.x() <= 350 && 370 <= cursor.y() && cursor.y() <= 420) {
            return QPoint(260, 370);
        }

        // GPU Location
        else if (300 <= cursor.x() && cursor.x() <= 350 && 430 <= cursor.y() && cursor.y() <= 550) {
            newDrop = QPoint(200, 370);
        }

        // RAM locations
        else if (420 <= cursor.x() && cursor.x() <= 440 && 280 <= cursor.y() && cursor.y() <= 410) {
            return QPoint(423, 270);
        } else if (440 <= cursor.x() && cursor.x() <= 460 && 280 <= cursor.y()
                   && cursor.y() <= 410) {
            return QPoint(443, 270);
        }

        break;
    }

    return newDrop;
}

void TestWindow::receiveAnswer(bool correctness, QString reason, QString part, QPoint newLocation)
{
    int step = emit getCurrentStep() - 1;
    if (step == 6 && correctness) {
        location = newLocation;
        dontMove.append(part);

        // Play the Win! sound
        winAudio->play();

        // Open the win window.
        emit completedTesting();

        // Update the progress
        double progress = step / 6.0;
        ui->progressBar->setValue(progress * 100.0);

        // Show "Nice Job!" text at 100%
        if (ui->progressBar->value() == 100) {
            ui->progressLabel->show();
            ui->progressLabel->setText("Nice Job!");
        }
    }

    else if (correctness) {
        location = newLocation;
        dontMove.append(part);
        step++;

        // Play the good! sound
        goodAudio->play();

        // Update the progress
        double progress = (step - 1) / 6.0;
        ui->progressBar->setValue(progress * 100.0);

        // Show "Almost There!" text at 50%
        if (ui->progressBar->value() >= 50) {
            ui->progressLabel->show();
            ui->progressLabel->setText("Almost There!");
        }

        else {
            ui->progressLabel->hide();
        }

        // Create info box letting you know you were right.
        InfoBox* dialog = new InfoBox("Correct", reason, this);
        dialog->exec();

        // Close when done.
        delete dialog;
    }

    else {
        reset = true;

        // Play the bad! sound
        badAudio->play();

        // Create info box with the reason why you were incorrect.
        InfoBox *dialog = new InfoBox("Incorrect", reason, this);
        dialog->exec();

        // Close when done.
        delete dialog;
    }
}

void TestWindow::setLearningWindow(LearningWindow* learningWindow)
{
    this->learningWindow = learningWindow;
}

void TestWindow::updateProgressLabel()
{
    ui->progressLabel->setStyleSheet(
        QString("color: %1; font-size: 60px;").arg(progressLabelColors[progressLabelIndex]));
    progressLabelIndex = (progressLabelIndex + 1) % progressLabelColors.size();
}

void TestWindow::openWinWindow()
{
    WinWindow* winWindow = new WinWindow(this);
    winWindow->show();
}
