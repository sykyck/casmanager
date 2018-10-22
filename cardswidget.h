#ifndef CARDSWIDGET_H
#define CARDSWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableView>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QTimer>
#include <QProcess>
#include "databasemgr.h"
#include "dvdcopytask.h"
#include "settings.h"
#include "carddetailwidget.h"
#include "casserver.h"
#include "sharedcardservices.h"

class CardsWidget : public QWidget
{
  Q_OBJECT
public:
  explicit CardsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent = 0);
  ~CardsWidget();
  bool isBusy();

protected:
  bool eventFilter(QObject *obj, QEvent *event);
  void showEvent(QShowEvent *);
  void hideEvent(QHideEvent *);

signals:
  
public slots:
  
private slots:
  void addNewCard(ArcadeCard card = ArcadeCard());
  void deleteCard();
  void editCard(const QModelIndex &index);
  void addCardFinished(ArcadeCard &card, bool ok);
  void getCardsFinished(QList<ArcadeCard> &cardList, bool ok);
  void getCardFinished(ArcadeCard &card, bool ok);
  void updateCardFinished(ArcadeCard &card, bool ok);
  void deleteCardFinished(QString cardNum, bool ok);
  void insertCardRow(ArcadeCard &card);
  void refreshCardListing();
  void userAuthorizationReceived(bool success,QString response);
  void timeout();
  void onServiceError(QString);
  void updatePageNumWidgets(bool isChecked);
  void getPageInfo(int pageNumber,int totalNumber);
  void onButtonLeftArrow();
  void onButtonRightArrow();

private:
  QStandardItemModel *cardsModel;
  QTableView *cardsTable;
  QLabel *lblCards;
  QPushButton *btnNewCard;
  QPushButton *btnDeleteCard;
  QPushButton *btnLeftArrow;
  QPushButton *btnRightArrow;
  QLabel *pageLabel;
  QLabel *OfLabel;
  QLabel *pageText;
  QLabel *pageTotalNumber;
  QHBoxLayout *buttonLayout;
  QHBoxLayout *videoLibraryLayout;
  QVBoxLayout *verticalLayout;
  DatabaseMgr *dbMgr;
  Settings *settings;
  CasServer *casServer;
  QTimer *updateCardsTimer;
  QTimer *authTimer;
  bool firstLoad;
  bool busy;  
  bool cardAccess;
  int pageNumber;
  int totalNumber;
  ArcadeCard currentCard;
  SharedCardServices *sharedCardService;
};

#endif // CARDSWIDGET_H
