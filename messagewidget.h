#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QTimer>

class MessageWidget : public QDialog
{
  Q_OBJECT
public:
  explicit MessageWidget(QString message, QString title, QString button1Label, QString button2Label = QString(), QString button3Label = QString(), QString button4Label = QString(), QString checkboxLabel = QString(), int timeout = 0, QWidget *parent = 0);
  ~MessageWidget();
  void setChecked(bool checked);
  bool isChecked();
  
signals:
  void button1Clicked();
  void button2Clicked();
  void button3Clicked();
  void button4Clicked();
  
public slots:
  
private slots:
  void updateCountdown();

private:
  QVBoxLayout *verticalLayout;
  QDialogButtonBox *buttonBox;
  QLabel *lblMessage;
  QPushButton *btn1;
  QPushButton *btn2;
  QPushButton *btn3;
  QPushButton *btn4;
  QCheckBox *chk1;
  QTimer *timer;
};

#endif // MESSAGEWIDGET_H
