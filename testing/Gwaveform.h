/*
 Copyright (C) 2011 Georgia Institute of Technology

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */

#include <default_gui_model.h>
#include <data_recorder.h>
#include <string>
#include <scatterplot.h>
#include <plotdialog.h>
#include <basicplot.h>
#include <RTXIprintfilter.h>
#include <QtGui>

class Gwaveform : public DefaultGUIModel
{

  Q_OBJECT

public:

  Gwaveform(void);
  virtual
  ~Gwaveform(void);
  void
  execute(void);
  void
  createGUI(DefaultGUIModel::variable_t *, int);

protected:

  void
  update(DefaultGUIModel::update_flags_t);

private:

  double systime;
  double Vm;
  double dt;
  double stimlength;
  double delay;
  double Ihold;
  double maxtrials;
  double GABArev;
  double GABAgain;
  double AMPArev;
  double AMPAgain;
  double NMDArev;
  double NMDAgain;
  double P1;
  double P2;
  QString gFile;
  QString dFile;
  QString userComment;
  double laserDuration;
  double laserNumPulses;
  double laserFreq;
  double laserDelay;

  // bookkeeping
  bool ready; // used to check if file is loaded
  bool currenton;
  bool ampaon;
  bool gabaon;
  bool nmdaon;
  bool clampon;
  bool laserTTLon;
  bool Iholdon;
  bool recordon;
  double triallength;
  std::vector<double> currentwave; // absolute current
  std::vector<double> GABAwave; // conductance waveforms
  std::vector<double> AMPAwave;
  std::vector<double> NMDAwave;
  std::vector<double> laserStim;
  double spktime;
  int trial;
  long long count;
  long long trialtimecount;
  int idx;
  int IholdID;
  DefaultGUIModel * IholdModule;
  QCheckBox *IholdCheckBox;

  void
  initParameters();
  void
  bookkeep();
  // Functions and parameters for saving data to file without using data recorder
  bool
  OpenFile(QString);
  QFile dataFile;
  QDataStream stream;

private slots:
  void
  loadFile();
  void
  loadFile(QString);
  void
  previewFile();
  void
  makeLaserTTL();
  void
  toggleCurrent(bool);
  void
  toggleAMPA(bool);
  void
  toggleGABA(bool);
  void
  toggleNMDA(bool);
  void
  toggleClamp(bool);
  void
  toggleLaserTTL(bool);
  void
  toggleIhold(bool);
  void
  toggleRecord(bool);

};
