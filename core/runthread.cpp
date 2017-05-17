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

#include "runthread.h"

#include "abstractmodule.h"
#include "modulemanager.h"
#include "interfacemanager.h"
#include "runmanager.h"
#include "abstractinterface.h"
#include "eventbuffer.h"

#include <QCoreApplication>
#include <sched.h>
#include <cstdio>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>

#define GECKO_PROFILE_RUN

#ifdef GECKO_PROFILE_RUN
#include <time.h>
static struct timespec starttime;
static uint64_t timeinacq;
static uint64_t timeinpoll;
static uint64_t timeForModule[20];
#endif

RunThread::RunThread () {

    triggered = false;
    running = false;
    abort = false;

    interruptBased = false;
    pollBased = false;

    setObjectName("RunThread");

    moveToThread(this);

    nofPolls = 0;
    nofSuccessfulEvents = 0;
    acquisitionOngoing=0;

    std::cout << "Run thread initialized." << std::endl;
}

RunThread::~RunThread()
{
    bool finished = wait(5000);
    if(!finished) terminate();

#ifdef GECKO_PROFILE_RUN
    struct timespec et;
    clock_gettime(CLOCK_MONOTONIC, &et);
    uint64_t rt = (et.tv_sec - starttime.tv_sec) * 1000000000 + (et.tv_nsec - starttime.tv_nsec);
    std::cout << "Runtime: " << (rt * 1e-9) <<" s, Acq%: " << (100.* timeinacq / rt) << ", Unsuccessful%: "
              << (100. * (nofPolls - nofSuccessfulEvents) / nofPolls)
              << " Polls per event: " << (1.*nofPolls/nofSuccessfulEvents)
              << std::endl;
    for(int i = 0; i < 10; ++i) {
        std::cout << "Time for module " << i << ": " << (100. * timeForModule[i] / rt) << "%" << std::endl;
    }
#endif

    std::cout << "Run thread stopped." << std::endl;
}


void RunThread::run()
{
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(3, &cpuset);

    pthread_setaffinity_np(pthread_self(),sizeof(cpuset),&cpuset);

    // Scheduling magic
    int cpu = sched_getcpu();
    printf("cpu: %d\n",cpu);

    __pid_t pid = getpid();
    printf("pid: %d\n",(uint32_t)pid);

    int scheduler = sched_getscheduler(pid);
    printf("scheduler: %d\n",scheduler);

    Qt::HANDLE threadId = this->thread()->currentThreadId();
    printf("threadId: 0x%08x\n",(uint)threadId);

    QThread* thisThread = this->thread()->currentThread();
    printf("thisThread: 0x%8p\n",(unsigned int*)thisThread);

    pid_t tid;
    tid = syscall(SYS_gettid);
    printf("tid: %d\n",(uint32_t)tid);

    int threadScheduler = sched_getscheduler(tid);
    printf("thread scheduler: %d\n",threadScheduler);

    rlimit rl_nice;
    getrlimit(RLIMIT_NICE,&rl_nice);
    printf("rlimit nice: %d, %d\n",(int)rl_nice.rlim_cur,(int)rl_nice.rlim_max);
    rlimit rl_prio;
    getrlimit(RLIMIT_RTPRIO,&rl_prio);
    printf("rlimit rtprio: %d, %d\n",(int)rl_prio.rlim_cur,(int)rl_prio.rlim_max);

    struct sched_param old_param;
    sched_getparam(tid,&old_param);
    int old_priority = old_param.__sched_priority;
    printf("old prio: %d\n",old_priority);

    /*int new_priority = rl_nice.rlim_max;
    int new_scheduler = SCHED_FIFO;
    struct sched_param new_param;
    new_param.__sched_priority = new_priority;
    int stat = sched_setscheduler(tid,new_scheduler,&new_param);
    if (stat == -1) perror("sched_setscheduler()");*/

    modules = *ModuleManager::ref ().list ();
    triggers = ModuleManager::ref ().getTriggers ().toList ();
    mandatories = ModuleManager::ref ().getMandatorySlots ().toList ();
    createConnections();

    // Hold external trigger logic
    InterfaceManager::ptr ()->getMainInterface()->setOutput1(true);

    // Reset modules
    foreach (AbstractModule *m, modules) {
        m->reset ();
        if (m->configure ())
            std::cout << "Run Thread: " << m->getName ().toStdString () <<": Configure failed!" << std::endl;
    }

    std::cout<<InterfaceManager::ptr()->getMainInterface()->writeA32D16(0xBB006090,3)<<std::endl;

    std::cout << "Run thread started." << std::endl;

    // Wait for reset to be done
    sleep(2);

#ifdef GECKO_PROFILE_RUN
    clock_gettime (CLOCK_MONOTONIC, &starttime);
    timeinacq = 0;
    timeinpoll = 0;
    for (int i = 0; i < 10; ++i) {
        timeForModule[i] = 0;
    }
#endif

    // Allow external trigger logic
    InterfaceManager::ptr ()->getMainInterface()->setOutput1(false);

    if(interruptBased)
    {
        exec();
    }
    else
    {
        pollLoop();
    }

    exit(0);
}

void RunThread::createConnections()
{
    QList<AbstractModule*>::iterator ch(triggers.begin());

    while(ch != triggers.end())
    {
        connect(*ch, SIGNAL(triggered(AbstractModule*)),this,SLOT(acquire()));
        ch++;
    }
}

bool RunThread::acquire()
{
    acquisitionOngoing=1;
    //std::cout << currentThreadId() << ": Run thread acquiring." << std::endl;
    InterfaceManager *imgr = InterfaceManager::ptr ();
    Event *ev = RunManager::ref ().getEventBuffer ()->createEvent ();

    int modulesz = modules.size ();

    imgr->getMainInterface()->setOutput1(true); // VETO signal for DAQ readout

    for (int i = 0; i < modulesz; ++i)
    {
        AbstractModule* curM = modules [i];

        imgr->getMainInterface()->setOutput2(true);
        if (/*curM == _trg ||*/ curM->dataReady ()) {
            //imgr->getMainInterface()->setOutput2(false);

#ifdef GECKO_PROFILE_RUN
            struct timespec st, et;
            clock_gettime (CLOCK_MONOTONIC, &st);
#endif
            imgr->getMainInterface()->setOutput2(true); // VETO signal for DAQ readout
            curM->acquire(ev);
            imgr->getMainInterface()->setOutput2(false); // VETO signal for DAQ readout
#ifdef GECKO_PROFILE_RUN
            clock_gettime (CLOCK_MONOTONIC, &et);
            timeForModule[i] += (et.tv_sec - st.tv_sec) * 1000000000 + (et.tv_nsec - st.tv_nsec);
#endif
//            if(curM->dataReady()) {
//                std::cout << "RunThread:acquire: ERROR: module " << curM->getName().toStdString()
//                          << " is still DRDY after acquisition" << std::endl;
//            }
        }
    }

    imgr->getMainInterface()->setOutput1(false); // Remove VETO signal for DAQ readout

    acquisitionOngoing=0;

    if (QSet<const EventSlot*>::fromList (mandatories).subtract(ev->getOccupiedSlots ()).empty()) {
        RunManager::ref ().getEventBuffer ()->queue (ev);
        emit acquisitionDone();
        return true;
    } else {
        RunManager::ref ().getEventBuffer ()->releaseEvent (ev);
        return false;
    }
}

void RunThread::stop()
{
    mutex.lock();
    abort = true;
    mutex.unlock();

    this->exit(0);
    std::cout << "Run thread stopping." << std::endl;
}

void RunThread::pollLoop()
{

    lastAcqPoll=0;
    lastResetPoll=0;
    while(!abort)
    {
        InterfaceManager *imgr = InterfaceManager::ptr ();
        nofPolls++;
            if(imgr->getMainInterface()->readIRQStatus())
            {
                if(!acquisitionOngoing)
                {
                lastAcqPoll=nofPolls;
#ifdef GECKO_PROFILE_RUN
                struct timespec st, et;
                clock_gettime (CLOCK_MONOTONIC, &st);
#endif
                if (acquire())
                    nofSuccessfulEvents++;
#ifdef GECKO_PROFILE_RUN
                clock_gettime (CLOCK_MONOTONIC, &et);
                timeinacq += (et.tv_sec - st.tv_sec) * 1000000000 + (et.tv_nsec - st.tv_nsec);
#endif
            }
            }

        if(nofPolls-lastAcqPoll>30000000)
            {
                lastAcqPoll=nofPolls;
                for(int i=0;i<modules.size();i++)
                    modules[i]->panicReset();
                std::cout<<"Acquisition blocked. Auto-reset"<<std::endl;
            }
    }
}

void RunThread::forceRead()
{
    if(!acquisitionOngoing)
    {
#ifdef GECKO_PROFILE_RUN
                struct timespec st, et;
                clock_gettime (CLOCK_MONOTONIC, &st);
#endif
                if (acquire())
                    nofSuccessfulEvents++;
#ifdef GECKO_PROFILE_RUN
                clock_gettime (CLOCK_MONOTONIC, &et);
                timeinacq += (et.tv_sec - st.tv_sec) * 1000000000 + (et.tv_nsec - st.tv_nsec);
#endif
    }
}
