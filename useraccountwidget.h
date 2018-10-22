#ifndef USERACCOUNTWIDGET_H
#define USERACCOUNTWIDGET_H

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
#include "useraccount.h"


class UserAccountWidget : public QDialog
{
  Q_OBJECT
public:
  explicit UserAccountWidget(Settings *settings, QWidget *parent = 0);
  UserAccount getUserAccount();

  enum EditMode
  {
    ADD,
    UPDATE
  };

protected:
  void showEvent(QShowEvent *);
  void accept();
  void reject();

signals:

public slots:
  void populateForm(UserAccount &userAccount);
  void setEditMode(EditMode mode);

private slots:
  void clearForm();

private:
  bool dataChanged();
  QString isValidInput();

  QLabel *lblUsername;
  QLabel *lblPassword;

  QLineEdit *txtUsername;
  QLineEdit *txtPassword;

  QPushButton *btnSave;
  QPushButton *btnCancel;

  QGridLayout *gridLayout;
  QHBoxLayout *buttonLayout;

  Settings *settings;
  UserAccount userAccount;
  EditMode editMode;
};

#endif // USERACCOUNTWIDGET_H
