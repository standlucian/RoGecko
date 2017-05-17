/*
Copyright 2011 Bastian Loeher, Roland Wirth

This file is part of GECKO.

GECKO is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

GECKO is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* this plugin is meant to replace the event plugin provided by gecko-gecko because of the limit of
  32 channels in the event. This limit is due to the output format which uses a 32 bit channel mask to
  inform which of the channels has data. At the moment of trying to implement this software, the RoSphere
  has 25 detectors, (both energy and time information being required) and a few more are envisaged with the
  addition of particle detectors. Gabi S.
*/

#ifndef EVENTBUILDERBIGPLUGIN_H
#define EVENTBUILDERBIGPLUGIN_H

#include "baseplugin.h"
#include "modulemanager.h"
#include "runmanager.h"
#include "pluginmanager.h"
#include "pluginconnectorqueued.h"
#include <iostream>
#include <QTimer>
#include <QGridLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <boost/filesystem/convenience.hpp>
#include <QSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <algorithm>
#include <vector>
#include <QTime>
#include <QCheckBox>
#include <fstream>
#include <QInputDialog>
#include <fstream>
#include <stdio.h>
#include <bitset>

class BasePlugin;

class EventBuilderBIGPlugin : public BasePlugin
{
    Q_OBJECT

protected:
    Attributes attribs_;
    QString filePrefix;
    uint64_t total_bytes_written;
    uint32_t current_bytes_written;
    uint32_t current_file_number;
    uint32_t number_of_mb;
    uint32_t hoursToReset;
    bool reset;
    QString writePath;
    bool rawWrite;
    bool outputValue;

    QTimer* resetTimer;
    QTimer* triggerToLogbook;
    QTimer* triggerToRunManager;

    QPushButton* addNote;

    QLabel* confNameLabel;
    QPushButton* confNameButton;
    QLabel* currentFileNameLabel;
    QLabel* currentBytesWrittenLabel;
    QLabel* timeElapsedLabel;
    QLabel* totalBytesWrittenLabel;
    QLabel* writeLabel2;
    QPushButton* writeButton;
    boost::filesystem::path runPath;
    QLineEdit* prefEdit;
    QLineEdit* titleEdit;
    QString title;

    QSpinBox* writtenReset;
    QSpinBox* timeReset;

    QSpinBox* setCoincInterval;
    QCheckBox* rawWriteBox;

    int offset;
    virtual void createSettings(QGridLayout*);
    QString makeFileName();
    QString makeRawName();

    QLabel* bytesFreeOnDiskLabel;

    QFile outFile;
    QFile rawFile;
    QDir outDir;
    QTime lastUpdateTime;
    QTime elapsedTime;
    QTime pulsingTime;
    QString confName;

public:
    EventBuilderBIGPlugin(int _id, QString _name, const Attributes &_attrs);

    static AbstractPlugin *create (int id, const QString &name, const Attributes &attrs) {
        return new EventBuilderBIGPlugin (id, name, attrs);
    }
    virtual ~EventBuilderBIGPlugin();

    AttributeMap getAttributeMap () const;
    Attributes getAttributes () const;
    static AttributeMap getEventBuilderAttributeMap ();

    void writeToCache();
    void setConfName(QString);
    void setWriteFolder(QString);

    virtual void userProcess();
    virtual void applySettings(QSettings*);
    virtual void saveSettings(QSettings*);
    virtual int writeCache();
    void configureDetectors(QString);


public slots:
    void updateRunName();
    void throwLastCache();

    void setTimeReset();
    void receiveInterfaceSignal(bool);
    void writeTrigger();
    void writeRunTrigger();

    void addNoteClicked();
    void confNameButtonClicked();
    void writeButtonClicked();
    void prefEditInput();
    void titleInput();

    void mbSizeChanged();
    void timeResetInput();

    void rawWriteChanged();

    void updateByteCounters();
    void runStartingEvent();
    void uiInput();

    void openNewFile();

private:
    std::ofstream logbook;
    QDataStream out;
    QDataStream raw;
    bool hasData;
    uint32_t leastTime;
    int nofInputs;

    int typeNo;
    int numberOfDet;
    int written;
    uint64_t nofTriggers;
    uint64_t lastNofTriggers;
    std::vector <uint16_t> noDetType;
    std::vector <uint32_t> totalNoDet;
    uint16_t cache[8192];
    std::vector <std::vector<uint32_t> > detchan;
    QVector <bool> toBeRead;
    QVector <bool> readIt;
    QVector <int> readPointer;
    QVector <int> typeParam;
    QVector <int> resetPosition;
    QVector<QVector<uint32_t> > data;
    QVector<QVector<uint32_t> > dataTemp;

};

#endif // EVENTBUILDERBIGPLUGIN_H
