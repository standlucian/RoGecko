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

#include "pulsing.h"

static PluginRegistrar reg ("pulsing", &Pulsing::create, AbstractPlugin::GroupAux, Pulsing::getPulsingAttributeMap ());

Pulsing::Pulsing(int id, QString name, const Attributes &attrs)
    : BasePlugin(id, name)
    , attrs_ (attrs)
    , beamStatus (0)
    , pulsingActive (1)
    , beamOnRecord (1)
    , beamOffRecord (1)
{
    createSettings(settingsLayout);

    //Number of inputs that have to have data in order for the plugin to run
    setNumberOfMandatoryInputs(0);

    connect(RunManager::ptr(),SIGNAL(runStopped()),this,SLOT(stopBeam()));

    //Define the timer that changes the pulser status and connect it
    setInterfaceOutput = new QTimer();
    connect(setInterfaceOutput,SIGNAL(timeout()),this,SLOT(sendInterfaceSignal()));

    updateInterfaceTimer = new QTimer();
    updateInterfaceTimer->start(1000);
    connect(updateInterfaceTimer,SIGNAL(timeout()),this,SLOT(updateInterface()));

    RunManager::ptr()->changeBeamStatus(beamStatus);
}

void Pulsing::createSettings (QGridLayout * l) {
    QWidget* container = new QWidget();
    {
        QGridLayout* cl = new QGridLayout;
        //Setting the pulsing interval
        setPulsingH = new QSpinBox();
        setPulsingH->setMinimum(0);
        setPulsingH->setMaximum(5);
        setPulsingH->setSingleStep(1);
        setPulsingH->setSuffix(" h");

        setPulsingM = new QSpinBox();
        setPulsingM->setMinimum(0);
        setPulsingM->setMaximum(60);
        setPulsingM->setSingleStep(1);
        setPulsingM->setSuffix(" min");

        setPulsingS = new QSpinBox();
        setPulsingS->setMinimum(0);
        setPulsingS->setMaximum(60);
        setPulsingS->setSingleStep(1);
        setPulsingS->setSuffix(" sec");

        setPulsingMs = new QSpinBox();
        setPulsingMs->setMinimum(0);
        setPulsingMs->setMaximum(1000);
        setPulsingMs->setSingleStep(1);
        setPulsingMs->setSuffix(" msec");

        //Connecting the pulsing interval Spin boxes
        connect(setPulsingH,SIGNAL(valueChanged(int)),this,SLOT(pulsingInput()));
        connect(setPulsingM,SIGNAL(valueChanged(int)),this,SLOT(pulsingInput()));
        connect(setPulsingS,SIGNAL(valueChanged(int)),this,SLOT(pulsingInput()));
        connect(setPulsingMs,SIGNAL(valueChanged(int)),this,SLOT(pulsingInput()));

        //Manual control of the pulsing
        pulsingManual = new QPushButton(tr(""));
        if(beamStatus)
            pulsingManual->setText(tr("Beam is now ON: Set Beam Off"));
        else
            pulsingManual->setText(tr("Beam is now OFF: Set Beam On"));
        connect(pulsingManual,SIGNAL(clicked()),this,SLOT(pulsingButtonPressed()));

        timeElapsedLabel= new QLabel(tr("%1:%2:%3").arg(0,2,10,QChar('0')).arg(0,2,10,QChar('0')).arg(0,2,10,QChar('0')));

        //Stop or start the pulsing
        stopStartPulsing = new QPushButton(tr(""));
        stopStartPalette = new QPalette(stopStartPulsing->palette());
        if(pulsingActive)
        {
            stopStartPulsing->setText(tr("Pulsing is now ON: Set Pulsing OFF"));
            stopStartPulsing->setStyleSheet("background-color: #ccffcc");
        }
        else
        {
            stopStartPulsing->setText(tr("Pulsing in now OFF: Set Pulsing ON"));
            stopStartPulsing->setStyleSheet("background-color: #ffb2b2");
        }
        stopStartPulsing->setPalette(*stopStartPalette);
        connect(stopStartPulsing,SIGNAL(clicked()),this,SLOT(stopStartPressed()));

        recordBeamOn = new QCheckBox();
        recordBeamOn->setChecked(1);
        connect(recordBeamOn,SIGNAL(stateChanged(int)),this,SLOT(changeRecordBeamOn(int)));

        recordBeamOff = new QCheckBox();
        recordBeamOff->setChecked(1);
        connect(recordBeamOff,SIGNAL(stateChanged(int)),this,SLOT(changeRecordBeamOff(int)));

        //Placing all of the above
        cl->addWidget(setPulsingH,                  1,0,1,1);
        cl->addWidget(setPulsingM,                  1,1,1,1);
        cl->addWidget(setPulsingS,                  1,2,1,1);
        cl->addWidget(setPulsingMs,                 1,3,1,1);
        cl->addWidget(pulsingManual,                2,0,1,2);
        cl->addWidget(new QLabel("Time until pulsing change"),              2,2,1,1);
        cl->addWidget(timeElapsedLabel,             2,3,1,1);
        cl->addWidget(stopStartPulsing,             3,0,1,4);
        cl->addWidget(new QLabel("Record data when the beam is ON"),        4,0,1,1);
        cl->addWidget(recordBeamOn,                 4,1,1,1);
        cl->addWidget(new QLabel("Record data when the beam is OFF"),       4,2,1,1);
        cl->addWidget(recordBeamOff,                4,3,1,1);

        container->setLayout(cl);
    }

    //Placing everything in the plugin window
    l->addWidget(container,0,0,1,1);
}

AbstractPlugin::AttributeMap Pulsing::getPulsingAttributeMap () {
    AbstractPlugin::AttributeMap attrs;
    return attrs;
}

AbstractPlugin::AttributeMap Pulsing::getAttributeMap () const {
    return getPulsingAttributeMap ();
}

AbstractPlugin::Attributes Pulsing::getAttributes () const {
    return attrs_;
}

void Pulsing::pulsingInput()
{
    //Set the time at which the beam is set on or off
    pulsingtime=(setPulsingH->value())*3600*1000+(setPulsingM->value())*60*1000+(setPulsingS->value())*1000+setPulsingMs->value();
}

void Pulsing::stopBeam()
{
    //Stop the beam
    modules = *ModuleManager::ref ().list ();
    AbstractInterface *iface = modules[0]->getInterface ();
    iface->setOutput3(1);

    beamStatus=0;
    pulsingManual->setText(tr("Beam is now OFF: Set Beam On"));
    RunManager::ptr()->changeBeamStatus(beamStatus);
    setInterfaceOutput->stop();
}

void Pulsing::sendInterfaceSignal()
{
    //Stop or start the beam
    modules = *ModuleManager::ref ().list ();
    AbstractInterface *iface = modules[0]->getInterface ();
    iface->setOutput3(beamStatus);

    //Change Manual Pulsing button text
    if(beamStatus)
    {
        beamStatus=0;
        pulsingManual->setText("Beam is now OFF: Set Beam On");
    }
    else
    {
        beamStatus=1;
        pulsingManual->setText("Beam is now ON: Set Beam Off");
    }

    RunManager::ptr()->changeBeamStatus(beamStatus);

    //Restart timer
    setInterfaceOutput->start(pulsingtime);
    elapsedTime.restart();
}

void Pulsing::runStartingEvent()
{
    beamStatus=0;
    sendInterfaceSignal();
    elapsedTime.start();
    setInterfaceOutput->start(pulsingtime);
    RunManager::ptr()->changeBeamStatus(beamStatus);
}

void Pulsing::pulsingButtonPressed()
{
    if(pulsingActive)
    {
        //Change the beam status
        modules = *ModuleManager::ref ().list ();
        AbstractInterface *iface = modules[0]->getInterface ();
        iface->setOutput3(beamStatus);

        //Change the button text
        if(beamStatus)
        {
            beamStatus=0;
            pulsingManual->setText("Beam is now OFF: Set Beam On");
        }
        else
        {
            beamStatus=1;
            pulsingManual->setText("Beam is now ON: Set Beam Off");
        }

        RunManager::ptr()->changeBeamStatus(beamStatus);

        //Restart timer
        setInterfaceOutput->start(pulsingtime);
        elapsedTime.restart();
    }
}

void Pulsing::stopStartPressed()
{
    //Change the beam status
    modules = *ModuleManager::ref ().list ();
    AbstractInterface *iface = modules[0]->getInterface ();
    iface->setOutput3(1);

    //Change the button text
    if(pulsingActive)
    {
        pulsingActive=0;
        stopStartPulsing->setText(tr("Pulsing in now OFF: Set Pulsing ON"));
        stopStartPulsing->setStyleSheet("background-color: #ffb2b2");
        pulsingManual->setText("Pulsing is STOPPED");
        setInterfaceOutput->stop();
    }
    else
    {
        pulsingActive=1;
        stopStartPulsing->setText(tr("Pulsing is now ON: Set Pulsing OFF"));
        pulsingManual->setText("Beam is now ON: Set Beam Off");
        stopStartPulsing->setStyleSheet("background-color: #ccffcc");
        setInterfaceOutput->start(pulsingtime);
        elapsedTime.restart();
    }

    RunManager::ptr()->changeBeamStatus(1);
}

void Pulsing::applySettings(QSettings *settings)
{
    QString set;
    settings->beginGroup(getName());
    set = "pulsingH";   if(settings->contains(set)) setPulsingH->setValue(settings->value(set).toInt());
    set = "pulsingM";   if(settings->contains(set)) setPulsingM->setValue(settings->value(set).toInt());
    set = "pulsingS";   if(settings->contains(set)) setPulsingS->setValue(settings->value(set).toInt());
    set = "pulsingMs";   if(settings->contains(set)) setPulsingMs->setValue(settings->value(set).toInt());
    set = "beamOnRecord"; if(settings->contains(set)) recordBeamOn->setChecked(settings->value(set).toInt());
    set = "beamOffRecord"; if(settings->contains(set)) recordBeamOff->setChecked(settings->value(set).toInt());
    settings->endGroup();
}

void Pulsing::saveSettings(QSettings* settings)
{
    //Save the settings
    if(settings == NULL)
    {
        std::cout << getName().toStdString() << ": no settings file" << std::endl;
        return;
    }
    else
    {
        std::cout << getName().toStdString() << " saving settings...";
        settings->beginGroup(getName());
            settings->setValue("pulsingH",setPulsingH->value());
            settings->setValue("pulsingM",setPulsingM->value());
            settings->setValue("pulsingS",setPulsingS->value());
            settings->setValue("pulsingMs",setPulsingMs->value());
            settings->setValue("beamOnRecord",recordBeamOn->checkState());
            settings->setValue("beamOffRecord",recordBeamOff->checkState());
        settings->endGroup();
        std::cout << " done" << std::endl;
    }
}

void Pulsing::updateInterface()
{
    int passed=elapsedTime.elapsed();
    int interval=setInterfaceOutput->interval();
    int diff=interval-passed;
    if(diff>0)
        timeElapsedLabel->setText(tr("%1:%2:%3").arg(diff/1000/60/60,2,10,QChar('0')).arg(diff/1000/60%60,2,10,QChar('0')).arg(diff/1000%60,2,10,QChar('0')));
    else
        timeElapsedLabel->setText(tr("00:00:00"));
}

void Pulsing::changeRecordBeamOn(int state)
{
    beamOnRecord=state/2;
}

void Pulsing::changeRecordBeamOff(int state)
{
    beamOffRecord=state/2;
}

void Pulsing::process () {

}
