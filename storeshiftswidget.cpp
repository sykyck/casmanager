#include "storeshiftswidget.h"
#include "qslog/QsLog.h"
#include <QMessageBox>
#include <math.h>

const int MAX_SHIFTS = 4;

StoreShiftsWidget::StoreShiftsWidget(Settings *settings, QWidget *parent) : QDialog(parent)
{
  // Disable maximize,minimize and resizing of window
  this->setFixedSize(300, 250);
  this->settings = settings;
  this->setWindowTitle(tr("Store Shifts Editor"));

  lblStartTime = new QLabel("Start Time");
  lblEndTime = new QLabel("End Time");

  btnSave = new QPushButton(tr("Save"));
  btnCancel = new QPushButton(tr("Cancel"));
  connect(btnSave, SIGNAL(clicked()), this, SLOT(accept()));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));

  gridLayout = new QGridLayout;
  gridLayout->addWidget(lblStartTime, 0, 1);
  gridLayout->addWidget(lblEndTime, 0, 2);

  for (int i = 1; i <= MAX_SHIFTS; ++i)
  {
    chkShifts.append(new QCheckBox(QString("Shift #%1").arg(i)));
    chkShifts.last()->setProperty("shiftNum", i);

    shiftStartTimes.append(new QTimeEdit);
    shiftStartTimes.last()->setDisplayFormat("hh:mm ap");

    shiftEndTimes.append(new QTimeEdit);
    shiftEndTimes.last()->setDisplayFormat("hh:mm ap");

    gridLayout->addWidget(chkShifts.last(), i, 0);
    gridLayout->addWidget(shiftStartTimes.last(), i, 1);
    gridLayout->addWidget( shiftEndTimes.last(), i, 2);

    connect(chkShifts.last(), SIGNAL(toggled(bool)), this, SLOT(onShiftCheckboxToggled(bool)));
  }

  buttonLayout = new QHBoxLayout;
  buttonLayout->addWidget(btnSave);
  buttonLayout->addWidget(btnCancel);

  gridLayout->addLayout(buttonLayout, 6, 0, 1, 3, Qt::AlignCenter);

  this->setLayout(gridLayout);
}

QList<StoreShift> StoreShiftsWidget::getShifts()
{
  return shifts;
}

void StoreShiftsWidget::showEvent(QShowEvent *)
{

}

void StoreShiftsWidget::accept()
{
  QString errors = isValidInput();

  //dataChanged = false;
  if (errors.isEmpty())
  {
    if (chkShifts.at(0)->checkState() != Qt::Checked)
    {
      QLOG_DEBUG() << "User submitted form with no shift checked";
      if (QMessageBox::question(this, tr("Store Shifts Editor"), tr("Without any shifts defined, the clerk station cannot show transaction details. Are you sure this is what you want?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
      {
        QLOG_DEBUG() << "User chose to continue clearing the shifts";
      }
      else
      {
        QLOG_DEBUG() << "User chose not to clear the shifts";
        return;
      }
    }

    shifts.clear();
    for (int i = 0; i < chkShifts.length(); ++i)
    {
      if (chkShifts.at(i)->isChecked())
      {
        StoreShift shift;
        shift.setShiftNum(i + 1);
        shift.setStartTime(shiftStartTimes.at(i)->time());
        shift.setEndTime(shiftEndTimes.at(i)->time());

        shifts.append(shift);
      }
      else
        break;
    }

    QDialog::accept();
  }
  else
  {
    QLOG_DEBUG() << QString("User tried to submit invalid store shifts. Error: %1").arg(errors);
    QMessageBox::warning(this, tr("Invalid"), tr("Correct the following problems:\n%1").arg(errors));
  }
}

void StoreShiftsWidget::reject()
{
  bool continueReject = true;

  if (dataChanged())
  {
    if (QMessageBox::question(this, tr("Changes made"), tr("Changes were made to the shifts, are you sure you want to lose these?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
      continueReject = false;
  }

  if (continueReject)
  {
    QLOG_DEBUG() << QString("User canceled editing store shifts");
    QDialog::reject();
  }
}

void StoreShiftsWidget::populateForm(QList<StoreShift> &shifts)
{
  this->shifts = shifts;

  foreach (QCheckBox *chk, chkShifts)
  {
    chk->setChecked(false);
    chk->setEnabled(false);
  }

  foreach (QTimeEdit *t, shiftStartTimes)
  {
    t->setTime(QTime(0, 0));
    t->setEnabled(false);
  }

  foreach (QTimeEdit *t, shiftEndTimes)
  {
    t->setTime(QTime(0, 0));
    t->setEnabled(false);
  }

  for (int i = 0; i < shifts.length(); ++i)
  {
    int shiftIndex = shifts.at(i).getShiftNum() - 1;
    QLOG_DEBUG() << QString("shift %1 index: %2").arg(shifts.at(i).getShiftNum()).arg(shiftIndex);

    chkShifts.at(shiftIndex)->setChecked(true);

    shiftStartTimes.at(shiftIndex)->setTime(shifts.at(i).getStartTime());
    shiftStartTimes.at(shiftIndex)->setEnabled(true);

    shiftEndTimes.at(shiftIndex)->setTime(shifts.at(i).getEndTime());
    shiftEndTimes.at(shiftIndex)->setEnabled(true);

    if (i + 1 == shifts.length())
    {
      chkShifts.at(shiftIndex)->setEnabled(true);
    }
  }

  if (shifts.length() < shifts.length())
  {
    QLOG_DEBUG() << QString("enabling checkbox for shift #%1").arg(shifts.length());
    chkShifts.at(shifts.length() - 1)->setEnabled(true);
  }

  if (shifts.length() == 0)
  {
    QLOG_DEBUG() << "No store shifts defined yet";
    chkShifts.first()->setEnabled(true);
  }
}

void StoreShiftsWidget::onShiftCheckboxToggled(bool checked)
{
  // Checkboxes are only allowed to be toggled sequentially. The form starts out
  // with none checked and all but the first disabled. If the first one is checked
  // then the next checkbox is enabled. If the next one is checked then the previous one is
  // disabled. If it is unchecked then the first one is enabled again
  QCheckBox *chk = qobject_cast<QCheckBox*>(sender());

  if (chk)
  {
    int shiftNum = chk->property("shiftNum").toInt();

    if (checked)
    {
      // Enable time fields for shift # that was checked
      shiftStartTimes.at(shiftNum - 1)->setEnabled(true);
      shiftEndTimes.at(shiftNum - 1)->setEnabled(true);

      // Enable the next shift's checkbox if it exists
      if (shiftNum < chkShifts.length())
      {
        chkShifts.at(shiftNum)->setEnabled(true);
      }

      // If there is a shift before this one then disable its checkbox
      if (shiftNum - 2 >= 0)
      {
        chkShifts.at(shiftNum - 2)->setEnabled(false);
      }
    }
    else
    {
      // Disable time fields for shift # that was unchecked
      shiftStartTimes.at(shiftNum - 1)->setEnabled(false);
      shiftEndTimes.at(shiftNum - 1)->setEnabled(false);

      // Disable the next shift's checkbox if it exists
      if (shiftNum < chkShifts.length())
      {
        chkShifts.at(shiftNum)->setEnabled(false);
      }

      // If there is a shift before this one then enable its checkbox
      if (shiftNum - 2 >= 0)
      {
        chkShifts.at(shiftNum - 2)->setEnabled(true);
      }
    }
  }
}

QString StoreShiftsWidget::isValidInput()
{
  //QLOG_DEBUG() << QString("Validating card num: %1").arg(txtCardNum->text());

  QStringList errorList;

  // Each shift's end time must be the beginning of the next shift's start time
  // The only exception to this is the last shift's end time does not have to in cases where
  // the store is not open 24 hours. Each shift start/end time should come before the next
  // and a start time must be less than the end time

  // Since shifts could go into the next day, looking at only the time component is not enough when
  // So assign the current date to each shift time and adjust the date if it looks like a shift is starting/ending
  // on the following day. Do not allow 1 shift that is 24 hours
  // if none checked ask user if he wants to clear all shifts

  QDateTime previousEndTime;
  QDate currentDate = QDate::currentDate();

  for (int i = 0; i < chkShifts.length() && errorList.length() == 0; ++i)
  {
    if (chkShifts.at(i)->checkState() == Qt::Checked)
    {
      QDateTime startTime(currentDate, shiftStartTimes.at(i)->time());
      QDateTime endTime(currentDate, shiftEndTimes.at(i)->time());
      bool lastShift = false;

      // Determine if this is the last shift based on either the current index
      if (i + 1 == chkShifts.length())
      {
        lastShift = true;
      }
      // Or if the next shift is not checked
      else if (chkShifts.at(i + 1)->checkState() != Qt::Checked)
      {
        lastShift = true;
      }

      if (startTime == endTime)
      {
        errorList.append(QString("Shift #%1: the start time cannot equal the end time").arg(i + 1));
        break;
      }
      else if (previousEndTime.isValid())
      {
        if (previousEndTime != startTime)
        {
          errorList.append(QString("Shift #%1: the end time of the previous shift must be the start time of this shift. There cannot be any gap or overlap.").arg(i + 1));
          break;
        }
      }

      // If startTime is > endTime then assume the endTime is into the following day
      if (startTime > endTime)
      {
        if (currentDate == QDate::currentDate())
        {
          // The rest of the shifts will be compared using the next day
          currentDate = currentDate.addDays(1);
        }
        else
        {
          // We've already incremented the currentDate previously so something ain't right
          errorList.append(QString("Shift #%1: the end time cannot be less than the start time.").arg(i + 1));
          break;
        }
      }

      if (lastShift)
      {
        // if the end time of last shift is in the next day, make sure it doesn't overlap 1st shift
        if (currentDate != QDate::currentDate())
        {
          // This is the last defined shift so make sure the end time does not
          // overlap the beginning time of the 1st shift
          QDateTime firstShiftStartTime(currentDate, shiftStartTimes.first()->time());

          // Update the endTime's date of the last shift's end time in case we incremented currentDate
          endTime.setDate(currentDate);

          if (endTime > firstShiftStartTime)
          {
            errorList.append(QString("Shift #%1: the end time cannot overlap the start time of the first shift.").arg(i + 1));
          }
        }
      }

      previousEndTime = endTime;
    }
    else
    {
      break;
    }
  }

  return errorList.join("\n");
}

bool StoreShiftsWidget::dataChanged()
{
  // TODO: Implement
  return false;
}
