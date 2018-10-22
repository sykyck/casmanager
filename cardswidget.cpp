#include "cardswidget.h"
#include "cardauthorizewidget.h"
#include "qslog/QsLog.h"
#include "global.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QStandardItem>
#include <QSortFilterProxyModel>

CardsWidget::CardsWidget(DatabaseMgr *dbMgr, CasServer *casServer, Settings *settings, QWidget *parent) : QWidget(parent)
{
  this->dbMgr = dbMgr;
  this->settings = settings;
  this->casServer = casServer;
  firstLoad = true;
  busy = false;
  cardAccess = false;
  currentCard.clear();

  connect(dbMgr, SIGNAL(addCardFinished(ArcadeCard&,bool)), this, SLOT(addCardFinished(ArcadeCard&,bool)));
  connect(dbMgr, SIGNAL(getCardsFinished(QList<ArcadeCard>&,bool)), this, SLOT(getCardsFinished(QList<ArcadeCard>&,bool)));
  connect(dbMgr, SIGNAL(getCardFinished(ArcadeCard&,bool)), this, SLOT(getCardFinished(ArcadeCard&,bool)));
  connect(dbMgr, SIGNAL(updateCardFinished(ArcadeCard&,bool)), this, SLOT(updateCardFinished(ArcadeCard&,bool)));
  connect(dbMgr, SIGNAL(deleteCardFinished(QString,bool)), this, SLOT(deleteCardFinished(QString,bool)));

  updateCardsTimer = new QTimer;
  updateCardsTimer->setInterval(settings->getValue(UPDATE_CARDS_INTERVAL).toInt());
  connect(updateCardsTimer, SIGNAL(timeout()), this, SLOT(refreshCardListing()));

  authTimer = new QTimer;
  authTimer->setInterval(settings->getValue(AUTH_INTERVAL).toInt());
  connect(authTimer,SIGNAL(timeout()), this, SLOT(timeout()));

  verticalLayout = new QVBoxLayout;

  // Card Type, Card #, Balance, Last Active, Last Booth, In Use, Activated, ArcadeCardObject
  cardsModel = new QStandardItemModel(0, 8);
  cardsModel->setHorizontalHeaderItem(0, new QStandardItem(QString("Card Type")));
  cardsModel->setHorizontalHeaderItem(1, new QStandardItem(QString("Card #")));
  cardsModel->setHorizontalHeaderItem(2, new QStandardItem(QString("Balance")));
  cardsModel->setHorizontalHeaderItem(3, new QStandardItem(QString("Last Active")));
  cardsModel->setHorizontalHeaderItem(4, new QStandardItem(QString("Last Booth")));
  cardsModel->setHorizontalHeaderItem(5, new QStandardItem(QString("In Use")));
  cardsModel->setHorizontalHeaderItem(6, new QStandardItem(QString("Activated")));
  cardsModel->setHorizontalHeaderItem(7, new QStandardItem(QString("ArcadeCardObject")));

  cardsTable = new QTableView;

  lblCards = new QLabel(tr("Cards"));

  btnNewCard = new QPushButton(tr("New Card"));
  connect(btnNewCard, SIGNAL(clicked()), this, SLOT(addNewCard()));

  btnDeleteCard = new QPushButton(tr("Delete Card"));
  connect(btnDeleteCard, SIGNAL(clicked()), this, SLOT(deleteCard()));

  pageLabel = new QLabel("Page");
  pageLabel->setLineWidth(1);

  btnLeftArrow = new QPushButton(tr("<"));
  connect(btnLeftArrow, SIGNAL(clicked()), this, SLOT(onButtonLeftArrow()));

  pageText = new QLabel("");
  pageText->setFixedWidth(30);

  btnRightArrow = new QPushButton(tr(">"));
  connect(btnRightArrow, SIGNAL(clicked()), this, SLOT(onButtonRightArrow()));

  OfLabel = new QLabel("OF");
  OfLabel->setLineWidth(1);

  pageTotalNumber = new QLabel("");
  pageTotalNumber->setFixedWidth(30);

  if(settings->getValue(USE_SHARE_CARDS,DEFAULT_USE_SHARE_CARDS,false).toBool())
  {
      btnLeftArrow->setVisible(true);
      pageText->setVisible(true);
      btnRightArrow->setVisible(true);
      pageTotalNumber->setVisible(true);
      pageLabel->setVisible(true);
      OfLabel->setVisible(true);
  }
  else
  {
      btnLeftArrow->setVisible(false);
      pageText->setVisible(false);
      btnRightArrow->setVisible(false);
      pageTotalNumber->setVisible(false);
      pageLabel->setVisible(false);
      OfLabel->setVisible(false);
  }

  // FIXME: Sorting doesn't match
  QSortFilterProxyModel *sortFilter = new QSortFilterProxyModel;
  sortFilter->setSourceModel(cardsModel);
  sortFilter->setSortRole(Qt::UserRole + 1);

  cardsTable->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  cardsTable->horizontalHeader()->setStretchLastSection(true);
  cardsTable->horizontalHeader()->setStyleSheet("font:bold Arial;");
  cardsTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  cardsTable->setSelectionMode(QAbstractItemView::SingleSelection);
  cardsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  cardsTable->setWordWrap(true);
  cardsTable->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
  cardsTable->verticalHeader()->hide();
  cardsTable->setModel(sortFilter);
  cardsTable->setSortingEnabled(true);
  cardsTable->setColumnHidden(cardsModel->columnCount() - 1, true);
  cardsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  cardsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
  cardsTable->installEventFilter(this);
  cardsTable->sortByColumn(3, Qt::DescendingOrder);
  connect(cardsTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(editCard(QModelIndex)));

  buttonLayout = new QHBoxLayout;
  buttonLayout->addWidget(btnNewCard, 0, Qt::AlignLeft);
  buttonLayout->addWidget(btnDeleteCard, 0, Qt::AlignLeft);
  buttonLayout->addWidget(pageLabel, 0, Qt::AlignLeft);
  buttonLayout->addWidget(btnLeftArrow, 0, Qt::AlignLeft);
  buttonLayout->addWidget(pageText, 0, Qt::AlignLeft);
  buttonLayout->addWidget(btnRightArrow, 0, Qt::AlignLeft);
  buttonLayout->addWidget(OfLabel, 0, Qt::AlignLeft);
  buttonLayout->addWidget(pageTotalNumber, 0, Qt::AlignLeft);
  buttonLayout->addStretch(1);

  verticalLayout->addWidget(lblCards, 0, Qt::AlignLeft);
  verticalLayout->addWidget(cardsTable);
  verticalLayout->addLayout(buttonLayout);

  this->setLayout(verticalLayout);

  this->sharedCardService = new SharedCardServices(this);
  connect(this->sharedCardService, SIGNAL(addCardFinished(ArcadeCard&,bool)), this, SLOT(addCardFinished(ArcadeCard&,bool)));
  connect(this->sharedCardService, SIGNAL(deleteCardFinished(QString,bool)), this, SLOT(deleteCardFinished(QString,bool)));
  connect(this->sharedCardService, SIGNAL(updateCardFinished(ArcadeCard&,bool)), this, SLOT(updateCardFinished(ArcadeCard&,bool)));
  connect(this->sharedCardService, SIGNAL(getCardsFinished(QList<ArcadeCard>&,bool)), this, SLOT(getCardsFinished(QList<ArcadeCard>&,bool)));
  connect(this->sharedCardService, SIGNAL(getCardFinished(ArcadeCard&,bool)), this, SLOT(getCardFinished(ArcadeCard&,bool)));
  connect(this->sharedCardService, SIGNAL(serviceError(QString)), this, SLOT(onServiceError(QString)));
  connect(this->sharedCardService, SIGNAL(sendPageInfo(int,int)), this, SLOT(getPageInfo(int,int)));
}

CardsWidget::~CardsWidget()
{
  verticalLayout->deleteLater();
}

void CardsWidget::onButtonLeftArrow()
{
    if(this->pageNumber>0)
    {
        this->pageNumber = this->pageNumber - 1;
        sharedCardService->getCards(this->pageNumber);
    }
}

void CardsWidget::onButtonRightArrow()
{
    if(this->pageNumber<totalNumber-1)
    {
        this->pageNumber = this->pageNumber + 1;
        sharedCardService->getCards(this->pageNumber);
    }
}

void CardsWidget::updatePageNumWidgets(bool isChecked)
{
    if(isChecked)
    {
        btnLeftArrow->setVisible(true);
        pageText->setVisible(true);
        btnRightArrow->setVisible(true);
        pageTotalNumber->setVisible(true);
        pageLabel->setVisible(true);
        OfLabel->setVisible(true);
    }
    else
    {
        btnLeftArrow->setVisible(false);
        pageText->setVisible(false);
        btnRightArrow->setVisible(false);
        pageTotalNumber->setVisible(false);
        pageLabel->setVisible(false);
        OfLabel->setVisible(false);
    }
}
bool CardsWidget::isBusy()
{
  return false;
}

void CardsWidget::onServiceError(QString error)
{
    this->setDisabled(true);
    QMessageBox::warning(this, tr("Network Problem!"), tr("Please check your Internet Connectivity or contact tech support."));
    QLOG_DEBUG()<<"Inside onServiceError"<<error;
}

bool CardsWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

    if (keyEvent->key() == Qt::Key_Delete)
    {
      deleteCard();
      return true;
    }
    else
    {
      // standard event processing
      return QWidget::eventFilter(obj, event);
    }
  }
  else
  {
    // standard event processing
    return QWidget::eventFilter(obj, event);
  }
}

void CardsWidget::showEvent(QShowEvent *)
{
  QLOG_DEBUG() << QString("Showing Cards tab");

  updateCardsTimer->start();
  refreshCardListing();
}

void CardsWidget::hideEvent(QHideEvent *)
{
  // Do not refresh cards when this widget is hidden (another tab is selected)
  updateCardsTimer->stop();
}

void CardsWidget::addNewCard(ArcadeCard card)
{
  if (!this->cardAccess)
  {
    connect(this->casServer, SIGNAL(userAuthorizationReceived(bool,QString)), this, SLOT(userAuthorizationReceived(bool,QString)));

    CardAuthorizeWidget cardAuthWidget(casServer);

    if (cardAuthWidget.exec() == QDialog::Rejected)
    {
      QLOG_DEBUG() << "disconnect userAuthorizationReceived";

      // Disconnect the signal/slot since it's also used on the clerk stations tab
      disconnect(casServer, SIGNAL(userAuthorizationReceived(bool,QString)), this, SLOT(userAuthorizationReceived(bool,QString)));
    }
  }
  else{
    currentCard.clear();

    CardDetailWidget cardDetailWin(settings);

    // If card set then there was an error with adding the card and we're allowing the
    // user to modify it to try again
    if (!card.getCardNum().isEmpty())
    {
      cardDetailWin.populateForm(card);
    }

    if (cardDetailWin.exec())
    {
      // if card detail widget returns accepted code
      //   try adding new card to database
      currentCard = cardDetailWin.getCard();
      currentCard.setActivated(QDateTime::currentDateTime());

      QLOG_DEBUG() << QString("User submitted card #: %1").arg(currentCard.getCardNum());

      if(settings->getValue(USE_SHARE_CARDS,DEFAULT_USE_SHARE_CARDS,false).toBool())
      {
          sharedCardService->checkCardPresent(currentCard);
      }
      else
      {
          dbMgr->getCard(currentCard.getCardNum());
      }
    }
  }
}

void CardsWidget::deleteCard()
{
  updateCardsTimer->stop();

  QModelIndexList selectedRows = cardsTable->selectionModel()->selectedRows(7);

  if (selectedRows.count() == 0)
  {
    QLOG_DEBUG() << "User pressed Delete key without row selected in cards table";
    QMessageBox::warning(this, tr("Delete Card"), tr("Select one or more rows in the table to delete. You can select multiple rows by holding down the Shift key."));
  }
  else
  {
    QLOG_DEBUG() << QString("User selected %1 row(s) to delete from the cards datagrid").arg(selectedRows.count());

    bool allowDelete = true;

    if (QMessageBox::question(this, tr("Delete Card"), tr("If a card is deleted it will no longer be available in the arcade. Are you sure you want to delete the selected card(s)?\n\nTip: You can select and delete multiple rows by holding down the Shift key."), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
      setEnabled(false);

      // Make sure none of the cards the user wants to delete are currently in use
      for (int i = 0; i < selectedRows.count() && allowDelete; ++i)
      {
        QSortFilterProxyModel *filter = (QSortFilterProxyModel *)cardsTable->model();
        QStandardItem *objectField = cardsModel->item(filter->mapToSource(selectedRows.at(i)).row(), cardsModel->columnCount() - 1);
        ArcadeCard card = objectField->data().value<ArcadeCard>();

        QLOG_DEBUG() << QString("User selected card #: %1 to delete").arg(card.getCardNum());

        // If card is in use and the last active time was less than 10 minutes ago then do not allow it to be deleted
        // The reason it is even permitted to delete a card that's in use is in case there is some software bug that
        // never cleared the in use flag of the card
        // TODO: Move this time to settings
        if (card.getInUse() && QDateTime::currentDateTime().toTime_t() - card.getLastActive().toTime_t() < (10 * 60))
        {
          QLOG_DEBUG() << QString("Not deleting card #: %1 since it was recently active: %2").arg(card.getCardNum()).arg(card.getLastActive().toString("MM/dd/yyyy h:mm AP"));

          // TODO: Move this time to settings
          QMessageBox::warning(this, tr("Delete Card"), tr("Cannot delete card #: %1 because it appears to be in use and has been less than %2 minutes since it was last active.").arg(card.getCardNum()).arg(10));
        }
        else
        {
            if(settings->getValue(USE_SHARE_CARDS,DEFAULT_USE_SHARE_CARDS,false).toBool())
            {
                sharedCardService->deleteCard(card);
            }
            else
            {
                dbMgr->deleteCard(card);
            }
        }
      }

      setEnabled(true);
    }
    else
    {
      QLOG_DEBUG() << "User canceled deleting the selected cards";
    }
  }

  updateCardsTimer->start();
}

void CardsWidget::editCard(const QModelIndex &index)
{
  updateCardsTimer->stop();

  QSortFilterProxyModel *filter = (QSortFilterProxyModel *)cardsTable->model();
  QStandardItem *objectField = cardsModel->item(filter->mapToSource(index).row(), cardsModel->columnCount() - 1);
  ArcadeCard card = objectField->data().value<ArcadeCard>();

  CardDetailWidget cardDetailWin(settings);
  cardDetailWin.populateForm(card);
  cardDetailWin.setEditMode(CardDetailWidget::UPDATE);

  if (cardDetailWin.exec())
  {
    ArcadeCard card = cardDetailWin.getCard();
    if(settings->getValue(USE_SHARE_CARDS,DEFAULT_USE_SHARE_CARDS,false).toBool())
    {
        sharedCardService->updateCard(card);
    }
    else
    {
        dbMgr->updateCard(card);
    }
  }

  updateCardsTimer->start();
}

void CardsWidget::addCardFinished(ArcadeCard &card, bool ok)
{
  if (ok)
  {
    QLOG_DEBUG() << QString("Added card #: %1, value: %2, type: %3 ").arg(card.getCardNum()).arg(card.getCashValue()).arg(card.cardTypeToString());

    refreshCardListing();
  }
  else
  {
    QLOG_ERROR() << "Could not add new card to database";

    QMessageBox::warning(this, tr("New Card"), tr("Could not add card. Please try again or contact tech support."));
  }
}

void CardsWidget::getCardsFinished(QList<ArcadeCard> &cardList, bool ok)
{
  if (ok)
  {
    if (cardsModel->rowCount() > 0)
      cardsModel->removeRows(0, cardsModel->rowCount());

    foreach (ArcadeCard card, cardList)
      insertCardRow(card);

    cardsTable->sortByColumn(cardsTable->horizontalHeader()->sortIndicatorSection(), cardsTable->horizontalHeader()->sortIndicatorOrder());
  }
  else
  {
     QLOG_WARN() << "Could not get cards from  SharedCardServices api";

     QMessageBox::warning(this, tr("Invalid Response"), tr("Could not get cards from web service. Please try again or contact tech support."));
  }

  this->setEnabled(true);
}

void CardsWidget::getPageInfo(int pageNumber, int totalNumber)
{
    this->pageNumber = pageNumber;
    this->totalNumber = totalNumber;
    if(totalNumber>0)
    {
       pageText->setText(QString("%1").arg(pageNumber+1));
    }
    else
    {
       pageText->setText(QString("%1").arg(pageNumber));
    }
    pageTotalNumber->setText(QString("%1").arg(totalNumber));
}

void CardsWidget::getCardFinished(ArcadeCard &card, bool ok)
{
  if (ok)
  {
    QLOG_DEBUG() << QString("Card #: %1 Already exists!").arg(card.getCardNum());

    QMessageBox::warning(this, tr("New Card"), tr("Card #: %1 already exists and cannot be added. Cards must be unique.").arg(card.getCardNum()));
    addNewCard(card);
  }
  else
  {
    QLOG_DEBUG() << QString("Card not found!, safe to add");
    if(settings->getValue(USE_SHARE_CARDS, DEFAULT_USE_SHARE_CARDS, false).toBool())
    {
        sharedCardService->addCard(currentCard);
    }
    else
    {
        dbMgr->addCard(currentCard);
    }
  }
}

void CardsWidget::updateCardFinished(ArcadeCard &card, bool ok)
{
  if (ok)
  {
    QLOG_DEBUG() << QString("Updated card #: %1, value: %2, type: %3 ").arg(card.getCardNum()).arg(card.getCashValue()).arg(card.cardTypeToString());
    refreshCardListing();
  }
  else
  {
    QLOG_ERROR() << "Card not updated in database";

    QMessageBox::warning(this, tr("Update Card"), tr("Could not update card!. Please try again or contact tech support."));
  }
}

void CardsWidget::deleteCardFinished(QString cardNum, bool ok)
{
  if (ok)
  {
    QLOG_DEBUG() << QString("Card #: %1 deleted").arg(cardNum);
    refreshCardListing();
  }
  else
  {
    if (this->isVisible())
    {
      QLOG_ERROR() << "Card not delete card from database";

      QMessageBox::warning(this, tr("Delete Card"), tr("Could not delete card #: %1 . Please try again or contact tech support.").arg(cardNum));
    }
  }
}

void CardsWidget::insertCardRow(ArcadeCard &card)
{
  // Card Type, Card #, Balance, Last Active, Last Booth, In Use, Activated
  QStandardItem *cardTypeField, *cardNumField, *balanceField, *lastActiveField, *lastBoothField, *inUseField, *activatedField, *cardObjectField;

  cardTypeField = new QStandardItem(card.cardTypeToString());
  cardTypeField->setData(card.getCardType());

  cardNumField = new QStandardItem(card.getCardNum());
  cardNumField->setData(card.getCardNum());

  balanceField = new QStandardItem(QString("$%L1").arg(card.getCashValue(), 0, 'f', 2));
  balanceField->setData(card.getCredits());

  lastActiveField = new QStandardItem(card.getLastActive().isValid() ? card.getLastActive().toString("MM/dd/yyyy h:mm AP") : "N/A");
  lastActiveField->setData(card.getLastActive());

  lastBoothField = new QStandardItem(card.getDeviceNum() > 0 ? QString::number(card.getDeviceNum()) : "N/A");
  lastBoothField->setData(card.getDeviceNum());

  inUseField = new QStandardItem(card.getInUse() ? "True" : "False");
  inUseField->setData(card.getInUse());

  activatedField = new QStandardItem(card.getActivated().isValid() ? card.getActivated().toString("MM/dd/yyyy h:mm AP") : "Error");
  activatedField->setData(card.getActivated());

  cardObjectField = new QStandardItem;
  QVariant v;
  v.setValue(card);
  cardObjectField->setData(v);

  int row = cardsModel->rowCount();

  cardsModel->setItem(row, 0, cardTypeField);
  cardsModel->setItem(row, 1, cardNumField);
  cardsModel->setItem(row, 2, balanceField);
  cardsModel->setItem(row, 3, lastActiveField);
  cardsModel->setItem(row, 4, lastBoothField);
  cardsModel->setItem(row, 5, inUseField);
  cardsModel->setItem(row, 6, activatedField);
  cardsModel->setItem(row, 7, cardObjectField);

  if(card.getMadeBy() == "CAS Manager")
  {
      QColor rowColor = Qt::green;
      cardTypeField->setData(rowColor, Qt::BackgroundColorRole);
      cardNumField->setData(rowColor, Qt::BackgroundColorRole);
      balanceField->setData(rowColor, Qt::BackgroundColorRole);
      lastActiveField->setData(rowColor, Qt::BackgroundColorRole);
      lastBoothField->setData(rowColor, Qt::BackgroundColorRole);
      inUseField->setData(rowColor, Qt::BackgroundColorRole);
      activatedField->setData(rowColor, Qt::BackgroundColorRole);
  }
}

void CardsWidget::refreshCardListing()
{
  QLOG_DEBUG() << "Gettings all cards from database...";
//  this->setEnabled(false);
  if(settings->getValue(USE_SHARE_CARDS, DEFAULT_USE_SHARE_CARDS, false).toBool())
  {
      sharedCardService->getCards(0);
      updatePageNumWidgets(true);
  }
  else
  {
      dbMgr->getCards();
      updatePageNumWidgets(false);
  }
}

void CardsWidget::userAuthorizationReceived(bool success, QString response)
{
  disconnect(casServer, SIGNAL(userAuthorizationReceived(bool,QString)), this, SLOT(userAuthorizationReceived(bool,QString)));

  if (success)
  {
    QMessageBox::information(this, tr("Authenticated"), response);
    this->authTimer->start();
    this->cardAccess = true;
    this->addNewCard();
  }
  else
  {
    QMessageBox::warning(this, tr("Failed"), response);
  }
}

void CardsWidget::timeout()
{
  this->authTimer->stop();
  this->cardAccess = false;
}
