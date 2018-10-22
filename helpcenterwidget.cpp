#include "helpcenterwidget.h"
#include <QScrollBar>
#include "qslog/QsLog.h"

HelpCenterWidget::HelpCenterWidget(QWidget *parent) : QDialog(parent)
{
  // Widget will always remain on top of other windows and only has a close button (Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint)
  this->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);

  // Allow interaction with other windows
  this->setModal(false);

  currentTipIndex = 0;

  containerLayout = new QVBoxLayout;
  headerLayout = new QHBoxLayout;
  footerLayout = new QHBoxLayout;
  tabs = new QTabWidget;
  tipOfTheDayWidget = new QWidget;
  tipOfTheDayContainerLayout = new QVBoxLayout;
  tipNavLayout = new QHBoxLayout;
  documentationWidget = new QWidget;
  documentationContainerLayout = new QVBoxLayout;
  changelogWidget = new QWidget;
  changelogContainerLayout = new QVBoxLayout;
  tipViewer = new QTextBrowser;

  documentationViewer = new QTextBrowser;
  documentationViewer->setOpenExternalLinks(true);  
  changelogViewer = new QTextBrowser;
  logo = new QLabel("<img width=\"64\" height=\"64\" src=\"cas-mgr.png\" />");
  title = new QLabel(tr("CAS Manager - Help Center"));
  title->setStyleSheet("QLabel {font-size: 22px; font-weight: bold; font-family: Verdana;}");
  chkShowOnStartup = new QCheckBox(tr("Show on startup"));
  btnNextTip = new QPushButton(tr("Next Tip >"));
  btnPrevTip = new QPushButton(tr("< Prev Tip"));
  btnClose = new QPushButton(tr("Close"));

  connect(btnNextTip, SIGNAL(clicked()), this, SLOT(gotoNextTip()));
  connect(btnPrevTip, SIGNAL(clicked()), this, SLOT(gotoPrevTip()));
  connect(btnClose, SIGNAL(clicked()), this, SLOT(accept()));

  tipNavLayout->addStretch(2);
  tipNavLayout->addWidget(btnPrevTip, 0, Qt::AlignRight);
  tipNavLayout->addWidget(btnNextTip, 0, Qt::AlignRight);
  tipOfTheDayContainerLayout->addWidget(tipViewer, 3);
  tipOfTheDayContainerLayout->addLayout(tipNavLayout);

  tipOfTheDayWidget->setLayout(tipOfTheDayContainerLayout);

  documentationContainerLayout->addWidget(documentationViewer);
  changelogContainerLayout->addWidget(changelogViewer);

  documentationWidget->setLayout(documentationContainerLayout);
  changelogWidget->setLayout(changelogContainerLayout);

  tabs->addTab(tipOfTheDayWidget, tr("Tip of the Day"));
  tabs->addTab(documentationWidget, tr("Documentation"));
  tabs->addTab(changelogWidget, tr("Changelog"));

  headerLayout->addWidget(logo);
  headerLayout->addWidget(title, 2, Qt::AlignVCenter | Qt::AlignLeft);

  footerLayout->addWidget(chkShowOnStartup, 0, Qt::AlignLeft);
  footerLayout->addWidget(btnClose, 0, Qt::AlignRight);

  containerLayout->addLayout(headerLayout);
  containerLayout->addWidget(tabs, 2);
  containerLayout->addLayout(footerLayout);

  this->setLayout(containerLayout);
  this->setWindowTitle(tr("Help Center"));
  this->tabs->setFocus();
}

HelpCenterWidget::~HelpCenterWidget()
{
}

bool HelpCenterWidget::getShowOnStartup()
{
  return chkShowOnStartup->isChecked();
}

void HelpCenterWidget::setCurrentTab(HelpCenterWidget::TabName tabName)
{
  switch (tabName)
  {
    case TipOfTheDay:
      tabs->setCurrentWidget(tipOfTheDayWidget);
      break;

    case Documentation:
      tabs->setCurrentWidget(documentationWidget);
      break;

    case Changelog:
      tabs->setCurrentWidget(changelogWidget);
      break;
  }
}

void HelpCenterWidget::setShowOnStartup(bool checked)
{
  chkShowOnStartup->setChecked(checked);
}

void HelpCenterWidget::setCurrentTip(int index)
{
  if (index >= 0 && index < htmlTips.count())
  {
    currentTipIndex = index;
    tipViewer->setHtml(htmlTips.at(currentTipIndex));
  }
}

void HelpCenterWidget::loadContent()
{
  // If no tips of the day then set a default message
  if (htmlTips.count() == 0)
    htmlTips.append("<html><head><meta charset=\"UTF-8\"><title>Tip of the Day</title></head><body>No tips of the day found.</body></html>");

  if (htmlChangelog.isEmpty())
    htmlChangelog = "<html><head><meta charset=\"UTF-8\"><title>Changelog</title></head><body>No changelog found.</body></html>";

  if (htmlDocumentation.isEmpty())
    htmlDocumentation = "<html><head><meta charset=\"UTF-8\"><title>Documentation</title></head><body>No documentation found.</body></html>";

  tipViewer->setHtml(htmlTips.at(currentTipIndex));
  changelogViewer->setHtml(htmlChangelog);
  documentationViewer->setHtml(htmlDocumentation);
}

void HelpCenterWidget::gotoNextTip()
{
  if (++currentTipIndex >= htmlTips.count())
    currentTipIndex = 0;

  setCurrentTip(currentTipIndex);

  // Scroll to top
  //QScrollBar *vScrollBar = tipViewer->verticalScrollBar();
  //vScrollBar->triggerAction(QScrollBar::SliderToMinimum);
}

void HelpCenterWidget::gotoPrevTip()
{
  if (--currentTipIndex < 0)
    currentTipIndex = htmlTips.count() - 1;

  setCurrentTip(currentTipIndex);
}

void HelpCenterWidget::setHtmlTips(const QStringList &value)
{
  htmlTips = value;

  if (currentTipIndex >= htmlTips.count())
    currentTipIndex = 0;

  // Verify there is at least one item in the list to display
  if (htmlTips.count() > 0)
    tipViewer->setHtml(htmlTips.at(currentTipIndex));
  else
    tipViewer->clear();
}

void HelpCenterWidget::setHtmlChangelog(const QString &value)
{
  htmlChangelog = value;
  changelogViewer->setHtml(htmlChangelog);
}

int HelpCenterWidget::countTips()
{
  return htmlTips.count();
}

void HelpCenterWidget::showEvent(QShowEvent *)
{
  loadContent();
}

void HelpCenterWidget::closeEvent(QCloseEvent *)
{
  accept();
}

void HelpCenterWidget::keyPressEvent(QKeyEvent *event)
{
  // Override ESC key behaviour by calling closeEvent instead of the default reject() slot
  if (event->key() == Qt::Key_Escape)
    closeEvent(NULL);
}

void HelpCenterWidget::setHtmlDocumentation(const QString &value)
{
  htmlDocumentation = value;
  documentationViewer->setHtml(htmlDocumentation);
}
