#ifndef COUCHDBRESPONSE_H
#define COUCHDBRESPONSE_H

#include <QObject>
#include "qjson-backport/qjsondocument.h"
#include "qjson-backport/qjsonobject.h"
#include "couchdbenums.h"

class CouchDBQuery;
class CouchDBResponsePrivate;
class CouchDBResponse : public QObject
{
    Q_OBJECT
public:
    explicit CouchDBResponse(QObject *parent = 0);
    virtual ~CouchDBResponse();

    CouchDBQuery* query() const;
    void setQuery(CouchDBQuery *query);

    CouchDBReplyStatus status() const;
    void setStatus(const CouchDBReplyStatus& status);

    QString revisionData() const;
    void setRevisionData(const QString& revision);

    QByteArray data() const;
    void setData(const QByteArray& data);

    QJsonDocument document() const;
    QJsonObject documentObj() const;

private:
    Q_DECLARE_PRIVATE(CouchDBResponse)
    CouchDBResponsePrivate * const d_ptr;
};

#endif // COUCHDBRESPONSE_H
