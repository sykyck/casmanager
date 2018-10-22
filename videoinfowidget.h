#ifndef VIDEOINFOWIDGET_H
#define VIDEOINFOWIDGET_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QCheckBox>
#include <QPushButton>
#include "databasemgr.h"
#include "settings.h"
#include "dvdcopytask.h"

class VideoInfoWidget : public QDialog
{
    Q_OBJECT
public:
    explicit VideoInfoWidget(DatabaseMgr *dbMgr, Settings *settings, QWidget *parent = 0);
    void populateForm(const DvdCopyTask &copyTask);

protected:
    bool eventFilter(QObject *target, QEvent *event);

private slots:
    void showFrontCover();
    void showBackCover();

private:
    DatabaseMgr *dbMgr;
    Settings *settings;

    QLabel *lblUPC;
    QLabel *lblTitle;
    QLabel *lblProducer;
    QLabel *lblCategory;
    QLabel *lblSubcategory;
    QLabel *lblFrontCover;
    QLabel *lblBackCover;
    QLineEdit *txtUPC;
    QLineEdit *txtTitle;
    QLineEdit *txtProducer;
    QLineEdit *txtCategory;
    QLineEdit *txtSubcategory;
    QPushButton *btnClose;
    QGridLayout *gridLayout;
    QString frontCoverFile;
    QString backCoverFile;
};

#endif // VIDEOINFOWIDGET_H
