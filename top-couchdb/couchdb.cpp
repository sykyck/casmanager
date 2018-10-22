#include "couchdb.h"
#include "couchdbserver.h"
#include "couchdbquery.h"
#include "couchdblistener.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QSslCertificate>
#include "qslog/QsLog.h"
#include <QNetworkCookieJar>
#include "qjson-backport/qjsonarray.h"

class CouchDBPrivate
{
public:
  CouchDBPrivate() :
    server(0),
    cleanServerOnQuit(true),
    networkManager(0),
    processing(false)
  {}

  virtual ~CouchDBPrivate()
  {
    if(server && cleanServerOnQuit) delete server;

    if(networkManager) delete networkManager;
  }

  CouchDBServer *server;
  bool cleanServerOnQuit;

  QNetworkAccessManager *networkManager;
  QHash<QNetworkReply*, CouchDBQuery*> currentQueries;
  QList<CouchDBQuery *> couchDbQueue;
  bool processing;
};

CouchDB::CouchDB(QObject *parent) :
  QObject(parent),
  d_ptr(new CouchDBPrivate)
{
  Q_D(CouchDB);

  d->server = new CouchDBServer(this);
  d->networkManager = new QNetworkAccessManager(this);

  // TODO: Move to settings!!!
  // It is necessary to add the server's certificate to our list of trusted certs since this
  // is self-signed
  QList<QSslCertificate> trustedCas = QSslCertificate::fromPath("/etc/couchdb/cert/couchdb.pem");
  if (trustedCas.empty())
  {
    QLOG_ERROR() << QString("Could not load the database server's certicate");
  }
  else
  {
    QLOG_DEBUG() << QString("Loaded database server's certificate");
    sslConfig.setCaCertificates(trustedCas);
  }
}

CouchDB::~CouchDB()
{
  delete d_ptr;
}

CouchDBServer *CouchDB::server() const
{
  Q_D(const CouchDB);
  return d->server;
}

void CouchDB::setServer(CouchDBServer *server)
{
  Q_D(CouchDB);
  if(d->server && d->cleanServerOnQuit) delete d->server;

  d->server = server;
  d->cleanServerOnQuit = false;
}

void CouchDB::setServerConfiguration(const QString &url, const int &port, const QString &username, const QString &password)
{
  Q_D(CouchDB);
  d->server->setUrl(url);
  d->server->setPort(port);
  if(!username.isEmpty() && !password.isEmpty()) d->server->setCredential(username, password);
}

void CouchDB::executeQuery(CouchDBQuery *query)
{
  Q_D(CouchDB);

  if(query->server()->hasCredential() && query->operation() != COUCHDB_STARTSESSION)
  {
    query->request()->setRawHeader("Authorization", "Basic " + query->server()->credential());
  }

  //qDebug() << "Invoked url:" << query->operation() << query->request()->url().toString();

  query->request()->setSslConfiguration(sslConfig);

  QNetworkReply *reply;  
  switch(query->operation())
  {
    case COUCHDB_CHECKINSTALLATION:
    default:
      reply = d->networkManager->get(*query->request());
      break;

    case COUCHDB_STARTSESSION:
      reply = d->networkManager->post(*query->request(), query->body());
      break;

    case COUCHDB_ENDSESSION:
      reply = d->networkManager->deleteResource(*query->request());
      break;

    case COUCHDB_LISTDATABASES:
      reply = d->networkManager->get(*query->request());
      break;

    case COUCHDB_CREATEDATABASE:
      reply = d->networkManager->put(*query->request(), query->body());
      break;

    case COUCHDB_DELETEDATABASE:
      reply = d->networkManager->deleteResource(*query->request());
      break;

    case COUCHDB_LISTDOCUMENTS:
      reply = d->networkManager->get(*query->request());
      break;

    case COUCHDB_RETRIEVEREVISION:
      reply = d->networkManager->head(*query->request());
      break;

    case COUCHDB_RETRIEVEDOCUMENT:
      reply = d->networkManager->get(*query->request());
      break;

    case COUCHDB_CREATEDOCUMENT:
      reply = d->networkManager->post(*query->request(), query->body());
      break;

    case COUCHDB_UPDATEDOCUMENT:
      reply = d->networkManager->put(*query->request(), query->body());
      break;

    case COUCHDB_DELETEDOCUMENT:
      reply = d->networkManager->deleteResource(*query->request());
      break;

    case COUCHDB_UPLOADATTACHMENT:
      reply = d->networkManager->put(*query->request(), query->body());
      break;

    case COUCHDB_DELETEATTACHMENT:
      reply = d->networkManager->deleteResource(*query->request());
      break;

    case COUCHDB_REPLICATEDATABASE:
      reply = d->networkManager->post(*query->request(), query->body());
      break;

    case COUCHDB_RETRIEVEVIEW:
      reply = d->networkManager->get(*query->request());
      break;

    case COUCHDB_BULKDELETE:
      reply = d->networkManager->post(*query->request(), query->body());
      break;
  }

  if (query->operation() != COUCHDB_REPLICATEDATABASE)
  {
    connect(query, SIGNAL(timeout()), SLOT(queryTimeout()));
    query->startTimeoutTimer();
  }

  connect(reply, SIGNAL(finished()), this, SLOT(queryFinished()));
  connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
  d->currentQueries[reply] = query;
}

void CouchDB::queryFinished()
{
  Q_D(CouchDB);

  QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
  if(!reply) return;

  QByteArray data;
  CouchDBQuery *query = d->currentQueries[reply];
  bool hasError = false;  

  if (reply->error() == QNetworkReply::NoError)
  {
    data = reply->readAll();
  }
  else
  {
    // Error code 299 (UnknownContentError) is returned when there is a document revision conflict
    // so we still want to parse the response
    if (reply->error() == QNetworkReply::UnknownContentError)
      data = reply->readAll();
    else
      QLOG_ERROR() << QString("Database server error code: %1").arg(reply->error());

    hasError = true;
  }

  CouchDBResponse response;
  response.setQuery(query);
  response.setData(data);
  response.setStatus(hasError || (query->operation() != COUCHDB_CHECKINSTALLATION && query->operation() != COUCHDB_RETRIEVEDOCUMENT &&
                                                                                                           !response.documentObj().value("ok").toBool()) ? COUCHDB_ERROR : COUCHDB_SUCCESS);

  switch(query->operation())
  {
    case COUCHDB_CHECKINSTALLATION:
    default:
      if (!hasError)
        response.setStatus(response.documentObj().contains("couchdb") ? COUCHDB_SUCCESS : COUCHDB_ERROR);
      emit installationChecked(response);
      break;

    case COUCHDB_STARTSESSION:
      if (hasError && reply->error() >= 201 && reply->error() <= 299)
        response.setStatus(COUCHDB_AUTHERROR);
      emit sessionStarted(response);
      break;

    case COUCHDB_ENDSESSION:
      emit sessionEnded(response);
      break;

    case COUCHDB_LISTDATABASES:
      if (!hasError)
        response.setStatus(COUCHDB_SUCCESS);

      emit databasesListed(response);
      break;

    case COUCHDB_CREATEDATABASE:
      emit databaseCreated(response);
      break;

    case COUCHDB_DELETEDATABASE:
      emit databaseDeleted(response);
      break;

    case COUCHDB_LISTDOCUMENTS:
      emit documentsListed(response);
      break;

    case COUCHDB_RETRIEVEREVISION:
    {
      QString revision = reply->rawHeader("ETag");
      revision.remove("\"");
      response.setRevisionData(revision);

      // If flag set then the call was made by this class to resolve a document update conflict
      if (query->property("resolveConflict").toBool())
      {
        QLOG_DEBUG() << QString("Retrieved revision, updating revision in update queue to %1").arg(response.revisionData());

        CouchDBQuery *pendingQuery = d->couchDbQueue[0];
        QJsonDocument doc = QJsonDocument::fromJson(pendingQuery->body());
        QJsonObject obj = doc.object();
        obj["_rev"] = response.revisionData();
        doc.setObject(obj);
        pendingQuery->setBody(doc.toJson(true));

        // The processing flag should be set so clear it now since we're done getting the revision
        // At the end of this method the update queue will be checked again and then the pending update
        // will be attempted again with the new revision ID
        d->processing = false;
      }
      else
        emit revisionRetrieved(response);

      break;
    }

    case COUCHDB_RETRIEVEDOCUMENT:
      QLOG_DEBUG() << "Retrieve doc response:" << response.documentObj();

      emit documentRetrieved(response);
      break;

    case COUCHDB_CREATEDOCUMENT:
    {
      QJsonObject obj = response.documentObj();
      QLOG_DEBUG() << "Create doc response:" << obj;

      if (response.status() == COUCHDB_SUCCESS &&
          obj.contains("id") && obj.contains("rev"))
      {
        // Get revision ID from response before replacing with the original document
        QString revID = obj["rev"].toString();

        // Replace response with the revised document that was sent to database
        response.setData(query->body());
        obj = response.documentObj();

        // Add rev property to document so the caller has access to this
        obj["_rev"] = revID;

        response.setData(QJsonDocument(obj).toJson(true));
      }
      emit documentCreated(response);
    }
      break;

    case COUCHDB_UPDATEDOCUMENT:
    {
      bool emitResponse = false;
      QJsonObject obj = response.documentObj();
      QLOG_DEBUG() << "Update doc response:" << obj;

      if (response.status() == COUCHDB_SUCCESS &&
          obj.contains("id") && obj.contains("rev"))
      {
        // Get revision ID from response before replacing with the original document
        QString revID = obj["rev"].toString();

        // Replace response with the revised document that was sent to database
        response.setData(query->body());
        obj = response.documentObj();

        // Add rev property to document so the caller has access to this
        obj["_rev"] = revID;

        response.setData(QJsonDocument(obj).toJson(true));

        if (!d->couchDbQueue.isEmpty())
        {
          CouchDBQuery *pendingQuery = d->couchDbQueue.takeFirst();
          pendingQuery->deleteLater();

          // Look for other updates that reference the same database and document ID
          // If found then update revision ID
          for (int i = 0; i < d->couchDbQueue.count(); ++i)
          {
            if (d->couchDbQueue[i]->documentID() == query->documentID() &&
                d->couchDbQueue[i]->database() == query->database())
            {
              QLOG_DEBUG() << QString("Found another update in the queue referencing doc ID: %1 in database: %2, updating rev ID to: %3")
                              .arg(query->documentID())
                              .arg(query->database())
                              .arg(revID);

              // Update rev ID of document so it doesn't result in a conflict error
              QJsonDocument doc = QJsonDocument::fromJson(d->couchDbQueue[i]->body());
              QJsonObject obj2 = doc.object();
              obj2["_rev"] = revID;
              doc.setObject(obj2);
              d->couchDbQueue[i]->setBody(doc.toJson(true));

              break;
            }
          }
        }

        d->processing = false;
        emitResponse = true;
      }
      else
      {
        if (obj["error"].toString().contains("conflict", Qt::CaseInsensitive))
        {
          if (query->property("numRetries").toInt() < MAX_UPDATE_RETRIES)
          {
            // Update the query object in the couchDbQueue since this query instance will be deleted
            d->couchDbQueue[0]->setProperty("numRetries", query->property("numRetries").toInt() + 1);

            QLOG_DEBUG() << QString("Update conflict with doc ID: %1, database: %2, requesting latest rev ID")
                            .arg(query->documentID())
                            .arg(query->database());

            // The document has been updated since we last retrieved it
            // request the latest revision and try updating again
            retrieveRevision(query->database(), query->documentID(), true);
          }
          else
          {
            // Reach maximum retries, give up
            if (!d->couchDbQueue.isEmpty())
            {
              QLOG_ERROR() << QString("Giving up on updating doc ID: %1, database: %2, reached maximum retries")
                              .arg(query->documentID())
                              .arg(query->database());

              CouchDBQuery *pendingQuery = d->couchDbQueue.takeFirst();
              pendingQuery->deleteLater();
            }

            d->processing = false;
            emitResponse = true;
          }
        }
        else
        {
          // Unknown error, give up
          if (!d->couchDbQueue.isEmpty())
          {
            QLOG_ERROR() << QString("Could not update document doc ID: %1, database: %2, due to unknown error")
                            .arg(query->documentID())
                            .arg(query->database());

            CouchDBQuery *pendingQuery = d->couchDbQueue.takeFirst();
            pendingQuery->deleteLater();
          }

          d->processing = false;
          emitResponse = true;
        }
      }

      // Only emit respone if we aren't trying to update the document again
      if (emitResponse)
        emit documentUpdated(response);
    }
      break;

    case COUCHDB_DELETEDOCUMENT:
    {
      // Provide the document ID in the response
      QJsonObject obj;
      obj["id"] = query->documentID();

      response.setData(QJsonDocument(obj).toJson(true));
      emit documentDeleted(response);
    }
      break;

    case COUCHDB_UPLOADATTACHMENT:
      emit attachmentUploaded(response);
      break;

    case COUCHDB_DELETEATTACHMENT:
      emit attachmentDeleted(response);
      break;

    case COUCHDB_REPLICATEDATABASE:
      emit databaseReplicated(response);
      break;

    case COUCHDB_RETRIEVEVIEW:
      emit databaseViewRetrieved(response);
      break;

    case COUCHDB_BULKDELETE:
      if (!hasError)
        response.setStatus(COUCHDB_SUCCESS);

      emit bulkDocumentsDeleted(response);
      break;
  }

  d->currentQueries.remove(reply);
  reply->deleteLater();

  delete query;

  checkUpdateQueue();
}

void CouchDB::queryTimeout()
{
  CouchDBQuery *query = qobject_cast<CouchDBQuery*>(sender());

  if (!query)
    return;

  QLOG_ERROR() << QString("Database request timed out. Retrying...");

  executeQuery(query);
}

void CouchDB::checkUpdateQueue()
{
  Q_D(CouchDB);

  if (!d->couchDbQueue.isEmpty())
  {
    if (!d->processing)
    {
      d->processing = true;

      // Use properties from next item in queue to build query
      // Separate instances of the CouchDBQuery class are used
      // since the query is released from memory after a query finishes
      // and we need to keep it around longer in the queue
      CouchDBQuery *queueItem = d->couchDbQueue.first();

      QByteArray postDataSize = QByteArray::number(queueItem->body().size());

      CouchDBQuery *query = new CouchDBQuery(queueItem->server(), this);
      query->setUrl(QString("%1/%2/%3").arg(queueItem->server()->baseURL(), queueItem->database(), queueItem->documentID()));
      query->setOperation(COUCHDB_UPDATEDOCUMENT);
      query->setDatabase(queueItem->database());
      query->setDocumentID(queueItem->documentID());
      query->request()->setRawHeader("Accept", "application/json");
      query->request()->setRawHeader("Content-Type", "application/json");
      query->request()->setRawHeader("Content-Length", postDataSize);
      query->setBody(queueItem->body());
      query->setProperty("numRetries", queueItem->property("numRetries"));

      executeQuery(query);
    }
    else
    {
      QLOG_DEBUG() << QString("Checking database update queue later, currently busy...");
    }
  }
  else
  {
    QLOG_DEBUG() << QString("Database update queue is now empty");
  }
}

void CouchDB::checkInstallation()
{
  Q_D(CouchDB);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1").arg(d->server->baseURL()));
  query->setOperation(COUCHDB_CHECKINSTALLATION);
  //QLOG_DEBUG() << query->url().toString() << query->server()->port();
  executeQuery(query);
}

void CouchDB::startSession(const QString &username, const QString &password)
{
  Q_D(CouchDB);

  //QUrlQuery postData;
  QUrl postData;
  postData.addQueryItem("name", username);
  postData.addQueryItem("password", password);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/_session").arg(d->server->baseURL()));
  query->setOperation(COUCHDB_STARTSESSION);
  query->request()->setRawHeader("Accept", "application/json");
  query->request()->setRawHeader("Content-Type", "application/x-www-form-urlencoded");
  //query->setBody(postData.toString(QUrl::FullyEncoded).toUtf8());
  query->setBody(postData.toEncoded());

  executeQuery(query);
}

void CouchDB::endSession()
{
  Q_D(CouchDB);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/_session").arg(d->server->baseURL()));
  query->setOperation(COUCHDB_ENDSESSION);

  executeQuery(query);
}

void CouchDB::listDatabases()
{
  Q_D(CouchDB);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/_all_dbs").arg(d->server->baseURL()));
  query->setOperation(COUCHDB_LISTDATABASES);

  executeQuery(query);
}

void CouchDB::createDatabase(const QString &database)
{
  Q_D(CouchDB);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/%2").arg(d->server->baseURL(), database));
  query->setOperation(COUCHDB_CREATEDATABASE);
  query->setDatabase(database);

  executeQuery(query);
}

void CouchDB::deleteDatabase(const QString &database)
{
  Q_D(CouchDB);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/%2").arg(d->server->baseURL(), database));
  query->setOperation(COUCHDB_DELETEDATABASE);
  query->setDatabase(database);

  executeQuery(query);
}

void CouchDB::listDocuments(const QString& database)
{
  Q_D(CouchDB);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/%2/_all_docs").arg(d->server->baseURL(), database));
  query->setOperation(COUCHDB_LISTDOCUMENTS);
  query->setDatabase(database);

  executeQuery(query);
}

void CouchDB::retrieveRevision(const QString &database, const QString &id, bool resolveConflict)
{
  Q_D(CouchDB);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/%2/%3").arg(d->server->baseURL(), database, id));
  query->setOperation(COUCHDB_RETRIEVEREVISION);
  query->setDatabase(database);
  query->setDocumentID(id);

  // If flag set then add property to query so when it returns this can be
  // handled differently
  if (resolveConflict)
    query->setProperty("resolveConflict", true);

  executeQuery(query);
}

void CouchDB::retrieveDocument(const QString &database, const QString &id, const QString &callerID)
{
  Q_D(CouchDB);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/%2/%3").arg(d->server->baseURL(), database, id));
  query->setOperation(COUCHDB_RETRIEVEDOCUMENT);
  query->setDatabase(database);
  query->setDocumentID(id);

  // Used by caller of this slot to route the result
  query->setProperty("callerID", callerID);

  executeQuery(query);
}

void CouchDB::updateDocument(const QString &database, const QString &id, QByteArray document, const QString &callerID)
{
  Q_D(CouchDB);

  QByteArray postDataSize = QByteArray::number(document.size());

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/%2/%3").arg(d->server->baseURL(), database, id));
  query->setOperation(COUCHDB_UPDATEDOCUMENT);
  query->setDatabase(database);
  query->setDocumentID(id);
  query->request()->setRawHeader("Accept", "application/json");
  query->request()->setRawHeader("Content-Type", "application/json");
  query->request()->setRawHeader("Content-Length", postDataSize);
  query->setBody(document);

  QLOG_DEBUG() << QString("Queueing update doc: %1").arg(document.data());

  // Add counter to limit the number of times we try to update the document
  query->setProperty("numRetries", 0);

  // Used by caller of this slot to route the result
  query->setProperty("callerID", callerID);

  d->couchDbQueue.append(query);
  checkUpdateQueue();
}

void CouchDB::createDocument(const QString &database, QByteArray document, const QString &callerID)
{
  Q_D(CouchDB);

  QByteArray postDataSize = QByteArray::number(document.size());

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/%2/").arg(d->server->baseURL(), database));
  query->setOperation(COUCHDB_CREATEDOCUMENT);
  query->setDatabase(database);

  // Used by caller of this slot to route the result
  query->setProperty("callerID", callerID);

  query->request()->setRawHeader("Accept", "application/json");
  query->request()->setRawHeader("Content-Type", "application/json");
  query->request()->setRawHeader("Content-Length", postDataSize);
  query->setBody(document);

  executeQuery(query);
}

void CouchDB::deleteDocument(const QString &database, const QString &id, const QString &revision, const QString &callerID)
{
  Q_D(CouchDB);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/%2/%3?rev=%4").arg(d->server->baseURL(), database, id, revision));
  query->setOperation(COUCHDB_DELETEDOCUMENT);
  query->setDatabase(database);
  query->setDocumentID(id);

  // Used by caller of this slot to route the result
  query->setProperty("callerID", callerID);

  executeQuery(query);
}

void CouchDB::bulkDocumentsDelete(const QString &database, QByteArray &documents, const QString &callerID)
{
  // The expected layout for deleting documents in bulk is:
  // {
  //   "docs": [
  //     {"_id": "", "_rev": "", "_deleted": true},
  //     ...
  //   ]
  // }

  // Response layout
  // [
  //   {"id": "", "ok": true, "rev": ""},
  //   ...
  // ]
  //
  Q_D(CouchDB);

  QByteArray postDataSize = QByteArray::number(documents.size());

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/%2/_bulk_docs").arg(d->server->baseURL(), database));
  query->setOperation(COUCHDB_BULKDELETE);
  query->setDatabase(database);

  // Used by caller of this slot to route the result
  query->setProperty("callerID", callerID);

  query->request()->setRawHeader("Accept", "application/json");
  query->request()->setRawHeader("Content-Type", "application/json");
  query->request()->setRawHeader("Content-Length", postDataSize);
  query->setBody(documents);

  executeQuery(query);
}

void CouchDB::uploadAttachment(const QString &database, const QString &id, const QString& attachmentName,
                               QByteArray attachment, QString mimeType, const QString& revision)
{
  Q_D(CouchDB);

  QByteArray postDataSize = QByteArray::number(attachment.size());

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/%2/%3/%4?rev=%5").arg(d->server->baseURL(), database, id, attachmentName, revision));
  query->setOperation(COUCHDB_DELETEDOCUMENT);
  query->setDatabase(database);
  query->setDocumentID(id);
  query->request()->setRawHeader("Content-Type", mimeType.toLatin1());
  query->request()->setRawHeader("Content-Length", postDataSize);
  query->setBody(attachment);

  executeQuery(query);
}

void CouchDB::deleteAttachment(const QString &database, const QString &id, const QString &attachmentName, const QString &revision)
{
  Q_D(CouchDB);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/%2/%3/%4?rev=%5").arg(d->server->baseURL(), database, id, attachmentName, revision));
  query->setOperation(COUCHDB_DELETEATTACHMENT);
  query->setDatabase(database);
  query->setDocumentID(id);

  executeQuery(query);
}

void CouchDB::replicateDatabaseFrom(CouchDBServer *sourceServer, const QString& sourceDatabase, const QString& targetDatabase,
                                    const bool& createTarget, const bool& continuous, const bool& cancel)
{
  Q_D(CouchDB);

  QString source = QString("%1/%2").arg(sourceServer->baseURL(true), sourceDatabase);
  QString target = d->server->url().contains("localhost") ? targetDatabase : QString("%1/%2").arg(d->server->baseURL(true), targetDatabase);

  replicateDatabase(source, target, targetDatabase, createTarget, continuous, cancel);
}

void CouchDB::replicateDatabaseTo(CouchDBServer *targetServer, const QString& sourceDatabase, const QString& targetDatabase,
                                  const bool& createTarget, const bool& continuous, const bool& cancel)
{
  Q_D(CouchDB);

  QString source = d->server->url().contains("localhost") ? sourceDatabase : QString("%1/%2").arg(d->server->baseURL(true), sourceDatabase);
  QString target = QString("%1/%2").arg(targetServer->baseURL(true), targetDatabase);

  replicateDatabase(source, target, targetDatabase, createTarget, continuous, cancel);
}

void CouchDB::replicateDatabase(const QString &source, const QString &target, const QString& database, const bool &createTarget,
                                const bool &continuous, const bool &cancel)
{
  Q_D(CouchDB);

  if (!cancel)
    QLOG_DEBUG() << QString("Starting replication from %1 to %2").arg(source).arg(target);
  else
    QLOG_DEBUG() << QString("Canceling replication from %1 to %2").arg(source).arg(target);

  QJsonObject object;
  object.insert("source", source);
  object.insert("target", target);
  object.insert("create_target", createTarget);
  object.insert("continuous", continuous);
  object.insert("cancel", cancel);
  QJsonDocument document(object);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);
  query->setUrl(QString("%1/_replicate").arg(d->server->baseURL(true)));
  query->setOperation(COUCHDB_REPLICATEDATABASE);
  query->setDatabase(database);
  query->request()->setRawHeader("Accept", "application/json");
  query->request()->setRawHeader("Content-Type", "application/json");
  query->setBody(document.toJson());

  executeQuery(query);
}

CouchDBListener* CouchDB::createListener(const QString &database, const QString &documentID)
{
  Q_D(CouchDB);

  CouchDBListener *listener = new CouchDBListener(d->server);
  listener->setCookieJar(d->networkManager->cookieJar());
  d->networkManager->cookieJar()->setParent(0);
  listener->setDatabase(database);
  listener->setDocumentID(documentID);
  listener->launch();

  QLOG_DEBUG() << QString("Created listener for database: %1, doc ID: %2").arg(database).arg(documentID);

  return listener;
}

void CouchDB::retrieveView(const QString &database, const QString &designDocName, const QString &viewName, QStringList keys)
{
  Q_D(CouchDB);

  CouchDBQuery *query = new CouchDBQuery(d->server, this);

  // Example: http://127.0.0.1:5984/<database>/<_design_doc/name>/_view/<view_name>
  QString q = QString("%1/%2/%3/_view/%4").arg(d->server->baseURL(), database, designDocName, viewName);
  //query->setUrl(QString("%1/%2/%3/_view/%4").arg(d->server->baseURL(), database, designDocName, viewName));

  if (keys.length() > 0)
  {
    // String enclose in quotes but quotes are NOT encoded!
    if (keys.length() == 1)
    {
      QString key = "\"" + QUrl::toPercentEncoding(keys.first()) + "\"";
      q += QString("?key=%1").arg(key);
    }
    else
    {
      QStringList encodedKeys;
      foreach (QString k, keys)
      {
        encodedKeys.append("\"" + QUrl::toPercentEncoding(k) + "\"");
      }

      // ?key=["key1","key2","..."]
      q += QString("?key=[%1]").arg(encodedKeys.join(","));
    }
  }

  //QLOG_DEBUG() << QString("Query string: %1").arg(q);

  query->setUrl(q);
  query->setOperation(COUCHDB_RETRIEVEVIEW);
  query->setDatabase(database);

  // Used by caller of this slot to route the result
  query->setProperty("callerID", viewName);

  executeQuery(query);
}

void CouchDB::sslErrors(const QList<QSslError> &errors)
{
  foreach (QSslError e, errors)
    QLOG_ERROR() << QString("Database server SSL error: %1").arg(e.errorString());
}
