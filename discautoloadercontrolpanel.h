#ifndef DISCAUTOLOADERCONTROLPANEL_H
#define DISCAUTOLOADERCONTROLPANEL_H

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QGridLayout>
#include "discautoloader.h"
#include "settings.h"

class DiscAutoloaderControlPanel : public QDialog
{
  Q_OBJECT
public:
  explicit DiscAutoloaderControlPanel(Settings *settings, QWidget *parent = 0);
  ~DiscAutoloaderControlPanel();
  
signals:
  
public slots:
  
private slots:
  void loadDiscClicked();
  void unloadDiscClicked();
  void rejectDiscCicked();
  void loadDiscFinished(bool success, QString message = QString());
  void unloadDiscFinished(bool success, QString message = QString());

private:
  QLabel *lblInstructions;
  QPushButton *btnLoadDisc;
  QPushButton *btnUnloadDisc;
  QPushButton *btnRejectDisc;
  QPushButton *btnClose;
  QHBoxLayout *buttonLayout;
  QHBoxLayout *buttonLayout2;
  QFormLayout *formLayout;
  DiscAutoLoader *discAutoloader;
};

#endif // DISCAUTOLOADERCONTROLPANEL_H
