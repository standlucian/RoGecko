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

#ifndef CAEN792MODULE_H
#define CAEN792MODULE_H

#include "basemodule.h"
#include "baseplugin.h"
#include "caenadcdmx.h"
#include "pluginmanager.h"

#define CAEN_V792_NOF_CHANNELS 32
#define CAEN_V792_MAX_NOF_WORDS 34 // per event

struct Caen792ModuleConfig {
    uint32_t base_addr;

    uint8_t irq_level;
    uint8_t irq_vector;
    uint8_t ev_trg;

    uint8_t thresholds[CAEN_V792_NOF_CHANNELS];
    bool killChannel[CAEN_V792_NOF_CHANNELS];

    uint8_t cratenumber;
    uint16_t fastclear;
    uint8_t i_ped;
    uint8_t slideconst;

    uint8_t cblt_addr;
    int cblt_ctrl;

    // tdc registers
    uint8_t fsr;
    bool stop_mode;

    // control register 1
    bool block_end;
    bool berr_enable;
    bool program_reset;
    bool align64;

    // bit set 2
    bool memTestModeEnabled;
    bool offline;
    bool overRangeSuppressionEnabled;
    bool zeroSuppressionEnabled;
    bool slidingScaleEnabled;
    bool zeroSuppressionThr;
    bool autoIncrementEnabled;
    bool emptyEventWriteEnabled;
    bool slideSubEnabled;
    bool alwaysIncrementEventCounter;

    unsigned int pollcount;

    Caen792ModuleConfig ()
    : irq_level (0), irq_vector (0), ev_trg (0)
    , cratenumber (0), fastclear (0), i_ped (180), slideconst (0)
    , cblt_addr (0xAA), cblt_ctrl (0)
    , fsr (0x18), stop_mode (false)
    , block_end (false), berr_enable (true), program_reset (false), align64 (false)
    , memTestModeEnabled (false), offline (false), overRangeSuppressionEnabled (true)
    , zeroSuppressionEnabled (true), slidingScaleEnabled (false), zeroSuppressionThr (false)
    , autoIncrementEnabled (true), emptyEventWriteEnabled (false), slideSubEnabled (false)
    , alwaysIncrementEventCounter (false)
    , pollcount (10000)
    {
        for (int i = 0; i < CAEN_V792_NOF_CHANNELS; ++i) {
            killChannel [i] = false;
            thresholds [i] = 0;
        }
    }
};

class Caen792Module : public BaseModule {
	Q_OBJECT
public:
    // Factory method
    static AbstractModule *createQdc (int id, const QString &name) {
        return new Caen792Module (id, name, true);
    }
    static AbstractModule *createTdc (int id, const QString &name) {
        return new Caen792Module (id, name, false);
    }

    virtual void saveSettings (QSettings*);
    virtual void applySettings (QSettings*);
    virtual int getSettings() {return 0;};


    int counterReset ();
    int dataReset ();
    int softReset ();

    virtual void setChannels ();
    virtual int acquire (Event* ev);
    virtual bool dataReady ();
    virtual int reset ();
    virtual void counterResetSync(){};
    virtual int panicReset ();
    virtual int configure ();

    virtual uint32_t getBaseAddress () const;
    virtual void setBaseAddress (uint32_t baddr);

    uint16_t getInfo () const;
    int readStatus ();

    uint16_t getStatus1 () const { return status1; }
    uint16_t getStatus2 () const { return status2; }
    uint16_t getBitset1 () const { return bitset1; }
    uint16_t getBitset2 () const { return bitset2; }
    uint32_t getEventCount () const { return evcnt; }

    Caen792ModuleConfig *getConfig () { return &conf_; }

    int acquireSingle (uint32_t *data, uint32_t *rd);

    void runStartingEvent() { dmx_.runStartingEvent(); }

private:
    Caen792Module (int _id, const QString &, bool _isqdc);
    void writeToBuffer(Event *ev);

    void REG_DUMP();

public slots:
    void singleShot (uint32_t *data, uint32_t *rd);
    virtual void prepareForNextAcquisition () {}

private:
    Caen792ModuleConfig conf_;
    bool isqdc;

    mutable uint16_t info_;
    uint16_t bitset1;
    uint16_t bitset2;
    uint16_t status1;
    uint16_t status2;
    uint32_t evcnt;
    uint32_t data [CAEN_V792_MAX_NOF_WORDS];
    uint32_t rd;

    CaenADCDemux dmx_;
    QVector<EventSlot*> evslots_;
};

#endif // CAEN792MODULE_H
