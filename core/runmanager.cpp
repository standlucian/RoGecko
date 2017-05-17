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

#include "runmanager.h"

#include <QThreadPool>
#include <QTimer>
#include <QDir>
#include <QFile>

#include "scopemainwindow.h"
#include "runthread.h"
#include "pluginthread.h"
#include "interfacemanager.h"
#include "abstractmodule.h"
#include "abstractinterface.h"
#include "systeminfo.h"
#include "eventbuffer.h"
#include "outputplugin.h"

#include <stdexcept>
#include <iostream>

RunManager *RunManager::inst = NULL;

RunManager *RunManager::ptr () {
    if (inst == NULL)
        inst = new RunManager();
    return inst;
}

RunManager &RunManager::ref () {
    return *ptr ();
}

RunManager::RunManager()
: singleeventmode (false)
, running (false)
, localRun (true)
, runName ("/tmp")
, evcnt (0)
, lastevcnt (0)
, nofTriggers (0)
, trigsPerSec (0)
, mainwnd (NULL)
, runthread (NULL)
, pluginthread (NULL)
, updateTimer (new QTimer (this))
, sysinfo (new SystemInfo ())
, evbuf (new EventBuffer (10))
, beamStatus(1)
{
    state.resize(2);
    state.setBit(StateRunning,false);
    state.setBit(StateRemoteControlled,false);

    updateTimer->setInterval (500);
    connect (updateTimer, SIGNAL(timeout()), SLOT(sendUpdate()));
    std::cout << "Instantiated RunManager" << std::endl;
}

RunManager::~RunManager()
{

}

void RunManager::setRunName (QString newValue) {
    if(!running) {
        runName = newValue;
        emit runNameChanged();
    }
    else
        throw std::logic_error ("cannot set run name while run is active");

}

void RunManager::start (QString info) {
    if (running)
        throw std::logic_error ("start while running");

    runInfo = info;
    running = true;
    state.setBit(StateRunning,true);

    foreach(AbstractInterface* iface, (*InterfaceManager::ref ().list ()))
    {
        if(!iface->isOpen())
            iface->open();
    }

    runthread = new RunThread ();

    // FIXME
   // foreach (AbstractModule *m, *ModuleManager::ref().list ()) {
  //      connect(runthread,SIGNAL(acquisitionDone()), m, SLOT(prepareForNextAcquisition()));
  //  }

    startTime = QDateTime::currentDateTime ();
    evcnt = 0;
    lastevcnt = 0;
    evpersec = 0;
    trigsPerSec = 0;
    writeRunStartFile (info);

    pluginthread = new PluginThread(PluginManager::ptr (), ModuleManager::ptr ());
    connect (runthread, SIGNAL(acquisitionDone()), pluginthread, SLOT(acquisitionDone()), Qt::DirectConnection);
    pluginthread->start(QThread::NormalPriority);

    runthread->start(QThread::TimeCriticalPriority);

    updateTimer->start ();
    emit runStarted ();
}

void RunManager::stop (QString info) {
    if (!running)
        return;

    emit runStopping ();

    updateTimer->stop ();
    runInfo = QString ();

    runthread->stop ();
    runthread->wait (1000);
    pluginthread->stop ();
    pluginthread->wait (1000);

    stopTime = QDateTime::currentDateTime ();
    writeRunStopFile (info);

    delete runthread;
    runthread = NULL;

    delete pluginthread;
    pluginthread = NULL;

    // Reset buffers
    foreach(AbstractModule* m, (*ModuleManager::ref ().list ()))
    {
        foreach(PluginConnector* bpc, (*m->getOutputPlugin()->getOutputs()))
        {
            bpc->reset();
        }
    }

    while (!evbuf->empty())
        delete evbuf->dequeue ();

    // Release dead time
    foreach(AbstractInterface* iface, (*InterfaceManager::ref ().list ()))
    {
        if(iface->isOpen())
            iface->setOutput1(false);
    }

    // close interfaces
    foreach(AbstractInterface* iface, (*InterfaceManager::ref ().list ()))
    {
        if(iface->isOpen())
            iface->close ();
    }


    running = false;
    state.setBit(StateRunning,false);

    emit runStopped ();
}

float RunManager::getEventRate () const {
    return (1000.0 * (evcnt - lastevcnt)) / updateTimer->interval ();
}

void RunManager::sendUpdate () {
    unsigned newev = runthread->getNofEvents ();
    //float evpersec = (1000.0 * (newev - evcnt)) / updateTimer->interval ();
    //float evpersec = ((evcnt) / (double)(getRunSeconds()) ) ; // New algo
    // exponentially decaying average
    evpersec = 0.9 * evpersec + 0.1 * (1000.0 * (newev - evcnt)) / updateTimer->interval ();
    lastevcnt = evcnt;
    evcnt = newev;

    if(nofTriggers!=lastNofTriggers)
    {
        trigsPerSec=(nofTriggers-lastNofTriggers)/5;
        lastNofTriggers=nofTriggers;
    }

    if(nofTriggers==0)
        trigsPerSec=0;

    emit runUpdate(evpersec, newev, nofTriggers, trigsPerSec);
}

uint64_t RunManager::sendTriggers()
{
    return nofTriggers;
}

uint64_t RunManager::sendTriggerRate()
{
    return trigsPerSec;
}

void RunManager::transmitTriggerRate(uint64_t trigs)
{
    nofTriggers=trigs;
}

void RunManager::changeBeamStatus(bool receivedBeamStatus)
{
    if((receivedBeamStatus!=beamStatus)&&(running))
    {
        beamStatus=receivedBeamStatus;
        runthread->forceRead();
        emit emitBeamStatus(beamStatus);
    }
}

void RunManager::changeRecordStatus(bool receivedRecordStatus)
{
    if((receivedRecordStatus!=recordStatus)&&(running))
    {
        recordStatus=receivedRecordStatus;
        runthread->forceRead();
        emit emitRecordStatus(recordStatus);
    }
}

QString RunManager::stateToString (State _state) const
{
    switch(_state)
    {
    case StateRunning:
        return QString("running");
    case StateRemoteControlled:
        return QString("remoteControlled");
    default:
        return QString("unknown");
    }
    return 0;
}

const QString RunManager::getStateString() const {
    QStringList list;
    for(int i = 0; i < state.size(); i++)
    {
        if(state.testBit(i) == true) list.append(stateToString((State)i));
        else list.append("not_"+stateToString((State)i));
    }
    return list.join(" ");
}

void RunManager::writeRunStartFile (QString info)
{
    QDir runDir(runName);
    if(!runDir.exists())
    {
        if(!runDir.mkpath(runName))
        {
            throw std::runtime_error ("cannot create run directory");
        }
    }

    QFile file(runName+"/start.info");
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QStringList infolines (info.trimmed().split('\n'));
        for (QStringList::iterator i = infolines.begin(); i != infolines.end (); ++i)
            i->prepend ("#  ");
        QTextStream out(&file);
        out << "# " "Run Start File generated by Gecko application" << "\n"
            << "# " "Run Name: " << runName << "\n"
            << "# " "Start Time: " << startTime.toString() << "\n"
            << "# " "Single event mode: " << singleeventmode << "\n"
            << "# " "Notes: " << "\n"
            << infolines.join ("\n") << "\n"
            ;
    }


    // Save settings for run
    QString tmpFileName = runName+"/settings.ini";
    mainwnd->saveSettingsToFile (tmpFileName);
}

void RunManager::writeRunStopFile (QString info) {
    QDir runDir(runName);
    if(!runDir.exists())
    {
        if(!runDir.mkpath(runName))
        {
            throw std::runtime_error ("cannot create run directory");
        }
    }

    QFile file(runName+"/stop.info");
    if(file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QStringList infolines (info.trimmed().split('\n'));
        for (QStringList::iterator i = infolines.begin(); i != infolines.end (); ++i)
            i->prepend ("#  ");
        QTextStream out(&file);
        out << "# " << "Run Stop File generated by Gecko application" << "\n"
            << "# " << "Run Name: " << runName << "\n"
            << "# " << "Stop Time: " << stopTime.toString() << "\n"
            << "# " << "Duration: " << startTime.secsTo(stopTime) << " s" << "\n"
            << "# " << "Number of recorded events: " << runthread->getNofEvents() << "\n"
            << "# " "Notes: " << "\n"
            << infolines.join ("\n") << "\n"
            ;
    }
}
