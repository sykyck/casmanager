#include "useraccountwidget.h"
#include "qslog/QsLog.h"
#include <QMessageBox>

const int MIN_USERNAME_LENGTH = 3;
const int MIN_PASSWORD_LENGTH = 6;

UserAccountWidget::UserAccountWidget(Settings *settings, QWidget *parent) : QDialog(parent)
{
  // Disable maximize,minimize and resizing of window
  this->setFixedSize(320, 150);
  this->settings = settings;

  editMode = ADD;

  lblUsername = new QLabel("Username");
  lblPassword = new QLabel("Password");

  txtUsername = new QLineEdit;
  txtUsername->setMaxLength(30);

  txtPassword = new QLineEdit;
  txtPassword->setMaxLength(30);
  txtPassword->setEchoMode(QLineEdit::Password);

  btnSave = new QPushButton(tr("Save"));
  btnCancel = new QPushButton(tr("Cancel"));

  connect(btnSave, SIGNAL(clicked()), this, SLOT(accept()));
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));

  buttonLayout = new QHBoxLayout;

  gridLayout = new QGridLayout;
  gridLayout->addWidget(lblUsername, 0, 0);
  gridLayout->addWidget(txtUsername, 0, 1);

  gridLayout->addWidget(lblPassword, 1, 0);
  gridLayout->addWidget(txtPassword, 1, 1);

  buttonLayout->addWidget(btnSave);
  buttonLayout->addWidget(btnCancel);
  gridLayout->addLayout(buttonLayout, 3, 0, 1, 2, Qt::AlignCenter);

  this->setLayout(gridLayout);

  setEditMode(editMode);
}

UserAccount UserAccountWidget::getUserAccount()
{
  return userAccount;
}


void UserAccountWidget::showEvent(QShowEvent *)
{
  if (editMode == ADD)
  {
    txtUsername->setFocus();
    txtUsername->selectAll();
  }
  else
  {
    txtPassword->setFocus();
    txtPassword->selectAll();
  }
}

void UserAccountWidget::clearForm()
{
  txtUsername->clear();
  txtPassword->clear();
}

void UserAccountWidget::accept()
{
  QString errors = isValidInput();

  //dataChanged = false;
  if (errors.isEmpty())
  {
    // Convert username to lowercase
    userAccount.setUsername(txtUsername->text().toLower());
    userAccount.setPassword(txtPassword->text());

    QDialog::accept();
  }
  else
  {
    QLOG_DEBUG() << QString("User tried to submit invalid data for user account. Error: %1").arg(errors);
    QMessageBox::warning(this, tr("Invalid"), tr("Correct the following problems:\n%1").arg(errors));
  }
}

void UserAccountWidget::reject()
{
  bool continueReject = true;

  if (dataChanged() && editMode == UPDATE)
  {
    if (QMessageBox::question(this, tr("Changes made"), tr("Changes were made to the user account, are you sure you want to lose these?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
      continueReject = false;
  }

  if (continueReject)
  {
    QLOG_DEBUG() << QString("User canceled adding/updating user account");
    QDialog::reject();
  }
}

void UserAccountWidget::populateForm(UserAccount &userAccount)
{
  this->userAccount = userAccount;

  txtUsername->setText(userAccount.getUsername());
  txtPassword->setText(userAccount.getPassword());
}

void UserAccountWidget::setEditMode(UserAccountWidget::EditMode mode)
{
  editMode = mode;

  if (editMode == ADD)
  {
    this->setWindowTitle(tr("New User Account"));
    txtUsername->setReadOnly(false);
    txtUsername->setEnabled(true);
  }
  else if (editMode == UPDATE)
  {
    this->setWindowTitle(tr("Update User Account"));
    txtUsername->setReadOnly(true);
    txtUsername->setEnabled(false);
  }
}

QString UserAccountWidget::isValidInput()
{
  QLOG_DEBUG() << QString("Validating user account");

  QStringList errorList;

  QString username = txtUsername->text().trimmed();
  txtUsername->setText(username);

  // Username must be >= 3 characters
  // TODO: Move these to settings
  if (username.length() < MIN_USERNAME_LENGTH)
  {
    errorList.append(QString("- Username must be at least %1 characters.").arg(MIN_USERNAME_LENGTH));
    txtUsername->setFocus();
    txtUsername->selectAll();
  }

  QString password = txtPassword->text().trimmed();
  txtPassword->setText(password);

  // Password must be >= 6 characters
  // TODO: Move these to settings
  if (password.length() < MIN_PASSWORD_LENGTH)
  {
    if (errorList.length() == 0)
    {
      txtPassword->setFocus();
      txtPassword->selectAll();
    }

    errorList.append(QString("- Password must be at least %1 characters.").arg(MIN_PASSWORD_LENGTH));
  }
  else if (password.contains(username, Qt::CaseInsensitive) || username.contains(password, Qt::CaseInsensitive))
  {
    errorList.append(QString("- Password cannot be the same as the username or include the username."));
  }
  else if (password.count(password.at(0)) == password.length())
  {
    errorList.append(QString("- Password cannot be all the same character."));
  }

  return errorList.join("\n");
}

bool UserAccountWidget::dataChanged()
{
  return (userAccount.getUsername() != txtUsername->text() ||
          userAccount.getPassword() != txtPassword->text());
}
