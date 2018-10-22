#ifndef CARDDETAILWIDGET_H
#define CARDDETAILWIDGET_H

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
#include <QKeyEvent>
#include "settings.h"
#include "arcadecard.h"

class CardDetailWidget : public QDialog
{
  Q_OBJECT
public:
  explicit CardDetailWidget(Settings *settings, QWidget *parent = 0);
  ArcadeCard getCard();

  enum EditMode
  {
    ADD,
    UPDATE
  };

protected:
  bool eventFilter(QObject *obj, QEvent *event);
  void showEvent(QShowEvent *);
  void accept();
  void reject();

signals:
  
public slots:
  void populateForm(ArcadeCard &card);
  void setEditMode(EditMode mode);

private slots:
  void generateCardNumClicked();
  void clearForm();
  
private:
  QVariantList getCardTypes();
  QString generateCardNum();
  QString isValidInput();
  bool dataChanged();

  QLabel *lblCardNum;
  QLabel *lblValue;
  QLabel *lblCardType;

  QLineEdit *txtCardNum;
  QDoubleSpinBox *spnValue;
  QComboBox *cmbCardType;

  QPushButton *btnSave;
  QPushButton *btnCancel;
  QPushButton *btnDelete;
  QToolButton *btnGenerateCardNum;

  QGridLayout *gridLayout;
  QHBoxLayout *buttonLayout;
  QHBoxLayout *frontCoverButtonLayout;
  QHBoxLayout *backCoverButtonLayout;

  Settings *settings;
  ArcadeCard card;
  EditMode editMode;

  //bool dataChanged;
};

#endif // CARDDETAILWIDGET_H
