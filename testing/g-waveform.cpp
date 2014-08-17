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

/* README
 * This module takes an external ASCII formatted file as input. The file should have
 * four columns with units in Amps and Siemens:
 *        absolute_current AMPA GABA (NMDA)
 * There should be one value for each time step and the total length of the stimulus
 * is determined by using the real-time period specified in the System->Control Panel.
 * If you change the real-time period, the length of the trial is recomputed. This
 * module automatically pauses itself when the protocol is complete.
 *
 * If you are using the Data Recorder, be sure to open the Data Recorder AFTER you open
 * this module or RTXI will crash. This module increments the trial number in the Data
 * Recorder so that each trial will be a separate structure in the HDF5 file. If you do
 * not open the Data Recorder, the module will still run as designed. This module will
 * automatically start and stop the Data Recorder. You must make sure to specify a data
 * filename and select the data you want to save.
 *
 * Use the checkboxes to select a combination of dynamic clamp stimuli and/or TTL pulses.
 * The dynamic clamp stimuli can be further filtered by using the checkboxes to make only
 * certain conductances (or current) active. The dynamic clamp output and the TTL pulses
 * are on two separate channels and must be assigned to the correct DAQ channels using
 * the System->Connector.
 *
 * There are both internal and external holding current parameters. The internal one is
 * specified using the "Holding Current (pA)" field in this module's GUI and is active
 * between repeated trials. When the external holding current is activated using the
 * checkbox, you must provide the instance ID of the correct holding current module in
 * the "Ihold ID" field. You will probably want to manually start the external Ihold
 * module first. When this dynamic clamp module unpauses, it will pause the
 * Ihold module, and vice versa.
 */

#include <g-waveform.h>
#include <basicplot.h>
#include <main_window.h>
#include <QtGui>

extern "C" Plugin::Object *
createRTXIPlugin(void)
{
  return new Gwaveform();
}

static DefaultGUIModel::variable_t
    vars[] =
      {
            { "Vm (mV)", "Membrane Potential", DefaultGUIModel::INPUT, },
            { "Command", "Total Current", DefaultGUIModel::OUTPUT, }, // output(0)
            { "AMPA Current", "AMPA Current", DefaultGUIModel::OUTPUT, }, // output(1)
            { "GABA Current", "GABA Current", DefaultGUIModel::OUTPUT, }, // output(2)
            { "NMDA Current", "NMDA Current", DefaultGUIModel::OUTPUT, }, // output(3)
            { "Laser TTL", "Laser TTL", DefaultGUIModel::OUTPUT, }, // output(4)
            {
                "Length (s)",
                "Length of trial is computed from the real-time period and the file size",
                DefaultGUIModel::STATE, },
            { "Comment", "Comment", DefaultGUIModel::COMMENT },
            {
                "Stimulus File Name",
                "ASCII file containing conductance waveform with values in siemens",
                DefaultGUIModel::COMMENT },
            {
                "Data File Name",
                "ASCII file containing conductance waveform with values in siemens",
                DefaultGUIModel::COMMENT },
            { "Ihold ID", "Instance ID of holding current module",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::INTEGER, },
            { "AMPA Rev (mV)", "Reversal Potential (mV) for AMPA",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "AMPA Gain", "Gain to multiply AMPA conductance values by",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "GABA Rev (mV)", "Reversal Potential (mV) for GABA",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "GABA Gain", "Gain to multiply GABA conductance values by",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "NMDA Rev (mV)", "Reversal Potential (mV) for NMDA",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "NMDA P1", "NMDA P1", DefaultGUIModel::PARAMETER
                | DefaultGUIModel::DOUBLE, },
            { "NMDA P2", "NMDA P2", DefaultGUIModel::PARAMETER
                | DefaultGUIModel::DOUBLE, },
            { "NMDA Gain", "Gain to multiply NMDA conductance values by",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "Wait time (s)", "Time to wait between trials",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "Holding Current (pA)",
                "Current to inject while waiting between trials",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "Laser TTL Duration (s)", "Duration of pulse",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "Laser TTL Pulses (#)", "Number of pulses",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "Laser TTL Freq (Hz)", "Frequency measured between pulse onsets",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "Laser TTL Delay (s)", "Time within trial to start pulse train",
                DefaultGUIModel::PARAMETER | DefaultGUIModel::DOUBLE, },
            { "Repeat", "Number of trials", DefaultGUIModel::PARAMETER
                | DefaultGUIModel::DOUBLE, },
            { "Time (s)", "Time (s)", DefaultGUIModel::STATE, }, };

static size_t num_vars = sizeof(vars) / sizeof(DefaultGUIModel::variable_t);

Gwaveform::Gwaveform(void) :
  DefaultGUIModel("Gwaveform", ::vars, ::num_vars)
{
  createGUI(vars, num_vars);
  initParameters();
  update( INIT);
  refresh();
  printf("\nStarting Dynamic Clamp:\n");

}

void
Gwaveform::createGUI(DefaultGUIModel::variable_t *var, int size)
{
  setWhatsThis(
      "<p><b>Waveform:</b><br>This module takes an external ASCII formatted file as input. The file should have"
 " four columns with units in Amps and Siemens: absolute_current AMPA GABA (NMDA)<br><br>"
 " There should be one value for each time step and the total length of the stimulus"
 " is determined by using the real-time period specified in the System->Control Panel."
 " If you change the real-time period, the length of the trial is recomputed. This"
 " module automatically pauses itself when the protocol is complete.<br><br>"
 " If you are using the Data Recorder, be sure to open the Data Recorder AFTER you open"
 " this module or RTXI will crash. This module increments the trial number in the Data"
 " Recorder so that each trial will be a separate structure in the HDF5 file. If you do"
 " not open the Data Recorder, the module will still run as designed. This module will"
 " automatically start and stop the Data Recorder. You must make sure to specify a data"
 " filename and select the data you want to save.<br><br>"
 " Use the checkboxes to select a combination of dynamic clamp stimuli and/or TTL pulses."
 " The dynamic clamp stimuli can be further filtered by using the checkboxes to make only"
 " certain conductances (or current) active. The dynamic clamp output and the TTL pulses"
 " are on two separate channels and must be assigned to the correct DAQ channels using"
 " the System->Connector.<br><br>"
 " There are both internal and external holding current parameters. The internal one is"
 " specified using the 'Holding Current (pA)' field in this module's GUI and is active"
 " between repeated trials. When the external holding current is activated using the"
 " checkbox, you must provide the instance ID of the correct holding current module in"
 " the 'Ihold ID' field. You will probably want to manually start the external Ihold"
 " module first. When this dynamic clamp module unpauses, it will pause the"
 " Ihold module, and vice versa."
 "</p>");

  setMinimumSize(200, 300);

  QBoxLayout *layout = new QHBoxLayout(this); // overall GUI layout

  // create custom GUI components
  QBoxLayout *leftlayout = new QVBoxLayout();

  QHButtonGroup *fileBox = new QHButtonGroup("File:", this);
  QPushButton *loadBttn = new QPushButton("Load File", fileBox);
  QPushButton *previewBttn = new QPushButton("Preview File", fileBox);
  QObject::connect(loadBttn, SIGNAL(clicked()), this, SLOT(loadFile()));
  QObject::connect(previewBttn, SIGNAL(clicked()), this, SLOT(previewFile()));

  QHButtonGroup *optionRow1 = new QHButtonGroup("Active Conductances", this);
  QToolTip::add(optionRow1,
      "Select which conductances will be active in the protocol");
  QCheckBox *currentCheckBox = new QCheckBox("Current", optionRow1);
  QCheckBox *ampaCheckBox = new QCheckBox("AMPA", optionRow1);
  QCheckBox *gabaCheckBox = new QCheckBox("GABA", optionRow1);
  QCheckBox *nmdaCheckBox = new QCheckBox("NMDA", optionRow1);
  currentCheckBox->setChecked(false); // set some defaults
  ampaCheckBox->setChecked(true);
  gabaCheckBox->setChecked(true);
  nmdaCheckBox->setChecked(false);
  QObject::connect(currentCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleCurrent(bool)));
  QObject::connect(ampaCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleAMPA(bool)));
  QObject::connect(gabaCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleGABA(bool)));
  QObject::connect(nmdaCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleNMDA(bool)));

  QHButtonGroup *optionRow2 = new QHButtonGroup("Active Stimuli", this);
  QToolTip::add(optionRow2,
      "Select which stimuli will be active in the protocol");
  QCheckBox *clampCheckBox = new QCheckBox("Dynamic Clamp", optionRow2);
  QCheckBox *laserCheckBox = new QCheckBox("Laser TTL", optionRow2);
  IholdCheckBox = new QCheckBox("Ext. Holding Current", optionRow2);
  clampCheckBox->setChecked(true); // set some defaults
  laserCheckBox->setChecked(false);
  IholdCheckBox->setChecked(false);
  IholdCheckBox->setEnabled(false);
  QObject::connect(clampCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleClamp(bool)));
  QObject::connect(laserCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleLaserTTL(bool)));
  QObject::connect(IholdCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleIhold(bool)));

  QHButtonGroup *optionRow3 = new QHButtonGroup("Data Recorder", this);
  QToolTip::add(optionRow3, "Select whether to sync with the data recorder");
  QCheckBox *recordCheckBox = new QCheckBox("Sync Data", optionRow3);
  recordCheckBox->setChecked(true); // set some defaults
  recordCheckBox->setEnabled(true);
  QObject::connect(recordCheckBox, SIGNAL(toggled(bool)), this, SLOT(toggleRecord(bool)));

  QHBox *utilityBox = new QHBox(this);
  pauseButton = new QPushButton("Pause", utilityBox);
  pauseButton->setToggleButton(true);
  QObject::connect(pauseButton, SIGNAL(toggled(bool)), this,
      SLOT(pause(bool)));
  QPushButton *modifyButton = new QPushButton("Modify", utilityBox);
  QObject::connect(modifyButton, SIGNAL(clicked(void)), this,
      SLOT(modify(void)));
  QPushButton *unloadButton = new QPushButton("Unload", utilityBox);
  QObject::connect(unloadButton, SIGNAL(clicked(void)), this,
      SLOT(exit(void)));
  QObject::connect(pauseButton, SIGNAL(toggled(bool)), modifyButton,
      SLOT(setEnabled(bool)));
  QToolTip::add(pauseButton, "Start/Stop dynamic clamp protocol");
  QToolTip::add(modifyButton, "Commit changes to parameter values");
  QToolTip::add(unloadButton, "Close plugin");

  // add custom GUI components to layout above default_gui_model components
  leftlayout->addWidget(fileBox);

  // create default_gui_model GUI DO NOT EDIT
  QScrollView *sv = new QScrollView(this);
  sv->setResizePolicy(QScrollView::AutoOneFit);
  leftlayout->addWidget(sv);

  QWidget *viewport = new QWidget(sv->viewport());
  sv->addChild(viewport);
  QGridLayout *scrollLayout = new QGridLayout(viewport, 1, 2);

  size_t nstate = 0, nparam = 0, nevent = 0, ncomment = 0;
  for (size_t i = 0; i < num_vars; i++)
    {
      if (vars[i].flags & (PARAMETER | STATE | EVENT | COMMENT))
        {
          param_t param;

          param.label = new QLabel(vars[i].name, viewport);
          scrollLayout->addWidget(param.label, parameter.size(), 0);
          param.edit = new DefaultGUILineEdit(viewport);
          scrollLayout->addWidget(param.edit, parameter.size(), 1);

          QToolTip::add(param.label, vars[i].description);
          QToolTip::add(param.edit, vars[i].description);

          if (vars[i].flags & PARAMETER)
            {
              if (vars[i].flags & DOUBLE)
                {
                  param.edit->setValidator(new QDoubleValidator(param.edit));
                  param.type = PARAMETER | DOUBLE;
                }
              else if (vars[i].flags & UINTEGER)
                {
                  QIntValidator *validator = new QIntValidator(param.edit);
                  param.edit->setValidator(validator);
                  validator->setBottom(0);
                  param.type = PARAMETER | UINTEGER;
                }
              else if (vars[i].flags & INTEGER)
                {
                  param.edit->setValidator(new QIntValidator(param.edit));
                  param.type = PARAMETER | INTEGER;
                }
              else
                param.type = PARAMETER;
              param.index = nparam++;
              param.str_value = new QString;
            }
          else if (vars[i].flags & STATE)
            {
              param.edit->setReadOnly(true);
              param.edit->setPaletteForegroundColor(Qt::darkGray);
              param.type = STATE;
              param.index = nstate++;
            }
          else if (vars[i].flags & EVENT)
            {
              param.edit->setReadOnly(true);
              param.type = EVENT;
              param.index = nevent++;
            }
          else if (vars[i].flags & COMMENT)
            {
              param.type = COMMENT;
              param.index = ncomment++;
            }

          parameter[vars[i].name] = param;
        }
    }

  // end default_gui_model GUI DO NOT EDIT

  // add custom components to layout below default_gui_model components
  leftlayout->addWidget(optionRow1);
  leftlayout->addWidget(optionRow2);
  leftlayout->addWidget(optionRow3);
  leftlayout->addWidget(utilityBox);
  layout->addLayout(leftlayout);

  //leftlayout->setResizeMode(QLayout::Fixed);
  layout->setResizeMode(QLayout::FreeResize);

  show();
}

Gwaveform::~Gwaveform(void)
{
}

void
Gwaveform::execute(void)
{
  Vm = input(0); // input is in V
  systime = count * dt; // module running time, s

  if (trial < maxtrials) // run trial
    {
      if (trialtimecount * dt < delay)
        {
          output(0) = Ihold;
        }
      else
        {
          if (clampon == true) // determine injected current
            {
              if (ampaon == true)
                {
                  output(1) = -1 * AMPAwave[idx] * (Vm - AMPArev) * AMPAgain;
                }
              else
                {
                  output(1) = 0;
                }
              if (gabaon == true)
                {
                  output(2) = -1 * GABAwave[idx] * (Vm - GABArev) * GABAgain;
                }
              else
                {
                  output(2) = 0;
                }
              if (nmdaon == true)
                {
                  output(3) = -1 * NMDAwave[idx] * (Vm - NMDArev) * 1 / (1 + P1
                      * exp(-P2 * Vm)) * NMDAgain;
                }
              else
                {
                  output(3) = 0;
                }
              output(0) = output(1) + output(2) + output(3);
              if (currenton == true)
                {
                  output(0) = output(0) + currentwave[idx];
                }
            }
          else if (clampon == false)
            {
              output(1) = 0;
              output(2) = 0;
              output(3) = 0;
              output(0) = 0;
            }

          if (laserTTLon == true) // determine TTL stimulus
            {
              output(4) = laserStim[idx];
            }
          else if (laserTTLon == false)
            {
              output(4) = 0;
            }

          if (laserTTLon == true or clampon == true)
            idx++;
        } // end single trial

    }
  else // all trials are done, send signal to holding current module, and pause
    {
      if (recordon)
        DataRecorder::stopRecording();
      output(5) = 1;
      pause(true);
    } // end protocol

  count++; // increment count to measure total module running time
  trialtimecount++; // increment count to measure time within single trial

  if (systime > triallength * (trial + 1))
    {
      if (recordon)
        DataRecorder::stopRecording();
      trial++;
      trialtimecount = 0;
      idx = 0;
      if (recordon)
        DataRecorder::startRecording();
    }
}

void
Gwaveform::update(Gwaveform::update_flags_t flag)
{
  switch (flag)
    {
  case INIT:
    setState("Length (s)", stimlength); // initialized in s, display in s
    setComment("Comment", userComment);
    setComment("Stimulus File Name", gFile);
    setComment("Data File Name", dFile);
    setParameter("Ihold ID", QString::number(IholdID));
    setParameter("GABA Rev (mV)", QString::number(GABArev * 1000)); // convert from V to mV
    setParameter("GABA Gain", QString::number(GABAgain));
    setParameter("AMPA Rev (mV)", QString::number(AMPArev * 1000)); // convert from V to mV
    setParameter("AMPA Gain", QString::number(AMPAgain));
    setParameter("NMDA Rev (mV)", QString::number(NMDArev * 1000)); // convert from V to mV
    setParameter("NMDA Gain", QString::number(NMDAgain));
    setParameter("NMDA P1", QString::number(P1));
    setParameter("NMDA P2", QString::number(P2));
    setParameter("Wait time (s)", QString::number(delay));
    setParameter("Holding Current (pA)", QString::number(Ihold * 1e12)); // convert from A to nA
    setParameter("Repeat", QString::number(maxtrials)); // initially 1
    setParameter("Laser TTL Duration (s)", QString::number(laserDuration)); // initially 1
    setParameter("Laser TTL Pulses (#)", QString::number(laserNumPulses)); // initially 1
    setParameter("Laser TTL Freq (Hz)", QString::number(laserFreq)); // initially 1
    setParameter("Laser TTL Delay (s)", QString::number(laserDelay)); // initially 1
    setState("Time (s)", systime);
    DataRecorder::openFile(dFile);

    break;
  case MODIFY:
    gFile = getComment("Stimulus File Name");
    dFile = getComment("Data File Name");
    userComment = getComment("Comment");
    printf("Saving to new file: %s\n", dFile.latin1());
    DataRecorder::openFile(dFile);
    IholdID = getParameter("Ihold ID").toInt();
    if (IholdID > 0 && IholdID != getID())
      {
        IholdModule
            = dynamic_cast<DefaultGUIModel*> (Settings::Manager::getInstance()->getObject(
                IholdID));
        IholdCheckBox->setEnabled(true);
      }
    else
      {
        IholdCheckBox->setEnabled(false);
      }
    GABArev = getParameter("GABA Rev (mV)").toDouble() / 1000; // convert from mV to V
    GABAgain = getParameter("GABA Gain").toDouble();
    AMPArev = getParameter("AMPA Rev (mV)").toDouble() / 1000; // convert from mV to V
    AMPAgain = getParameter("AMPA Gain").toDouble();
    NMDArev = getParameter("NMDA Rev (mV)").toDouble() / 1000; // convert from mV to V
    NMDAgain = getParameter("NMDA Gain").toDouble();
    P1 = getParameter("NMDA P1").toDouble();
    P2 = getParameter("NMDA P2").toDouble();
    delay = getParameter("Wait time (s)").toDouble();
    Ihold = getParameter("Holding Current (pA)").toDouble() * 1e-12; // convert from pA to A
    maxtrials = getParameter("Repeat").toDouble();
    bookkeep();
    if (getParameter("Laser TTL Duration (s)").toDouble() >= (getParameter(
        "Laser TTL Freq (Hz)").toDouble()))
      {
        QMessageBox::critical(
            this,
            "Dynamic Clamp",
            tr(
                "The laser TTL pulses will overlap with the specified pulse duration and frequency.\n"));
      }
    else if (1 / getParameter("Laser TTL Freq (Hz)").toDouble()
        * (getParameter("Laser TTL Pulses (#)").toDouble() - 1) + getParameter(
        "Laser TTL Duration (s)").toDouble() + getParameter(
        "Laser TTL Delay (s)").toDouble() > stimlength)
      {
        QMessageBox::critical(this, "Dynamic Clamp", tr(
            "The laser TTL pulse train is too long for the trial length.\n"));
      }
    else
      {
        laserDuration = getParameter("Laser TTL Duration (s)").toDouble();
        laserFreq = getParameter("Laser TTL Freq (Hz)").toDouble();
        laserNumPulses = getParameter("Laser TTL Pulses (#)").toDouble();
        laserDelay = getParameter("Laser TTL Delay (s)").toDouble();
        makeLaserTTL();
      }
    //loadFile(gFile);

    break;
  case PAUSE:
    output(0) = 0; // stop command in case pause occurs in the middle of command
    printf("Protocol paused.\n");
    if (Iholdon)
      {
        IholdModule->setActive(true);
        IholdModule->refresh();
      }
    break;
  case UNPAUSE:
    bookkeep();
    output(5) = 1;
    if (recordon)
      DataRecorder::startRecording();
    printf("Starting protocol.\n");
    if (Iholdon)
      {
        IholdModule->setActive(false);
        IholdModule->refresh();
      }
    break;
  case PERIOD:
    dt = RT::System::getInstance()->getPeriod() * 1e-9;
    printf("New real-time period: %f\n", dt);
    stimlength = GABAwave.size() * dt;
    loadFile(gFile);
  default:
    break;
    }
}

// custom functions

void
Gwaveform::initParameters()
{
  stimlength = 0; // seconds
  maxtrials = 1;
  Ihold = 0; // Amps
  delay = 1; // seconds
  GABArev = -.070; // V
  GABAgain = 1;
  AMPArev = 0; // V
  AMPAgain = 1;
  NMDArev = 0; // V
  NMDAgain = 1;
  P1 = .002;
  P2 = .109;
  dt = RT::System::getInstance()->getPeriod() * 1e-9; // s
  gFile = "No file loaded.";
  dFile = "default.h5";
  userComment = "None.";
  laserDuration = .25; // s
  laserNumPulses = 1;
  laserFreq = 1; // Hz
  laserDelay = .5; // s
  currenton = false;
  ampaon = true;
  gabaon = true;
  nmdaon = false;
  clampon = true;
  laserTTLon = false;
  IholdID = 0;
  Iholdon = false;
  recordon = true;
  ready = false;
  pauseButton->setEnabled(ready);
  bookkeep();
  makeLaserTTL();
}

void
Gwaveform::bookkeep()
{
  trial = 0;
  count = 0;
  trialtimecount = 0;
  systime = 0;
  idx = 0;
  triallength = stimlength + delay;
}

void
Gwaveform::toggleCurrent(bool on)
{
  currenton = on;
}

void
Gwaveform::toggleAMPA(bool on)
{
  ampaon = on;
}

void
Gwaveform::toggleGABA(bool on)
{
  gabaon = on;
}

void
Gwaveform::toggleNMDA(bool on)
{
  nmdaon = on;
}

void
Gwaveform::toggleClamp(bool on)
{
  clampon = on;
}

void
Gwaveform::toggleLaserTTL(bool on)
{
  laserTTLon = on;
}

void
Gwaveform::toggleIhold(bool on)
{
  Iholdon = on;
}

void
Gwaveform::toggleRecord(bool on)
{
  recordon = on;
}

void
Gwaveform::makeLaserTTL()
{
  laserStim.clear();
  for (int i = 0; i < laserDelay / dt; i++) // initial delay in trial before starting laser
    {
      laserStim.push_back(0);
    }
  for (int n = 0; n < laserNumPulses; n++)
    {
      for (int i = 0; i < laserDuration / dt; i++)
        {
          laserStim.push_back(5);
        }
      // fill in zeros for frequency
      for (int i = 0; i < ((1 / laserFreq) - laserDuration) / dt; i++)
        {
          laserStim.push_back(0);
        }
    }
  double remainder = stimlength - (laserDelay + 1 / laserFreq * (laserNumPulses
      - 1) + laserDuration);
  for (int i = 0; i < remainder / dt; i++) // pad the rest of the stimlength
    {
      laserStim.push_back(0);
    }
  for (int i = 0; i < delay / dt; i++) // overall delay in trial
    {
      laserStim.push_back(0);
    }
}

void
Gwaveform::loadFile()
{
  QFileDialog* fd = new QFileDialog(this, "Conductance waveform file", TRUE);
  fd->setMode(QFileDialog::AnyFile);
  fd->setViewMode(QFileDialog::Detail);
  QString fileName;
  if (fd->exec() == QDialog::Accepted)
    {
      fileName = fd->selectedFile();
      printf("Loading new file: %s\n", fileName.latin1());
      gFile = fileName;
      setComment("Stimulus File Name", fileName);
      currentwave.clear();
      GABAwave.clear();
      AMPAwave.clear();
      NMDAwave.clear();
      QFile file(fileName);

      if (file.open(IO_ReadOnly))
        {
          QTextStream stream(&file);
          double value;
          while (!stream.atEnd())
            {
              stream >> value;
              currentwave.push_back(value);
              stream >> value;
              AMPAwave.push_back(value);
              stream >> value;
              GABAwave.push_back(value);
              stream >> value;
              NMDAwave.push_back(value);
            }
        }
      stimlength = GABAwave.size() * dt;
      setState("Length (s)", stimlength); // initialized in s, display in s
      // pad waveform to account for wait between trials
      for (int i = 0; i < delay / dt; i++)
        {
          currentwave.push_back(0);
          GABAwave.push_back(0);
          AMPAwave.push_back(0);
          NMDAwave.push_back(0);
        }
      ready = true;
    }
  else
    {
      setComment("Stimulus File Name", "No file loaded.");
      ready = false;
    }
  pauseButton->setEnabled(ready);
}

void
Gwaveform::loadFile(QString fileName)
{
  if (fileName == "No file loaded.")
    {
      printf("No file name specified. \n");
      setComment("Stimulus File Name", fileName);
      ready = false;
      return;
    }
  else
    {
      printf("Loading new file: %s\n", fileName.latin1());
      currentwave.clear();
      GABAwave.clear();
      AMPAwave.clear();
      NMDAwave.clear();
      QFile file(fileName);
      if (file.open(IO_ReadOnly))
        {
          QTextStream stream(&file);
          double value;
          while (!stream.atEnd())
            {
              stream >> value;
              currentwave.push_back(value);
              stream >> value;
              AMPAwave.push_back(value);
              stream >> value;
              GABAwave.push_back(value);
              stream >> value;
              NMDAwave.push_back(value);
            }
        }
      stimlength = GABAwave.size() * dt;
      setState("Length (s)", stimlength); // initialized in s, display in s
      // pad waveform with to account for wait between trials
      for (int i = 0; i < delay / dt; i++)
        {
          currentwave.push_back(0);
          GABAwave.push_back(0);
          AMPAwave.push_back(0);
          NMDAwave.push_back(0);
        }
      ready = true;
      setComment("Stimulus File Name", fileName);
    }
  pauseButton->setEnabled(ready);
}

void
Gwaveform::previewFile()
{
  double* time = new double[static_cast<int> (GABAwave.size())];
  double* currentData = new double[static_cast<int> (currentwave.size())];
  double* gabaData = new double[static_cast<int> (GABAwave.size())];
  double* ampaData = new double[static_cast<int> (AMPAwave.size())];
  double* nmdaData = new double[static_cast<int> (NMDAwave.size())];

  for (int i = 0; i < GABAwave.size(); i++)
    {
      time[i] = dt * i;
      currentData[i] = currentwave[i];
      gabaData[i] = GABAwave[i];
      ampaData[i] = AMPAwave[i];
      nmdaData[i] = NMDAwave[i];
    }
  PlotDialog *current = new PlotDialog(this, "Current", time, currentData,
      currentwave.size());
  current->show();
  PlotDialog *gaba = new PlotDialog(this, "GABA Conductance", time, gabaData,
      GABAwave.size());
  gaba->show();
  PlotDialog *ampa = new PlotDialog(this, "AMPA Conductance", time, ampaData,
      AMPAwave.size());
  ampa->show();
  PlotDialog *nmda = new PlotDialog(this, "NMDA Conductance", time, nmdaData,
      NMDAwave.size());
  nmda->show();

}

bool
Gwaveform::OpenFile(QString FName)
{
  dataFile.setName(FName);
  if (dataFile.exists())
    {
      switch (QMessageBox::warning(this, "Dynamic Clamp", tr(
          "This file already exists: %1.\n").arg(FName), "Overwrite", "Append",
          "Cancel", 0, 2))
        {
      case 0: // overwrite
        dataFile.remove();
        if (!dataFile.open(IO_Raw | IO_WriteOnly))
          {
            return false;
          }
        break;
      case 1: // append
        if (!dataFile.open(IO_Raw | IO_WriteOnly | IO_Append))
          {
            return false;
          }
        break;
      case 2: // cancel
        return false;
        break;
        }
    }
  else
    {
      if (!dataFile.open(IO_Raw | IO_WriteOnly))
        return false;
    }
  stream.setDevice(&dataFile);
  stream.setPrintableData(false); // write binary
  printf("File opened: %s\n", FName.latin1());
  return true;
}
