#ifndef HELPCENTERWIDGET_H
#define HELPCENTERWIDGET_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QKeyEvent>

class HelpCenterWidget : public QDialog
{
  Q_OBJECT
public:
  explicit HelpCenterWidget(QWidget *parent = 0);
  ~HelpCenterWidget();
  bool getShowOnStartup();
  
  enum TabName
  {
    TipOfTheDay,
    Documentation,
    Changelog
  };

  void setHtmlTips(const QStringList &value);
  void setHtmlDocumentation(const QString &value);
  void setHtmlChangelog(const QString &value);
  int countTips();

protected:
  void showEvent(QShowEvent *);
  void closeEvent(QCloseEvent *);
  void keyPressEvent(QKeyEvent *event);

signals:
  
public slots:
  void setCurrentTab(TabName tabName);
  void setShowOnStartup(bool checked);
  void setCurrentTip(int index);
  void gotoNextTip();
  void gotoPrevTip();

private slots:
  void loadContent();  
  
private:
  QVBoxLayout *containerLayout;
  QHBoxLayout *headerLayout;
  QHBoxLayout *footerLayout;
  QTabWidget *tabs;
  QWidget *tipOfTheDayWidget;
  QVBoxLayout *tipOfTheDayContainerLayout;
  QHBoxLayout *tipNavLayout;
  QWidget *documentationWidget;
  QVBoxLayout *documentationContainerLayout;
  QWidget *changelogWidget;
  QVBoxLayout *changelogContainerLayout;
  QTextBrowser *tipViewer;
  QTextBrowser *documentationViewer;
  QTextBrowser *changelogViewer;
  QLabel *logo;
  QLabel *title;
  QCheckBox *chkShowOnStartup;
  QPushButton *btnNextTip;
  QPushButton *btnPrevTip;
  QPushButton *btnClose;

  QStringList htmlTips;
  QString htmlDocumentation;
  QString htmlChangelog;
  int currentTipIndex;
};

#endif // HELPCENTERWIDGET_H
