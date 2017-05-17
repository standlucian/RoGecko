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

#include "multiplecachehistogramplugin.h"
#include "baseplugin.h"
#include "pluginmanager.h"
#include "runmanager.h"
#include "pluginconnectorqueued.h"
#include "math.h"
#include <QFont>
#include <QGridLayout>
#include <QStackedWidget>
#include <QLabel>
#include <QComboBox>
#include <QSettings>


static PluginRegistrar registrar ("Multiplecachehistogramplugin", MultipleCacheHistogramPlugin::create, AbstractPlugin::GroupCache, MultipleCacheHistogramPlugin::getMCHPAttributeMap());

MultipleCacheHistogramPlugin::MultipleCacheHistogramPlugin(int _id, QString _name, const Attributes &_attrs)
    : BasePlugin(_id, _name),
    attribs_ (_attrs),
    binWidth(1),
    secsToTimeout(1),
    scheduleReset(true),
    calibrated(0)
{
    createSettings(settingsLayout);

    //Checking the number of inputs is valid.
    if (ninputs <= 0 || ninputs > 256) {
        ninputs = 256;
        std::cout << _name.toStdString () << ": nofInputs invalid. Setting to 256." << std::endl;
    }

    //Creating the standard inputs
    for(int n = 0; n < ninputs; n++)
    {
        addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,QString("in %1").arg(n)));
    }

    //Creating another set of inputs if the user requests to veto the data with the BGOs
    if(BGOVeto)
    {
        for(int n = 0; n < ninputs/2; n++)
        {
            addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,QString("Veto %1").arg(n)));
        }
    }

    //Creating another set of inputs if the user requests to have a second time panel
    if(secondTime)
        for(int n=0; n<ninputs/2; n++)
            addConnector(new PluginConnectorQVUint(this,ScopeCommon::in,QString("Extra inputs %1").arg(n)));

    //The plugin needs only one input to have data in order to run
    setNumberOfMandatoryInputs(1);

    //Creating the standard set of plots
    for(int i=0;i<2*ninputs+2;i++)
    {
        mplot[i]->addChannel(0,tr("histogram"),QVector<double>(1,0),
                     QColor(153,153,153),Channel::steps,1);
    }

    //Creating extra plots if the user requests to have a second time panel
    if(secondTime)
        for(int i=2*ninputs+2;i<5*ninputs/2+2;i++)
        {
            mplot[i]->addChannel(0,tr("histogram"),QVector<double>(1,0),
                         QColor(153,153,153),Channel::steps,1);
        }

    //Starting a timer that updates all the visuals. It takes a ms value, thus the *1000
    secondTimer = new QTimer();
    secondTimer->start(secsToTimeout*1000);
    connect(secondTimer,SIGNAL(timeout()),this,SLOT(updateVisuals()));

    std::cout << "Instantiated MultipleCacheHistogram" << std::endl;
}

MultipleCacheHistogramPlugin::~MultipleCacheHistogramPlugin()
{
   //Destructor of the class. Deallocates the plots and the plot window.
   for(int i=0;i<mplot.size();i++)
   {
       mplot[i]->close();
       delete mplot[i];
       mplot[i]=NULL;
   }
   Multiplewindow->close();
   delete Multiplewindow;
   Multiplewindow=NULL;
}

AbstractPlugin::AttributeMap MultipleCacheHistogramPlugin::getMCHPAttributeMap() {
    AbstractPlugin::AttributeMap attrs;
    attrs.insert ("nofInputs", QVariant::Int);
    attrs.insert ("BGO Veto",  QVariant::Bool);
    attrs.insert ("Third row of inputs", QVariant::Bool);
    return attrs;
}

AbstractPlugin::Attributes MultipleCacheHistogramPlugin::getAttributes () const { return attribs_;}

void MultipleCacheHistogramPlugin::recalculateBinWidth()
{
    double calibxmax=0, xmax=0;

        if(calibrated)
        {
            for(int i=0;i<ninputs/2;i++)
            {
                calibxmax=0;
                for(int j=0;j<calibcoef[0][0];j++)
                {
                    calibxmax=calibxmax+pow(conf.nofBins,j)*calibcoef[0][j+1];
                }
                if(xmax<calibxmax) xmax=calibxmax;
            binWidth = xmax/((double)conf.nofBins);
            }
        }
        else
            binWidth = 1;
}

void MultipleCacheHistogramPlugin::applySettings(QSettings* settings)
{
    QString set;
    settings->beginGroup(getName());
        set = "nofBins"; if(settings->contains(set)) conf.nofBins = settings->value(set).toInt();
        set = "nofTBins"; if(settings->contains(set)) conf.nofTBins = settings->value(set).toInt();
        set = "nofSBins"; if(settings->contains(set)) conf.nofSBins = settings->value(set).toInt();
        set = "calName"; if(settings->contains(set)) conf.calName = settings->value(set).toString();
        set = "secsToTimeout"; if(settings->contains(set)) secsToTimeout = settings->value(set).toInt();
        settings->endGroup();

    calibNameEdit->setText(conf.calName);
    updateSpeedSpinner->setValue(secsToTimeout);
    nofBinsBox->setCurrentIndex(nofBinsBox->findData(conf.nofBins,Qt::UserRole));
    nofTBinsBox->setCurrentIndex(nofTBinsBox->findData(conf.nofTBins,Qt::UserRole));
    nofSBinsBox->setCurrentIndex(nofSBinsBox->findData(conf.nofSBins,Qt::UserRole));
    }

void MultipleCacheHistogramPlugin::saveSettings(QSettings* settings)
{
    if(settings == NULL)
    {
        std::cout << getName().toStdString() << ": no settings file" << std::endl;
        return;
    }
    else
    {
        std::cout << getName().toStdString() << " saving settings...";
        settings->beginGroup(getName());
            settings->setValue("nofBins",conf.nofBins);
            settings->setValue("nofTBins",conf.nofTBins);
            settings->setValue("nofSBins",conf.nofSBins);
            settings->setValue("calName",conf.calName);
            settings->setValue("secsToTimeout",secsToTimeout);
            settings->endGroup();
        std::cout << " done" << std::endl;
    }
}

void MultipleCacheHistogramPlugin::createSettings(QGridLayout * l)
{
    Attributes _attrib=getAttributes();
    bool ok;
    ninputs=_attrib.value ("nofInputs", QVariant (4)).toInt (&ok);
    BGOVeto=_attrib.value ("BGO Veto", QVariant (4)).toBool ();
    secondTime=_attrib.value ("Third row of inputs",QVariant (4)).toBool ();
    int rowcol=sqrt(ninputs/2);
    if(rowcol*rowcol!=ninputs/2) rowcol++;
        totalCache.resize(2);

    if(secondTime)
    {
        mplot.resize(5*ninputs/2+2);
        cache.resize(3*ninputs/2);
        plotCounts.resize(5*ninputs/2);
        prevCount.resize(5*ninputs/2);
    }
    else
    {
        mplot.resize(2*ninputs+2);
        cache.resize(ninputs);
        plotCounts.resize(ninputs*2);
        prevCount.resize(2*ninputs);
    }

    rawcache.resize(ninputs);
    calibcoef.resize(ninputs/2);

    for(int i=0;i<ninputs/2;i++)
    {
        calibcoef[i].resize(3);
        calibcoef[i][0]=2;
        calibcoef[i][1]=0;
        calibcoef[i][2]=1;

    }

    // Plugin specific code here
    QWidget* container = new QWidget();
    {
        QGridLayout* cl = new QGridLayout;

        previewButton = new QPushButton(tr("Show..."));
        resetButton = new QPushButton(tr("Reset"));
        resetButton2 = new QPushButton("Clear Spectra");
        resetButton2->setFont(QFont("Helvetica", 10, QFont::Bold));
        if (secondTime)
            plotCountsLabel.resize(5*ninputs/2);
        else  plotCountsLabel.resize(2*ninputs);
        for(int i=0;i<2*ninputs;i++)
        {
            plotCountsLabel[i] = new QLabel (tr ("0"));
            plotCountsLabel[i]->setAlignment(Qt::AlignCenter);
        }

        if(secondTime)
        {
            for(int i=2*ninputs;i<5*ninputs/2;i++)
            {
                plotCountsLabel[i] = new QLabel (tr ("0"));
                plotCountsLabel[i]->setAlignment(Qt::AlignCenter);
            }
        }

        totalCountsLabel = new QLabel (tr ("0"));
        totalCountsLabel->setAlignment(Qt::AlignCenter);

        for(int i=0;i<2*ninputs+2;i++)
            mplot[i]=new plot2d(0,QSize(640,480),i);

        if(secondTime)
            for(int i=2*ninputs+2;i<5*ninputs/2+2;i++)
                mplot[i]=new plot2d(0,QSize(640,480),i);

        Multiplewindow= new QWidget;
        QGridLayout *mainLayout = new QGridLayout(Multiplewindow);
        QWidget *topWidget = new QWidget();
        QGridLayout *topLayout = new QGridLayout(topWidget);
        QStackedWidget *stack = new QStackedWidget();
        QWidget *Window1= new QWidget();
        QWidget *Window2= new QWidget();
        QWidget *Window3= new QWidget();
        QWidget *Window4= new QWidget();
        QWidget *Window5= new QWidget();
        QWidget *Window6= new QWidget();
        QGridLayout *multiplelayout1= new QGridLayout(Window1);
        QGridLayout *multiplelayout2= new QGridLayout(Window2);
        QGridLayout *multiplelayout3= new QGridLayout(Window3);
        QGridLayout *multiplelayout4= new QGridLayout(Window4);
        QGridLayout *multiplelayout5= new QGridLayout(Window5);
        QGridLayout *multiplelayout6= new QGridLayout(Window6);
        QComboBox *setPlotStyle=new QComboBox();
            setPlotStyle->addItem("Normal");
            setPlotStyle->addItem("Logarithmic");
            setPlotStyle->addItem("Square root");

        QComboBox *pageSelect = new QComboBox();
            pageSelect->addItem("Totals");
            pageSelect->addItem("Energy");
            pageSelect->addItem("Time");
            if(secondTime)
                pageSelect->addItem("Extra Inputs");
            pageSelect->addItem("Raw Energy");
            pageSelect->addItem("Raw Time");
            connect(pageSelect, SIGNAL(activated(int)), stack, SLOT(setCurrentIndex(int)));
        topWidget->setMaximumWidth(480);
        topLayout->addWidget(pageSelect,0,0,1,1);
        topLayout->addWidget(resetButton2,0,1,1,1);
        topLayout->addWidget(setPlotStyle,0,2,1,1);
        mainLayout->addWidget(topWidget,0,0,1,1);

        for(int i=0;i<rowcol;i++)
            for(int j=0;j<rowcol;j++)
            {
                multiplelayout1->addWidget(mplot[i*rowcol+j],12*i+2,j+1,11,1);
                multiplelayout1->addWidget(plotCountsLabel[ninputs+i*rowcol+j],12*i+13,j+1,1,1);
                if(i*rowcol+j+1>=ninputs/2) {i=rowcol; j=rowcol;}
            }

        for(int i=0;i<rowcol;i++)
            for(int j=0;j<rowcol;j++)
            {
                multiplelayout2->addWidget(mplot[ninputs/2+i*rowcol+j],12*i+2,j+1,11,1);
                multiplelayout2->addWidget(plotCountsLabel[3*ninputs/2+i*rowcol+j],12*i+13,j+1,1,1);
                if(i*rowcol+j+1>=ninputs/2) {i=rowcol; j=rowcol;}
            }

        for(int i=0;i<rowcol;i++)
            for(int j=0;j<rowcol;j++)
            {
                multiplelayout3->addWidget(mplot[ninputs+i*rowcol+j],12*i+2,j+1,11,1);
                multiplelayout3->addWidget(plotCountsLabel[i*rowcol+j],12*i+13,j+1,1,1);
                if(i*rowcol+j+1>=ninputs/2) {i=rowcol; j=rowcol;}
            }

        for(int i=0;i<rowcol;i++)
            for(int j=0;j<rowcol;j++)
            {
                multiplelayout4->addWidget(mplot[3*ninputs/2+i*rowcol+j],12*i+2,j+1,11,1);
                multiplelayout4->addWidget(plotCountsLabel[ninputs/2+i*rowcol+j],12*i+13,j+1,1,1);
                if(i*rowcol+j+1>=ninputs/2) {i=rowcol; j=rowcol;}
            }

        if(secondTime)
            for(int i=0;i<rowcol;i++)
                for(int j=0;j<rowcol;j++)
                {
                    multiplelayout6->addWidget(mplot[2*ninputs+2+i*rowcol+j],12*i+2,j+1,11,1);
                    multiplelayout6->addWidget(plotCountsLabel[2*ninputs+i*rowcol+j],12*i+13,j+1,1,1);
                    if(i*rowcol+j+1>=ninputs/2) {i=rowcol; j=rowcol;}
                }

        multiplelayout5->addWidget(mplot[2*ninputs],0,0,1,1);
        multiplelayout5->addWidget(mplot[2*ninputs+1],1,0,1,1);

        multiplelayout1->setHorizontalSpacing(15);
        multiplelayout2->setHorizontalSpacing(15);
        multiplelayout3->setHorizontalSpacing(15);
        multiplelayout4->setHorizontalSpacing(15);
        multiplelayout6->setHorizontalSpacing(15);
        stack->addWidget(Window5);
        stack->addWidget(Window3);
        stack->addWidget(Window4);
        if(secondTime)
            stack->addWidget(Window6);
        stack->addWidget(Window1);
        stack->addWidget(Window2);
        mainLayout->addWidget(stack,1,0,1,1);
        mainLayout->addWidget(totalCountsLabel,2,0,1,1);

        Multiplewindow->setWindowTitle(getName());
        Multiplewindow->setMinimumSize(QSize(1280,720));

        QLabel* updateSpeedLabel = new QLabel(tr("Update Speed (s)"));
        QLabel* nofBinsLabel = new QLabel(tr("Number of Bins"));
        QLabel* nofTBinsLabel = new QLabel(tr("Number of Time Bins"));
        QLabel* nofSBinsLabel = new QLabel(tr("Numbers of extra input Bins"));
        QLabel* calibLabel = new QLabel(tr("Calibration file name:"));

        calibNameEdit = new QLineEdit();
        calibNameButton = new QPushButton(tr("..."));

         // In milliseconds
        updateSpeedSpinner = new QSpinBox();
        updateSpeedSpinner->setMinimum(1);
        updateSpeedSpinner->setMaximum(60);
        updateSpeedSpinner->setSingleStep(1);
        updateSpeedSpinner->setValue(secsToTimeout);

        nofBinsBox = new QComboBox();
        nofBinsBox->addItem("1024",1024);
        nofBinsBox->addItem("2048",2048);
        nofBinsBox->addItem("4096",4096);
        nofBinsBox->addItem("8192",8192);
        nofBinsBox->addItem("16384",16384);
        nofBinsBox->addItem("32768",32768);
        nofBinsBox->addItem("65536",65536);
        nofBinsBox->setCurrentIndex(nofBinsBox->findData(conf.nofBins,Qt::UserRole));

        nofTBinsBox = new QComboBox();
        nofTBinsBox->addItem("1024",1024);
        nofTBinsBox->addItem("2048",2048);
        nofTBinsBox->addItem("4096",4096);
        nofTBinsBox->addItem("8192",8192);
        nofTBinsBox->addItem("16384",16384);
        nofTBinsBox->addItem("32768",32768);
        nofTBinsBox->addItem("65536",65536);
        nofTBinsBox->setCurrentIndex(nofTBinsBox->findData(conf.nofTBins,Qt::UserRole));

        nofSBinsBox = new QComboBox();
        nofSBinsBox->addItem("1024",1024);
        nofSBinsBox->addItem("2048",2048);
        nofSBinsBox->addItem("4096",4096);
        nofSBinsBox->addItem("8192",8192);
        nofSBinsBox->addItem("16384",16384);
        nofSBinsBox->addItem("32768",32768);
        nofSBinsBox->addItem("65536",65536);
        nofSBinsBox->setCurrentIndex(nofSBinsBox->findData(conf.nofSBins,Qt::UserRole));

        numCountsLabel = new QLabel (tr ("0"));

        connect(previewButton,SIGNAL(clicked()),this,SLOT(previewButtonClicked()));
        connect(resetButton,SIGNAL(clicked()),this,SLOT(resetButtonClicked()));
        connect(resetButton2,SIGNAL(clicked()),this,SLOT(resetButtonClicked()));
        connect(updateSpeedSpinner,SIGNAL(valueChanged(int)),this,SLOT(setTimerTimeout(int)));
        connect(nofBinsBox,SIGNAL(currentIndexChanged(int)),this,SLOT(nofBinsChanged(int)));
        connect(nofTBinsBox,SIGNAL(currentIndexChanged(int)),this,SLOT(nofTBinsChanged(int)));
        connect(nofSBinsBox,SIGNAL(currentIndexChanged(int)),this,SLOT(nofSBinsChanged(int)));
        connect(calibNameButton,SIGNAL(clicked()),this,SLOT(calibNameButtonClicked()));
        connect(calibNameEdit, SIGNAL(textChanged(QString)), this, SLOT(findCalibName(QString)));
        connect(setPlotStyle, SIGNAL(currentIndexChanged(int)), this, SLOT(modifyPlotState(int)));
        for(int i=0;i<2*ninputs+2;i++)
            connect(mplot[i], SIGNAL(histogramCleared(unsigned int,unsigned int)), this, SLOT(resetSingleHistogram(unsigned int,unsigned int)));
        for(int i=0;i<2*ninputs+2;i++)
            connect(mplot[i], SIGNAL(changeZoomForAll(unsigned int, double, double)), this, SLOT(changeBlockZoom(unsigned int, double, double)));

        if(secondTime)
        {
            for(int i=2*ninputs+2;i<5*ninputs/2+2;i++)
                connect(mplot[i], SIGNAL(histogramCleared(unsigned int,unsigned int)), this, SLOT(resetSingleHistogram(unsigned int,unsigned int)));
            for(int i=2*ninputs+2;i<5*ninputs/2+2;i++)
                connect(mplot[i], SIGNAL(changeZoomForAll(unsigned int, double, double)), this, SLOT(changeBlockZoom(unsigned int, double, double)));
        }
        cl->addWidget(previewButton,0,0,1,2);
        cl->addWidget(resetButton,  0,2,1,2);

        cl->addWidget(new QLabel ("Counts in histogram:"), 1, 0, 1, 1);
        cl->addWidget(numCountsLabel, 1, 1, 1, 3);
        cl->addWidget(updateSpeedLabel,  1,2,1,1);
        cl->addWidget(updateSpeedSpinner,1,3,1,1);

        cl->addWidget(nofBinsLabel,2,0,1,1);
        cl->addWidget(nofBinsBox,  2,1,1,1);
        cl->addWidget(nofTBinsLabel,2,2,1,1);
        cl->addWidget(nofTBinsBox,  2,3,1,1);

        if(secondTime)
        {
            cl->addWidget(nofSBinsLabel,3,0,1,1);
            cl->addWidget(nofSBinsBox,  3,1,1,1);
        }

        cl->addWidget(calibLabel,     4,0,1,1);
        cl->addWidget(calibNameEdit,      4,1,1,2);
        cl->addWidget(calibNameButton,    4,3,1,1);

        container->setLayout(cl);
    }

    // End

    l->addWidget(container,0,0,1,1);
}

void MultipleCacheHistogramPlugin::changeBlockZoom(unsigned int id0, double x, double width)
{
    int id = (int) id0;
    if(id<ninputs/2)
    {
        for(int i=0;i<ninputs/2;i++)
            mplot[i]->setZoom(x,width);
    }
    else if (id<ninputs)
    {
        for(int i=ninputs/2;i<ninputs;i++)
            mplot[i]->setZoom(x,width);
    }
    else if((id<3*ninputs/2)||(id==2*ninputs))
    {
        for(int i=ninputs;i<3*ninputs/2;i++)
            mplot[i]->setZoom(x,width);
        mplot[2*ninputs]->setZoom(x,width);
    }
    else if((id<2*ninputs)||(id==2*ninputs+1))
    {
        for(int i=3*ninputs/2;i<2*ninputs;i++)
            mplot[i]->setZoom(x,width);
        mplot[2*ninputs+1]->setZoom(x,width);
    }
    else if(secondTime)
    {
        for(int i=2*ninputs+2;i<5*ninputs/2+2;i++)
            mplot[i]->setZoom(x,width);
    }
}

void MultipleCacheHistogramPlugin::resetSingleHistogram(unsigned int,unsigned int c)
{
    int b = (int) c;
    if(b<ninputs)
    {
        rawcache[b].clear();
        rawcache[b].fill(0, conf.nofBins);
    }
    else if(b<ninputs*2)
    {
        cache[b-ninputs].clear();
        cache[b-ninputs].fill(0, conf.nofBins);
    }
    else if(b==ninputs*2)
    {
        totalCache[0].clear();
        totalCache[0].fill(0, conf.nofBins);
    }
    else if(b==ninputs*2+1)
    {
        totalCache[1].clear();
        totalCache[1].fill(0, conf.nofBins);
    }
    else if((secondTime)&&(b<5*ninputs/2+2))
    {
        cache[b-ninputs-2].clear();
        cache[b-ninputs-2].fill(0, conf.nofSBins);
    }

    mplot[b]->resetBoundaries(0);
}

void MultipleCacheHistogramPlugin::previewButtonClicked()
{
    if(Multiplewindow->isHidden())
    {
         Multiplewindow->show();
    }
    else
    {
         Multiplewindow->hide();
    }
}

void MultipleCacheHistogramPlugin::nofBinsChanged(int newValue)
{
     conf.nofBins = nofBinsBox->itemData(newValue,Qt::UserRole).toInt();
}

void MultipleCacheHistogramPlugin::nofTBinsChanged(int newValue)
{
     conf.nofTBins = nofTBinsBox->itemData(newValue,Qt::UserRole).toInt();
}

void MultipleCacheHistogramPlugin::nofSBinsChanged(int newValue)
{
     conf.nofSBins = nofSBinsBox->itemData(newValue,Qt::UserRole).toInt();
}

void MultipleCacheHistogramPlugin::calibNameButtonClicked()
{
     setCalibName(QFileDialog::getOpenFileName(this,tr("Choose calibration file name"), "/home",tr("Text (*.cal)")));
}

void MultipleCacheHistogramPlugin::setCalibName(QString _runName)
{
    calibNameEdit->setText(_runName);
    conf.calName=_runName;
}

void MultipleCacheHistogramPlugin::modifyPlotState(int state)
{
    for(int i=0;i<2*ninputs+2;i++)
        mplot[i]->setPlotState(state);
    if(secondTime)
        for(int i=2*ninputs+2;i<5*ninputs/2+2;i++)
            mplot[i]->setPlotState(state);
}

void MultipleCacheHistogramPlugin::findCalibName(QString newValue)
{
    QByteArray ba = newValue.toLatin1();
    const char *c_str2 = ba.data();
    calibration.open (c_str2);
    int dump, det;
    if(calibration.is_open())
    {
        calibrated=1;

        while(!calibration.eof())
        {
            calibration>>dump;
            calibration>>det;
            calibration>>calibcoef[det][0];
            calibcoef[det].resize(1+calibcoef[det][0]);
            for(int j=0;j<calibcoef[det][0];j++)
            {
                calibration>>calibcoef[det][j+1];
            }
        }
    }
    else calibrated=0;

    recalculateBinWidth();
    for(int i=ninputs;i<3*ninputs/2;i++)
        mplot[i]->setCalibration(binWidth);
    mplot[2*ninputs]->setCalibration(binWidth);

    calibration.close();
}

void MultipleCacheHistogramPlugin::updateVisuals()
{
    int totalCounts=0, totalRate=0;

    for(int i=0;i<ninputs;i++)
    {
            if(!rawcache[i].empty())
                mplot[i]->getChannelById(0)->setData(rawcache[i]);
            if(!cache[i].empty())
                mplot[ninputs+i]->getChannelById(0)->setData(cache[i]);
    }

    if(secondTime)
        for(int i=ninputs;i<3*ninputs/2;i++)
        {
            if(!cache[i].empty())
                mplot[ninputs+2+i]->getChannelById(0)->setData(cache[i]);
        }

    if(!totalCache[0].empty())
                mplot[2*ninputs]->getChannelById(0)->setData(totalCache[0]);
    if(!totalCache[1].empty())
                mplot[2*ninputs+1]->getChannelById(0)->setData(totalCache[1]);

    Multiplewindow->update();
    numCountsLabel->setText(tr("%1").arg(nofCounts));

    for(int i=0;i<ninputs*2;i++)
    {
        int evRate = (plotCounts[i]-prevCount[i])/secsToTimeout;
        if(i<ninputs/2)
        {
            totalCounts+=plotCounts[i];
            totalRate+=evRate;
        }
        plotCountsLabel[i]->setText(tr("%1 events     %2 ev/s").arg(plotCounts[i]).arg(evRate));
        prevCount[i]=plotCounts[i];
    }

    if(secondTime)
    {
        for(int i=ninputs*2;i<5*ninputs/2;i++)
        {
            int evRate = (plotCounts[i]-prevCount[i])/secsToTimeout;
            plotCountsLabel[i]->setText(tr("%1 events     %2 ev/s").arg(plotCounts[i]).arg(evRate));
            prevCount[i]=plotCounts[i];
        }
    }

    uint64_t nofTriggers = RunManager::ptr()->sendTriggers();
    uint64_t trigspersec = RunManager::ptr()->sendTriggerRate();

    totalCountsLabel->setText(tr("TOTALS: %1 events     %2 ev/s     TRIGGERS: %3 triggers      %4 trig/s").arg(totalCounts).arg(totalRate).arg(nofTriggers).arg(trigspersec));
}

void MultipleCacheHistogramPlugin::resetButtonClicked()
{
  scheduleReset = true;
}

void MultipleCacheHistogramPlugin::setTimerTimeout(int msecs)
{
    secsToTimeout = msecs;
    secondTimer->setInterval(1000*msecs);
}

bool MultipleCacheHistogramPlugin::notVeto(uint32_t time, int det)
{
    if(BGOVeto)
    {
        if(vetoData[det].size()>readPointer[ninputs+det])
        {
            while(time+10>vetoData[det][readPointer[ninputs+det]+1])
            {
                if(time-vetoData[det][readPointer[ninputs+det]+1]<10)
                {
                    readPointer[ninputs+det]+=2;
                    return 0;
                }
                else readPointer[ninputs+det]+=2;
                if(vetoData[det].size()<readPointer[ninputs+det])
                    break;
            }
        }
    }

    return 1;
}

void MultipleCacheHistogramPlugin::writeEnergyOnly(int32_t energy, int det)
{
    if(energy > 0 && energy < conf.nofBins)
    {
        rawcache[det][energy]++;
        plotCounts[det+ninputs]++;
    }
    readPointer[det]+=2;
}

void MultipleCacheHistogramPlugin::writeTimeOnly(int32_t time, int det)
{
    if(time > 0 && time < conf.nofTBins)
    {
        rawcache[det+ninputs/2][time]++;
        plotCounts[det+3*ninputs/2] ++;
    }
    readPointer[det+ninputs/2]+=2;
}

/*!
* @fn void CacheHistogramPlugin::userProcess()
* @brief For plugin specific processing code
*
* @warning This function must ONLY be called from the plugin thread;
* @variable pdata wants to be a vector<double> of new values to be inserted into the histogram
*/
void MultipleCacheHistogramPlugin::userProcess()
{
    QVector< QVector<uint32_t> > idata;
    if(BGOVeto)
    {
        vetoData.resize(ninputs/2);
    }

    if(secondTime)
    {
        secondTimeData.resize(ninputs/2);
    }
    idata.resize(ninputs);
    readPointer.fill(0);
    uint32_t stampEnergy;
    uint32_t stampTime;
    int binEnergy,bin3=0, binTime;
    double bin2=0, randomized, bin4;

    for(int i=0;i<ninputs;i++)
    {
        idata[i] = inputs->at(i)->getData().value< QVector<uint32_t> > ();
    }

    if(BGOVeto)
        for(int i=0;i<ninputs/2;i++)
        {
            vetoData[i] = inputs->at(i+ninputs)->getData().value< QVector<uint32_t> > ();
        }

    if(secondTime)
    {
        if(BGOVeto) secondTimePosition=3*ninputs/2;
        else secondTimePosition=ninputs;
        for(int i=0;i<ninputs/2;i++)
        {
            secondTimeData[i] = inputs->at(i+secondTimePosition)->getData().value< QVector<uint32_t> > ();
        }
    }

    if(scheduleReset)
    {
        for(int i=0;i<ninputs/2;i++)
        {
            cache[i].clear();
            cache[i].fill(0, conf.nofBins);
            rawcache[i].clear();
            rawcache[i].fill(0, conf.nofBins);
        }
        for(int i=ninputs/2;i<ninputs;i++)
        {
            cache[i].clear();
            cache[i].fill(0, conf.nofTBins);
            rawcache[i].clear();
            rawcache[i].fill(0, conf.nofTBins);
        }
        if(secondTime)
        {
            for(int i=ninputs;i<3*ninputs/2;i++)
            {
                cache[i].clear();
                cache[i].fill(0, conf.nofSBins);
            }
            for(int i=2*ninputs+2;i<5*ninputs/2+2;i++)
                mplot[i]->resetBoundaries(0);
        }
        totalCache[0].clear();
        totalCache[0].fill(0, conf.nofBins);
        totalCache[1].clear();
        totalCache[1].fill(0, conf.nofTBins);
        recalculateBinWidth();
        scheduleReset = false;
        for(int i=0;i<2*ninputs+2;i++)
            mplot[i]->resetBoundaries(0);
    }

    for(int i=0;i<ninputs/2;i++)
    {
        if((int)(cache[i].size()) != conf.nofBins) cache[i].resize(conf.nofBins);
        if((int)(rawcache[i].size()) != conf.nofBins) rawcache[i].resize(conf.nofBins);
    }
    for(int i=ninputs/2;i<ninputs;i++)
    {
        if((int)(cache[i].size()) != conf.nofTBins) cache[i].resize(conf.nofTBins);
        if((int)(rawcache[i].size()) != conf.nofTBins) rawcache[i].resize(conf.nofTBins);
    }
    if(secondTime)
        for(int i=ninputs;i<3*ninputs/2;i++)
        {
            if((int)(cache[i].size()) != conf.nofSBins) cache[i].resize(conf.nofSBins);
        }

    if((int)(totalCache[0].size()) != conf.nofBins) totalCache[0].resize(conf.nofBins);
    if((int)(totalCache[1].size()) != conf.nofTBins) totalCache[1].resize(conf.nofTBins);

    // Add data to histogram
    for(int i=0;i<ninputs/2;i++)
    {
        while((readPointer[i]<idata[i].size())&&(readPointer[i+ninputs/2]<idata[i+ninputs/2].size()))
        {
            stampEnergy = idata[i][readPointer[i]+1];
            stampTime   = idata[i+ninputs/2][readPointer[i+ninputs/2]+1];
            binEnergy = idata[i][readPointer[i]];
            binTime= idata[i+ninputs/2][readPointer[i+ninputs/2]];
            if(abs(stampEnergy-stampTime)<100)
            {
                if(notVeto(stampTime,i))
                {
                        if(calibrated)
                        {
                            bin2=0;
                            randomized=((double) rand() / (RAND_MAX));
                            bin4=binEnergy+randomized;
                            for(int j=0;j<calibcoef[i][0];j++)
                                bin2=bin2+pow(bin4,j)*calibcoef[i][j+1];
                            bin3=bin2/binWidth;
                        }
                        else bin3=binEnergy;

                        if(binEnergy > 0 && binEnergy < conf.nofBins)
                        {
                            rawcache[i][binEnergy]++;
                            plotCounts[i+ninputs]++;
                        }
                        if(bin3 > 0 && bin3 < conf.nofBins)
                        {
                            cache[i][bin3] ++;
                            totalCache[0][bin3]++;
                            ++nofCounts;
                            plotCounts[i] ++;
                        }
                        if(binTime > 0 && binTime < conf.nofTBins)
                        {
                            cache[i+ninputs/2][binTime] ++;
                            rawcache[i+ninputs/2][binTime]++;
                            totalCache[1][binTime]++;
                            ++nofCounts;
                            plotCounts[i+ninputs/2] ++;
                            plotCounts[i+3*ninputs/2] ++;
                        }
                        readPointer[i]+=2;
                        readPointer[i+ninputs/2]+=2;
                }
                else { writeEnergyOnly(binEnergy,i); writeTimeOnly(binTime,i);}

            }
            else if (stampEnergy<stampTime)
            {
                writeEnergyOnly(binEnergy,i);
            }
            else if (stampEnergy>stampTime)
            {
                writeTimeOnly(binTime,i);
            }
        }
        while(readPointer[i]<idata[i].size())
        {
            binEnergy = idata[i][readPointer[i]];
            writeEnergyOnly(binEnergy,i);
        }
        while(readPointer[i+ninputs/2]<idata[i+ninputs/2].size())
        {
            binTime= idata[i+ninputs/2][readPointer[i+ninputs/2]];
            writeTimeOnly(binTime,i);
        }

        if(secondTime)
        {
            int kk=0;
            foreach(double datum, secondTimeData[i])
            {
                if(kk%2==0)
                {
                    if(datum < conf.nofSBins && datum >= 0)
                    {
;
                            cache[ninputs+i][datum] ++;
                            plotCounts[2*ninputs+i]++;
                    }
                }
                kk++;
            }
        }
    }
}


void MultipleCacheHistogramPlugin::runStartingEvent () {
    // reset all timers and the histogram before starting anew
    secondTimer->stop();
    scheduleReset = true;
    nofCounts = 0;
    plotCounts.fill(0);
    prevCount.fill(0);
    if(BGOVeto)
        readPointer.resize(3*ninputs/2);
    else readPointer.resize(ninputs);
    recalculateBinWidth();
    for(int i=0;i<ninputs/2;i++)
    {
        cache[i].resize(conf.nofBins);
        cache[i].fill(0, conf.nofBins);
        rawcache[i].resize(conf.nofBins);
        rawcache[i].fill(0, conf.nofBins);
    }
    for(int i=ninputs/2;i<ninputs;i++)
    {
        cache[i].resize(conf.nofTBins);
        cache[i].fill(0, conf.nofTBins);
        rawcache[i].resize(conf.nofTBins);
        rawcache[i].fill(0, conf.nofTBins);
    }
    if(secondTime)
    {
        for(int i=ninputs;i<3*ninputs/2;i++)
        {
            cache[i].resize(conf.nofSBins);
            cache[i].fill(0, conf.nofSBins);
        }
    }
    totalCache[0].resize(conf.nofBins);
    totalCache[0].fill(0, conf.nofBins);
    totalCache[1].resize(conf.nofTBins);
    totalCache[1].fill(0, conf.nofTBins);

   secondTimer->start(secsToTimeout*1000);
}

/*!
\page cachehistogramplg Histogram Cache Plugin
\li <b>Plugin names:</b> \c cachehistogramplugin
\li <b>Group:</b> Cache

\section pdesc Plugin Description
The histogram cache plugin creates histograms from values fed to its input connector.
The computed histogram may be stored to disk periodically.
It is also possible to define a timespan after which the histogram is reset (increasing the index of the file the histogram is stored to).
That way, one can get a separate histogram for every timeslice.

The plugin can also display a plot of the current histogram.

\section attrs Attributes
None

\section conf Configuration
\li <b>Auto reset</b>: Enable or disable automatic reset after given interval
\li <b>Auto reset interval</b>: Interval after which the histogram is reset (in minutes)
\li <b>Auto save</b>: Enable or disable automatic saving after given interval
\li <b>Auto save interval</b>: Interval after which the current histogram is saved (in seconds)
\li <b>From</b>: lower bound of the lowest histogram bin
\li <b>Max Height</b>: Not yet implemented
\li <b>Normalize</b>: Normalizes the histogram to its maximum value (\b Attention: This is still buggy)
\li <b>Number of Bins</b>: Number of bins the range [From..To] is divided into
\li <b>Preview</b>: Shows a live plot of the histogram
\li <b>Reset</b>: Manually reset the histogram. \b Attention: This will NOT create a new file. The histogram will be saved to the current file.
\li <b>To</b>: upper bound of the highest histogram bin
\li <b>Update Speed</b>: Interval between updates of the histogram plot and counter

\section inputs Input Connectors
\li \c in \c &lt;double>: Input for the data to be histogrammed

\section outputs Output Connectors
\li \c fileOut, out \c &lt;double>: Contains the current histogram
*/
