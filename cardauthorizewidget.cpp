#include "cardauthorizewidget.h"
#include "qslog/QsLog.h"
#include <QMessageBox>

CardAuthorizeWidget::CardAuthorizeWidget(CasServer *casServer, QWidget *parent) : QDialog(parent)
{
    this->casServer = casServer;
    this->setFixedSize(320, 100);

    lblPassword = new QLabel("Password");

    txtPassword = new QLineEdit;
    txtPassword->setMaxLength(30);
    txtPassword->setEchoMode(QLineEdit::Password);

    btnLogin = new QPushButton(tr("Login"));
    btnCancel = new QPushButton(tr("Cancel"));
    connect(btnLogin, SIGNAL(clicked()), this, SLOT(accept()));
    connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));

    gridLayout = new QGridLayout;
    gridLayout->addWidget(lblPassword, 1, 0);
    gridLayout->addWidget(txtPassword, 1, 1, 1, 3);

    buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(btnLogin);
    buttonLayout->addWidget(btnCancel);
    gridLayout->addLayout(buttonLayout, 4, 0, 1, 4, Qt::AlignCenter);

    this->setLayout(gridLayout);
    this->setWindowTitle(tr("User Login"));
}

void CardAuthorizeWidget::accept()
{
    if(!txtPassword->text().isEmpty())
    {
        casServer->userAuthorization(txtPassword->text());
        QDialog::accept();
    }
    else
    {
        QMessageBox::warning(this, tr("Invalid"), tr("Enter password"));
    }

}

void CardAuthorizeWidget::reject()
{
    QLOG_DEBUG() << QString("User canceled authorization");
    QDialog::reject();
}

void CardAuthorizeWidget::showEvent(QShowEvent *)
{
    txtPassword->setFocus();
}
