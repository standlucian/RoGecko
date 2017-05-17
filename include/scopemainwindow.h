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

#ifndef SCOPEMAINWINDOW_H
#define SCOPEMAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QObject>
#include <QSettings>
#include <QCoreApplication>
#include <QApplication>

#include <QLabel>

#include <QGridLayout>

#include <QFileDialog>
#include <QDateTimeEdit>
#include <QDateTime>

#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>

#include <QStatusBar>

#include <QDockWidget>
#include <QStackedWidget>

#include <QTreeWidget>
#include <QTreeView>
#include <QStandardItemModel>

#include <QTimer>
#include <QTextStream>

#include <QUdpSocket>
#include <QTcpSocket>
#include <QTcpServer>

#include <typeinfo>

// Include external headers
#include "runthread.h"
#include "pluginthread.h"

#include "pluginmanager.h"
#include "baseplugin.h"

#include "runmanager.h"

#include "modulemanager.h"
#include "abstractinterface.h"
#include "geckoremote.h"

class QStandardItem;
class QTreeWidget;
class QCheckBox;
class QHostAddress;
class QTextEdit;
class QComboBox;

class SystemInfo;
class RemoteControlPanel;

Q_DECLARE_METATYPE(QWidget*)
Q_DECLARE_METATYPE(QHostAddress)
Q_DECLARE_METATYPE(QAbstractSocket::SocketError)

// Class definition

class ScopeMainWindow : public QMainWindow
{
        Q_OBJECT

protected:
    void createUI();
    void createTreeView();
    void createActions();
    void createMenu();
    void createConnections();
    void createStatusBar();

    void createRunSetupPage();
    void createRunControlPage();
    void createRemoteControlPage();
    void createUdpSocket();
    void createTcpSocket();
    void loadChannelList();

    QStandardItem *addTabToTree(QWidget* newTab);
    void addRunPageToTree(QWidget* newWidget);

    void closeEvent(QCloseEvent *event);
    void setRunName(QString);

    void writeRunStartFile();
    void writeRunStopFile();
    void writeDotFile (QTextStream &out);

    void saveConfig (QSettings *);
    void loadConfig (QSettings *);

    void setConfigEnabled (bool enabled);
    void setLocalAddress();

    QList<QHostAddress> findOutIpAddresses();
    void processDatagram(QByteArray, QHostAddress sender, quint16 serverPort);
    void processQuery(QStringList query, QHostAddress sender);
    void processPost(QStringList post, QHostAddress sender);
    void processRemoteState(QStringList state);

public:
    explicit ScopeMainWindow(QWidget *parent = 0);
    ~ScopeMainWindow();
    ModuleManager *mmgr;
    PluginManager *pmgr;
    RunManager    *rmgr;

    void saveSettingsToFile (const QString file);

signals:

public slots:
    void treeViewClicked(const QModelIndex & idx, const QModelIndex & prev);
    void triggerListChanged(QTreeWidgetItem*,int);
    void channelListChanged(QTreeWidgetItem*,int);
    void setStatusText(QString);
    void runNameButtonClicked();
    void applySettings();
    void saveSettings();
    bool saveSettingsQuery();

    void startAcquisition();
    void stopAcquisition();
    void runStarted ();
    void runStopping ();
    void runStopped ();
    void updateRunPage(float evspersec, unsigned evs, uint64_t triggers, uint64_t trigspersec);

    void addModuleToTree(AbstractModule* newModule);
    void addInterfaceToTree(AbstractInterface* newModule);
    void addPluginToTree(AbstractPlugin* newPlugin);
    void removeModuleFromTree(AbstractModule* newModule);
    void removeInterfaceFromTree(AbstractInterface* newModule);
    void removePluginFromTree(AbstractPlugin* newPlugin);
    void moduleNameChanged(AbstractModule *m, QString oldname);
    void interfaceNameChanged(AbstractInterface *m, QString oldname);
    void pluginNameChanged(AbstractPlugin *p, QString oldname);

    void open();
    bool save();
    bool saveAs();

    void exportDot ();

    void createModule ();
    void editModule ();
    void removeModule ();

    void createInterface ();
    void editInterface ();
    void removeInterface ();

    void createPlugin ();
    void editPlugin ();
    void removePlugin ();

    void createComponent ();
    void editComponent ();
    void removeComponent ();

    void makeMainInterface ();

    void jumpToPlugin (AbstractPlugin *);

    void oneSecondTimerTimeout();

private:
    const QString defaultIni;
    QString fileName;

    QMenuBar *menubar;
    QMenu *fileMenu;
    QMenu *modulesMenu;
    QMenu *pluginsMenu;
    QMenu *interfacesMenu;

    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *exportDotAct;

    QAction *createModAct;
    QAction *editModAct;
    QAction *removeModAct;

    QAction *createIfAct;
    QAction *editIfAct;
    QAction *removeIfAct;

    QAction *createPlugAct;
    QAction *editPlugAct;
    QAction *removePlugAct;

    QAction *createAct;
    QAction *editAct;
    QAction *removeAct;

    QAction *makeMainIfaceAct;

    QStatusBar *statusBar;
    QLabel *statusLabel;
    QLabel *statusCpuLabel;
    QLabel *statusNetLabel;

    QDockWidget *treeDock;
    QTreeWidget *treeWidget;
    QTreeView *treeView;
    QStandardItemModel *treeModel;

    // Run setup
    QTreeWidget* triggerList;
    QTreeWidget* channelList;

    // Run control
    QPushButton* runStartButton;
    QPushButton* runNameButton;
    QLineEdit* runNameEdit;
    QTextEdit* runInfoEdit;
    QDateTimeEdit* startTimeEdit;
    QDateTimeEdit* stopTimeEdit;
    QLineEdit* nofEventsEdit;
    QLineEdit* eventsPerSecondEdit;
    QLineEdit* nofTriggersEdit;
    QLineEdit* triggersPerSecondEdit;
    QCheckBox *singleEventModeBox;

    // Timers
    QTimer* oneSecondTimer;

    // Remote control
    RemoteControlPanel* remoteControl;

    // Layout
    QStackedWidget *mainArea;
    QStandardItem  *runItem;
    QStandardItem  *pluginItem;
    QStandardItem  *ifaceItem;
    QStandardItem  *moduleItem;

    QSettings* settings;

    bool configEditAllowed;    // Configuration can be changed
};

#endif // SCOPEMAINWINDOW_H
