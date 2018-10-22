#ifndef STORESHIFTSWIDGET_H
#define STORESHIFTSWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QStringList>
#include <QTimeEdit>
#include <QCheckBox>
#include "settings.h"
#include "storeshift.h"


class StoreShiftsWidget : public QDialog
{
  Q_OBJECT
public:
  explicit StoreShiftsWidget(Settings *settings, QWidget *parent = 0);
  QList<StoreShift> getShifts();

protected:
  void showEvent(QShowEvent *);
  void accept();
  void reject();

signals:
  
public slots:
  void populateForm(QList<StoreShift> &shifts);

private slots:
  void onShiftCheckboxToggled(bool checked);

  
private:
  QString isValidInput();
  bool dataChanged();

  QLabel *lblStartTime;
  QLabel *lblEndTime;

  QList<QCheckBox *>chkShifts;
  QList<QTimeEdit *>shiftStartTimes;
  QList<QTimeEdit *>shiftEndTimes;

  QPushButton *btnSave;
  QPushButton *btnCancel;

  QGridLayout *gridLayout;
  QHBoxLayout *buttonLayout;

  Settings *settings;
  QList<StoreShift> shifts;
};

#endif // STORESHIFTSWIDGET_H
