#include "addvideowidget.h"
#include "qslog/QsLog.h"
#include <QStringList>
#include <QMessageBox>

static int MIN_UPC_LENGTH = 10;

AddVideoWidget::AddVideoWidget(int currentMovieChangeSetSize, Settings *settings, DatabaseMgr *dbMgr, QWidget *parent) : QDialog(parent)
{
  // TODO: Use QFormLayout instead
  this->currentMovieChangeSetSize = currentMovieChangeSetSize;
  this->settings = settings;
  this->dbMgr = dbMgr;  

  if (currentMovieChangeSetSize > 0)
  {
    QLOG_DEBUG() << QString("The existing movie change set being added to has %1 movie(s)").arg(currentMovieChangeSetSize);
  }

  lblUPC = new QLabel(tr("UPC"));
  lblCategory = new QLabel(tr("Category"));
  lblVideoQueue = new QLabel(tr("DVD Queue"));
  lblVideoQueueSize = new QLabel(tr("DVD Count: 0"));

  txtUPC = new QLineEdit;
  txtUPC->setMaxLength(15);

  if (settings->getValue(SHOW_EXTRA_ADD_VIDEO_FIELDS).toBool())
  {
    videoCategories = new QComboBox;
    //videoCategories->setEditable(true);
    //videoCategories->setInsertPolicy(QComboBox::NoInsert);

    // Store employees sometimes select Non-Adult as the category by mistake. There is some special handling for the Non-Adult category in the CASplayer software
    // where it's ignored in certain situations. Need to look into the logic behind that and either remove it as an option in the CAS Manager or change CASplayer to deal with it
    QStringList categoryList = dbMgr->getVideoAttributes(2);
    if (categoryList.contains("Non-Adult"))
      categoryList.removeAll("Non-Adult");

    videoCategories->addItems(categoryList);
    videoCategories->insertItem(0, "");
    videoCategories->setCurrentIndex(0);
  }

  btnStart = new QPushButton(tr("Start Copying"));
  btnStart->setDefault(false);
  btnStart->setAutoDefault(false);
  connect(btnStart, SIGNAL(clicked()), this, SLOT(accept()));

  btnCancel = new QPushButton(tr("Cancel"));
  btnCancel->setDefault(false);
  btnCancel->setAutoDefault(false);
  connect(btnCancel, SIGNAL(clicked()), this, SLOT(reject()));

  btnAdd = new QPushButton(tr("Add"));
  btnAdd->setDefault(false);
  btnAdd->setAutoDefault(false);
  connect(btnAdd, SIGNAL(clicked()), this, SLOT(addToQueue()));

  btnRemove = new QPushButton(tr("Remove"));
  btnRemove->setDefault(false);
  btnRemove->setAutoDefault(false);
  connect(btnRemove, SIGNAL(clicked()), this, SLOT(removeFromQueue()));

  videoQueue = new QListWidget;
  videoQueue->setSelectionMode(QAbstractItemView::SingleSelection);

  gridLayout = new QGridLayout;
  gridLayout->addWidget(lblUPC, 1, 0);
  gridLayout->addWidget(txtUPC, 1, 1, 1, 2);

  // Only show Categories drop down if flag set
  if (settings->getValue(SHOW_EXTRA_ADD_VIDEO_FIELDS).toBool())
  {
    connect(txtUPC, SIGNAL(returnPressed()), videoCategories, SLOT(setFocus()));

    gridLayout->addWidget(lblCategory, 2, 0);
    gridLayout->addWidget(videoCategories, 2, 1, 1, 2);
  }
  else
    connect(txtUPC, SIGNAL(returnPressed()), this, SLOT(addToQueue()));

  gridLayout->addWidget(btnAdd, 3, 1);
  gridLayout->addWidget(btnRemove, 3, 2);
  gridLayout->addWidget(lblVideoQueue, 4, 0, 1, 1, Qt::AlignTop);
  gridLayout->addWidget(videoQueue, 4, 1, 1, 2);
  gridLayout->addWidget(lblVideoQueueSize, 5, 1);
  gridLayout->addWidget(btnStart, 7, 1);
  gridLayout->addWidget(btnCancel, 7, 2);

  this->setWindowTitle(tr("Add DVDs to Queue"));
  this->setLayout(gridLayout);

  // Disable maximize,minimize and resizing of window
  this->setFixedSize(this->sizeHint());
}

AddVideoWidget::~AddVideoWidget()
{
  gridLayout->deleteLater();
}

QStringList AddVideoWidget::getDvdQueue()
{
  QStringList queue;

  while (videoQueue->count() > 0)
  {
    QListWidgetItem *item = videoQueue->takeItem(0);
    queue.append(item->data(Qt::UserRole).toString());

    delete item;
  }

  return queue;
}

void AddVideoWidget::accept()
{
  if (btnStart->hasFocus())
  {
    QString errorMessage = isValidInput();

    if (errorMessage.isEmpty())
      QDialog::accept();
    else
    {
      QLOG_DEBUG() << QString("Problem with DVD queue: %1").arg(errorMessage);
      QMessageBox::warning(this, tr("DVD Queue"), tr("Please correct the following problems:\n\n%1").arg(errorMessage));

      txtUPC->setFocus();
      txtUPC->selectAll();
    }
  }
}

void AddVideoWidget::reject()
{
  bool continueRejecting = true;

  if (videoQueue->count() > 0)
  {
    QLOG_DEBUG() << QString("User wants to cancel creating DVD Copy Job after adding items, make sure this is not a mistake");
    if (QMessageBox::question(this, tr("Are you sure?"), tr("Are you sure you want to cancel creating a DVD Copy Job?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
    {
      QLOG_DEBUG() << QString("User changed mind");
      continueRejecting = false;
    }
  }

  if (continueRejecting)
    QDialog::reject();
}

void AddVideoWidget::addToQueue()
{
  QString upc = txtUPC->text().trimmed();

  QString category = "";
  if (settings->getValue(SHOW_EXTRA_ADD_VIDEO_FIELDS).toBool())
    category = videoCategories->currentText();

  QLOG_DEBUG() << QString("User submitted UPC: %1").arg(upc);

  // UPC needs to be at least x digits and either the category should be selected or extra fields are hidden
  if (upc.length() >= MIN_UPC_LENGTH && (!category.isEmpty() || !settings->getValue(SHOW_EXTRA_ADD_VIDEO_FIELDS).toBool()))
  {
    // Verify the UPC only contains digits
    QRegExp re("\\d*");  // a digit (\d), zero or more times (*)
    if (!re.exactMatch(upc))
    {
      QLOG_DEBUG() << QString("Invalid UPC");
      QMessageBox::warning(this, tr("Invalid"), tr("The UPC should only contain numbers."), QMessageBox::Ok);
      txtUPC->setFocus();
      txtUPC->selectAll();
    }
    else if (dbMgr->upcExists(upc))
    {
      QLOG_DEBUG() << QString("UPC already exists in the system");
      QMessageBox::warning(this, tr("Already Exists"), tr("The UPC already exists in the system."), QMessageBox::Ok);
      txtUPC->setFocus();
      txtUPC->selectAll();
    }
    else
    {
      if (settings->getValue(SHOW_EXTRA_ADD_VIDEO_FIELDS).toBool())
        QLOG_DEBUG() << QString("User submitted UPC: %1, Category: %2 to DVD queue").arg(upc).arg(category);
      else
        QLOG_DEBUG() << QString("User submitted UPC: %1 to DVD queue").arg(upc);

      // Verify the UPC is not a duplicate
      QList<QListWidgetItem *> matches;

      if (settings->getValue(SHOW_EXTRA_ADD_VIDEO_FIELDS).toBool())
        matches = videoQueue->findItems(QString("UPC: %1,").arg(upc), Qt::MatchStartsWith);
      else
        matches = videoQueue->findItems(upc, Qt::MatchExactly);

      if (matches.count() > 0)
      {
        QLOG_DEBUG() << QString("User tried to submit a UPC that is already in the queue");
        QMessageBox::warning(this, tr("Duplicate"), tr("The UPC is already in the list."), QMessageBox::Ok);
        txtUPC->setFocus();
        txtUPC->selectAll();
      }
      else
      {
        if (currentMovieChangeSetSize + videoQueue->count() + 1 > settings->getValue(MAX_MOVIE_CHANGE_SET_SIZE).toInt())
        {
          QLOG_DEBUG() << QString("User reached the maximum number of DVDs in the queue which is: %1").arg(settings->getValue(MAX_MOVIE_CHANGE_SET_SIZE).toInt());
          QMessageBox::warning(this, tr("Queue Full"), tr("You cannot add anymore to this queue. The maximum number of DVDs is %1. If you are adding to an existing movie change set then those are included in the count.").arg(settings->getValue(MAX_MOVIE_CHANGE_SET_SIZE).toInt()), QMessageBox::Ok);
          txtUPC->setFocus();
          txtUPC->selectAll();
        }
        else
        {
          QListWidgetItem *item = new QListWidgetItem;

          if (settings->getValue(SHOW_EXTRA_ADD_VIDEO_FIELDS).toBool())
          {
            item->setData(Qt::UserRole, QString("%1|%2").arg(upc).arg(category));
            item->setText(QString("UPC: %1, Category: %2").arg(upc).arg(category));
          }
          else
          {
            item->setData(Qt::UserRole, upc);
            item->setText(upc);
          }

          if (settings->getValue(ENABLE_AUTO_LOADER).toBool())
            videoQueue->addItem(item);
          else
            videoQueue->insertItem(0, item);

          txtUPC->clear();
          txtUPC->setFocus();

          lblVideoQueueSize->setText(tr("DVD Count: %1").arg(videoQueue->count()));
        }
      }
    }
  }
  else
  {
    if (settings->getValue(SHOW_EXTRA_ADD_VIDEO_FIELDS).toBool())
    {
      QLOG_DEBUG() << QString("User submitted incomplete information for DVD. UPC: %1, Category: %2").arg(upc).arg(category);
      QMessageBox::warning(this, tr("Incomplete"), tr("Specify the UPC and Category of the DVD before adding to the queue."), QMessageBox::Ok);

      if (upc.length() < MIN_UPC_LENGTH)
      {
        txtUPC->setFocus();
        txtUPC->selectAll();
      }
      else
        videoCategories->setFocus();
    }
    else
    {
      QLOG_DEBUG() << QString("User submitted incomplete information for DVD. UPC: %1").arg(upc);
      QMessageBox::warning(this, tr("Incomplete"), tr("Specify the UPC of the DVD before adding to the queue."), QMessageBox::Ok);
      txtUPC->setFocus();
      txtUPC->selectAll();
    }
  }
}

void AddVideoWidget::removeFromQueue()
{
  if (videoQueue->currentRow() >= 0)
  {
    QListWidgetItem *item = videoQueue->takeItem(videoQueue->currentRow());
    QStringList record = item->data(Qt::UserRole).toString().split("|");

    if (settings->getValue(SHOW_EXTRA_ADD_VIDEO_FIELDS).toBool())
      QLOG_DEBUG() << QString("Removing UPC: %1, Category: %2 from the DVD queue.").arg(record.at(0)).arg(record.at(1));
    else
      QLOG_DEBUG() << QString("Removing UPC: %1 from the DVD queue.").arg(record.at(0));

    lblVideoQueueSize->setText(tr("DVD Count: %1").arg(videoQueue->count()));

    delete item;
  }
  else
  {
    QLOG_DEBUG() << QString("User tried to remove a DVD from the queue without selecting anything");
    QMessageBox::warning(this, tr("No item"), tr("Select a DVD from the queue to remove."), QMessageBox::Ok);
  }

  // Return focus to UPC field to wait for next barcode
  txtUPC->setFocus();
  txtUPC->selectAll();
}

QString AddVideoWidget::isValidInput()
{
  QString errorMessage = "";

  if (videoQueue->count() == 0)
    errorMessage += "- Add at least one UPC to the DVD Queue before continuing.\n";

  txtUPC->setText(txtUPC->text().trimmed());
  if (txtUPC->text().length() > 0)
    errorMessage += "- A value was found in the UPC field. Don't forget to click the Add button to put it in the queue. If you don't want to add this UPC then clear the UPC field.\n";

  return errorMessage;
}

