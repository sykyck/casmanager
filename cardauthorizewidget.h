#ifndef CARDAUTHORIZEWIDGET_H
#define CARDAUTHORIZEWIDGET_H

#include <QDialog>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include "casserver.h"

class CardAuthorizeWidget : public QDialog
{
    Q_OBJECT
public:
    explicit CardAuthorizeWidget(CasServer *casServer,  QWidget *parent = 0);

protected:
    void showEvent(QShowEvent *);
    void accept();
    void reject();

private:
    QLabel *lblPassword;

    QLineEdit *txtPassword;

    QPushButton *btnLogin;
    QPushButton *btnCancel;

    QGridLayout *gridLayout;
    QHBoxLayout *buttonLayout;
    CasServer *casServer;

};

#endif // CARDAUTHORIZEWIDGET_H
