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

#ifndef MULTIPLECACHEHISTOGRAMPLUGIN_H
#define MULTIPLECACHEHISTOGRAMPLUGIN_H

#include <QWidget>
#include <vector>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QTimer>
#include <QCheckBox>
#include <QLineEdit>
#include <QVector>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QDateTime>

#include "baseplugin.h"
#include "plot2d.h"

class QComboBox;
class BasePlugin;
class QLabel;

struct MultipleCacheHistogramPluginConfig
{
    int nofBins, nofTBins, nofSBins;
    QString calName;

    MultipleCacheHistogramPluginConfig()
        : nofBins(8192),nofTBins(8192),nofSBins(8192)
    {}
};

class MultipleCacheHistogramPlugin : public BasePlugin
{
    Q_OBJECT

protected:
    Attributes attribs_;
    double binWidth;
    int secsToTimeout;
    bool scheduleReset;

    QLabel* nofInputsLabel;
    MultipleCacheHistogramPluginConfig conf;

    QComboBox* nofBinsBox;
    QComboBox* nofTBinsBox;
    QComboBox* nofSBinsBox;
    std::vector < QVector<double> > cache;
    std::vector < QVector<double> > scache;
    std::vector < QVector<double> > rawcache;
    std::vector <QVector<double> > totalCache;
    QLabel* totalCountsLabel;

    QSpinBox* updateSpeedSpinner;
    QPushButton* resetButton;
    QPushButton* resetButton2;
    QPushButton* previewButton;
    QLabel* numCountsLabel;
    QLabel* calibLabel;
    std::vector <std::vector <double> > calibcoef;
    std::ifstream calibration;
    bool calibrated;
    QVector< QVector<uint32_t> > vetoData;
    QVector< QVector<uint32_t> > secondTimeData;

    QPushButton* calibNameButton;
    QLineEdit* calibNameEdit;
    int secondTimePosition;

    void setCalibName(QString);
    uint64_t nofCounts;
    QVector <uint64_t> plotCounts;
    QVector <uint64_t> prevCount;
    std::vector <QLabel*> plotCountsLabel;
    int ninputs;
    bool BGOVeto;
    bool secondTime;
    QTimer* secondTimer;
    QWidget* Multiplewindow;
    std::vector <plot2d*> mplot;

    virtual void createSettings(QGridLayout*);

public:
    MultipleCacheHistogramPlugin(int _id, QString _name, const Attributes &attrs);
    static AbstractPlugin *create (int id, const QString &name, const Attributes &attrs) {
        return new MultipleCacheHistogramPlugin (id, name, attrs);
    }

    Attributes getAttributes () const;
    static AttributeMap getMCHPAttributeMap ();

    virtual AbstractPlugin::Group getPluginGroup () { return AbstractPlugin::GroupCache; }
    virtual void applySettings(QSettings*);
    virtual void saveSettings(QSettings*);
    virtual void userProcess();
    virtual ~MultipleCacheHistogramPlugin();
    void recalculateBinWidth();
    virtual void runStartingEvent();
    uint32_t nofInputs;
    QVector <int> readPointer;

private:

public slots:
    void nofBinsChanged(int);
    void nofTBinsChanged(int);
    void nofSBinsChanged(int);
    void calibNameButtonClicked();
    void previewButtonClicked();
    void findCalibName(QString);
    void updateVisuals();
    bool notVeto(uint32_t, int);
    void writeTimeOnly(int32_t,int);
    void writeEnergyOnly(int32_t,int);
    void modifyPlotState(int);
    virtual void resetButtonClicked();
    void setTimerTimeout(int msecs);
    void resetSingleHistogram(unsigned int,unsigned int);
    void changeBlockZoom(unsigned int, double, double);
};

#endif // MultipleCACHEHISTOGRAMPLUGIN_H
