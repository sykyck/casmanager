#-------------------------------------------------
#
# Project created by QtCreator 2014-05-04T21:27:51
#
#-------------------------------------------------
include(qslog/QsLog.pri)

QT       += core gui network sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = cas-mgr
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    qtsingleapplication/qtsinglecoreapplication.cpp \
    qtsingleapplication/qtsingleapplication.cpp \
    qtsingleapplication/qtlockedfile_unix.cpp \
    qtsingleapplication/qtlockedfile.cpp \
    qtsingleapplication/qtlocalpeer.cpp \
    movielibrarywidget.cpp \
    addvideowidget.cpp \
    databasemgr.cpp \
    dvdcopytask.cpp \
    videolookupservice.cpp \
    qjson/serializerrunnable.cpp \
    qjson/serializer.cpp \
    qjson/qobjecthelper.cpp \
    qjson/parserrunnable.cpp \
    qjson/parser.cpp \
    qjson/json_scanner.cpp \
    qjson/json_scanner.cc \
    qjson/json_parser.cc \
    sleephelper.cpp \
    movie.cpp \
    mediainfowidget.cpp \
    formpost/formpost.cpp \
    imageviewerwidget.cpp \
    global.cpp \
    moviechangewidget.cpp \
    moviechangeweeknumwidget.cpp \
    encdec.cpp \
    casserver.cpp \
    settings.cpp \
    boothstatus.cpp \
    boothsettingswidget.cpp \
    boothsettings.cpp \
    appsettingswidget.cpp \
    updater.cpp \
    alerter.cpp \
    cardswidget.cpp \
    collectionswidget.cpp \
    filehelper.cpp \
    dvdcopier.cpp \
    discautoloader.cpp \
    directorywatcher.cpp \
    discautoloadercontrolpanel.cpp \
    videotranscoder.cpp \
    viewtimesreportswidget.cpp \
    uftpserver.cpp \
    collectioncontainerwidget.cpp \
    collectionreportswidget.cpp \
    existingmoviechangewidget.cpp \
    exportmoviechangewidget.cpp \
    moviechangesetindex.cpp \
    filesystemwatcher.cpp \
    filecopier.cpp \
    helpcenterwidget.cpp \
    paymentgatewaywidget.cpp \
    creditcard.cpp \
    ccchargeresult.cpp \
    authorizenetservice.cpp \
    moviechangecontainerwidget.cpp \
    moviechangehistorywidget.cpp \
    messagewidget.cpp \
    moviechangeinfo.cpp \
    carddetailwidget.cpp \
    arcadecard.cpp \
    top-couchdb/couchdb.cpp \
    top-couchdb/couchdblistener.cpp \
    top-couchdb/couchdbquery.cpp \
    top-couchdb/couchdbresponse.cpp \
    top-couchdb/couchdbserver.cpp \
    qjson-backport/qjson.cpp \
    qjson-backport/qjsonarray.cpp \
    qjson-backport/qjsondocument.cpp \
    qjson-backport/qjsonobject.cpp \
    qjson-backport/qjsonparser.cpp \
    qjson-backport/qjsonvalue.cpp \
    qjson-backport/qjsonwriter.cpp \
    storeshift.cpp \
    storeshiftswidget.cpp \
    useraccountwidget.cpp \
    useraccount.cpp \
    videoinfowidget.cpp \
    cardauthorizewidget.cpp \
    clerksessionsreportingwidget.cpp \
    analytics/piwiktracker.cpp \
    analytics/piwikofflinetracker.cpp \
    sharedcardservices.cpp \
    forcemoviechangewidget.cpp

HEADERS  += mainwindow.h \
    qtsingleapplication/qtsinglecoreapplication.h \
    qtsingleapplication/QtSingleApplication \
    qtsingleapplication/qtsingleapplication.h \
    qtsingleapplication/QtLockedFile \
    qtsingleapplication/qtlockedfile.h \
    qtsingleapplication/qtlocalpeer.h \
    movielibrarywidget.h \
    addvideowidget.h \
    databasemgr.h \
    dvdcopytask.h \
    videolookupservice.h \
    qjson/stack.hh \
    qjson/serializerrunnable.h \
    qjson/serializer.h \
    qjson/qobjecthelper.h \
    qjson/qjson_export.h \
    qjson/qjson_debug.h \
    qjson/position.hh \
    qjson/parserrunnable.h \
    qjson/parser_p.h \
    qjson/parser.h \
    qjson/location.hh \
    qjson/json_scanner.h \
    qjson/json_parser.hh \
    qjson/FlexLexer.h \
    sleephelper.h \
    movie.h \
    mediainfowidget.h \
    formpost/formpostinterface.h \
    formpost/formpost.h \
    imageviewerwidget.h \
    global.h \
    moviechangewidget.h \
    moviechangeweeknumwidget.h \
    encdec.h \
    casserver.h \
    settings.h \
    boothstatus.h \
    boothsettingswidget.h \
    boothsettings.h \
    appsettingswidget.h \
    updater.h \
    alerter.h \
    cardswidget.h \
    collectionswidget.h \
    moviechangeinfo.h \
    filehelper.h \
    dvdcopier.h \
    discautoloader.h \
    directorywatcher.h \
    discautoloadercontrolpanel.h \
    videotranscoder.h \
    viewtimesreportswidget.h \
    uftpserver.h \
    collectioncontainerwidget.h \
    collectionreportswidget.h \
    existingmoviechangewidget.h \
    exportmoviechangewidget.h \
    moviechangesetindex.h \
    exportedmoviechangeset.h \
    filesystemwatcher.h \
    filecopier.h \
    helpcenterwidget.h \
    paymentgatewaywidget.h \
    creditcard.h \
    ccchargeresult.h \
    authorizenetservice.h \
    moviechangecontainerwidget.h \
    moviechangehistorywidget.h \
    messagewidget.h \
    carddetailwidget.h \
    arcadecard.h \
    top-couchdb/couchdb.h \
    top-couchdb/couchdbenums.h \
    top-couchdb/couchdblistener.h \
    top-couchdb/couchdbquery.h \
    top-couchdb/couchdbresponse.h \
    top-couchdb/couchdbserver.h \
    qjson-backport/qjson_p.h \
    qjson-backport/qjsonarray.h \
    qjson-backport/qjsondocument.h \
    qjson-backport/qjsonobject.h \
    qjson-backport/qjsonparser_p.h \
    qjson-backport/qjsonvalue.h \
    qjson-backport/qjsonwriter_p.h \
    storeshift.h \
    storeshiftswidget.h \
    useraccountwidget.h \
    useraccount.h \
    videoinfowidget.h \
    cardauthorizewidget.h \
    clerksessionsreportingwidget.h \
    analytics/piwiktracker.h \
    analytics/piwikofflinetracker.h \
    sharedcardservices.h \
    forcemoviechangewidget.h

OTHER_FILES += \
    notes.txt \
    installer/transcode-video.sh \
    installer/install.sh \
    installer/copy-dvd.sh
