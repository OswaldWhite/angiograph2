#include <QtWidgets>

#include "screenshot.h"

Screenshot::Screenshot()
    :  screenshotLabel(new QLabel(this))
{
    m_counter = 0;
    m_stop = true;

    screenshotLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    screenshotLabel->setAlignment(Qt::AlignCenter);

    const QRect screenGeometry = QApplication::desktop()->screenGeometry(this);
    screenshotLabel->setMinimumSize(screenGeometry.width() / 8, screenGeometry.height() / 8);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(screenshotLabel);

    QGroupBox *optionsGroupBox = new QGroupBox(tr("Options"), this);
    delaySpinBox = new QSpinBox(optionsGroupBox);
    delaySpinBox->setSuffix(tr(" s"));
    delaySpinBox->setMaximum(60);

    typedef void (QSpinBox::*QSpinBoxIntSignal)(int);
    connect(delaySpinBox, static_cast<QSpinBoxIntSignal>(&QSpinBox::valueChanged),
            this, &Screenshot::updateCheckBox);

    hideThisWindowCheckBox = new QCheckBox(tr("Hide This Window"), optionsGroupBox);

    QGridLayout *optionsGroupBoxLayout = new QGridLayout(optionsGroupBox);
    optionsGroupBoxLayout->addWidget(new QLabel(tr("Screenshot Delay:"), this), 0, 0);
    optionsGroupBoxLayout->addWidget(delaySpinBox, 0, 1);
    optionsGroupBoxLayout->addWidget(hideThisWindowCheckBox, 1, 0, 1, 2);

    mainLayout->addWidget(optionsGroupBox);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;

    startRegularScreenshotButton = new QPushButton(tr("Start screen shooting"), this);
    connect(startRegularScreenshotButton, &QPushButton::clicked, this, &Screenshot::startShooting);
    buttonsLayout->addWidget(startRegularScreenshotButton);

    QPushButton *stopShootingButton = new QPushButton(tr("Stop screen shooting"), this);
    connect(stopShootingButton, &QPushButton::clicked, this, &Screenshot::stopShooting);
    buttonsLayout->addWidget(stopShootingButton);

    QPushButton *saveScreenshotButton = new QPushButton(tr("Save Screenshot"), this);
    connect(saveScreenshotButton, &QPushButton::clicked, this, &Screenshot::saveScreenshot);
    buttonsLayout->addWidget(saveScreenshotButton);

    QPushButton *quitScreenshotButton = new QPushButton(tr("Quit"), this);
    quitScreenshotButton->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(quitScreenshotButton, &QPushButton::clicked, this, &QWidget::close);
    buttonsLayout->addWidget(quitScreenshotButton);
    buttonsLayout->addStretch();
    mainLayout->addLayout(buttonsLayout);

    shootScreen();
    delaySpinBox->setValue(5);

    setWindowTitle(tr("Screenshot"));
    resize(300, 200);
}

void Screenshot::resizeEvent(QResizeEvent * /* event */)
{
    QSize scaledSize = originalPixmap.size();
    scaledSize.scale(screenshotLabel->size(), Qt::KeepAspectRatio);
    if (!screenshotLabel->pixmap() || scaledSize != screenshotLabel->pixmap()->size())
        updateScreenshotLabel();
}

void Screenshot::newScreenshot()
{
    if (hideThisWindowCheckBox->isChecked())
        hide();

    startRegularScreenshotButton->setDisabled(true);

    QTimer::singleShot(1000, this, &Screenshot::shootScreen);
}

void Screenshot::regularShooting()
{
    if (m_stop)
        return;

    newScreenshot();
    QTimer::singleShot(1000, this, &Screenshot::regularShooting);
}


void Screenshot::saveScreenshotToDisk()
{
    QDir dir("E:/film/angio/screenshot");
    if (!dir.exists()) {
        QDir dir2("E:/film/angio/");
        dir2.mkdir("screenshot");
    }

    const QString format = "png";

    QString fileName = tr("E:/film/angio/screenshot/%1%2").arg(dir.dirName()).arg(m_counter) + tr(".") + format;
    qDebug() << "File trys to be saved to " << fileName;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadWrite))
        return;
    if (!originalPixmap.save(fileName, 0)) {
        QMessageBox::warning(this, tr("Save Error"), tr("The image could not be saved to \"%1\".")
                             .arg(QDir::toNativeSeparators(fileName)));
    } else {
        qDebug() << "File is created";
        m_counter++;
        qDebug() << " m_counter =" << m_counter;
    }
}

void Screenshot::saveScreenshot()
{
    const QString format = "png";
    QString initialPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (initialPath.isEmpty())
        initialPath = QDir::currentPath();
    initialPath += tr("/untitled.") + format;

    QFileDialog fileDialog(this, tr("Save As"), initialPath);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setFileMode(QFileDialog::AnyFile);
    fileDialog.setDirectory(initialPath);
    QStringList mimeTypes;
    foreach (const QByteArray &bf, QImageWriter::supportedMimeTypes())
        mimeTypes.append(QLatin1String(bf));
    fileDialog.setMimeTypeFilters(mimeTypes);
    fileDialog.selectMimeTypeFilter("image/" + format);
    fileDialog.setDefaultSuffix(format);
    if (fileDialog.exec() != QDialog::Accepted)
        return;
    const QString fileName = fileDialog.selectedFiles().first();
    if (!originalPixmap.save(fileName)) {
        QMessageBox::warning(this, tr("Save Error"), tr("The image could not be saved to \"%1\".")
                             .arg(QDir::toNativeSeparators(fileName)));
    }
}

void Screenshot::shootScreen()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (const QWindow *window = windowHandle())
        screen = window->screen();
    if (!screen)
        return;

    if (delaySpinBox->value() != 0)
        QApplication::beep();

    originalPixmap = screen->grabWindow(0);
    updateScreenshotLabel();

    startRegularScreenshotButton->setDisabled(false);

    if (hideThisWindowCheckBox->isChecked())
        show();
    saveScreenshotToDisk();
}

void Screenshot::updateCheckBox()
{
    if (delaySpinBox->value() == 0) {
        hideThisWindowCheckBox->setDisabled(true);
        hideThisWindowCheckBox->setChecked(false);
    } else {
        hideThisWindowCheckBox->setDisabled(false);
    }
}

void Screenshot::updateScreenshotLabel()
{
    screenshotLabel->setPixmap(originalPixmap.scaled(screenshotLabel->size(),
                                                     Qt::KeepAspectRatio,
                                                     Qt::SmoothTransformation));
}

void Screenshot::startShooting()
{
    m_stop = false;

    deleteOldShots();

    regularShooting();
}

void Screenshot::stopShooting()
{
    m_stop = true;
}

void Screenshot::deleteOldShots()
{
     QString filePath = tr("E:/film/angio/screenshot/");
     QDir dir(filePath);
     if (dir.exists()) {
         dir.setNameFilters(QStringList() << "*.*");
         dir.setFilter(QDir::Files);
         foreach(QString dirFile, dir.entryList()) {
             dir.remove(dirFile);
         }
}
