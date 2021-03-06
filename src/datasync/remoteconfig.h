#ifndef QTDATASYNC_REMOTECONFIG_H
#define QTDATASYNC_REMOTECONFIG_H

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include <QtCore/qhash.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qvariant.h>

#include "QtDataSync/qtdatasync_global.h"

namespace QtDataSync {

class RemoteConfigPrivate;
//! A configuration on how to connect to a remote server
class Q_DATASYNC_EXPORT RemoteConfig
{
	Q_GADGET

	//! The websocket url of the server to connect to
	Q_PROPERTY(QUrl url READ url WRITE setUrl)
	//! A access secret needed in order to connect to the server
	Q_PROPERTY(QString accessKey READ accessKey WRITE setAccessKey)
	//! A collection of additional HTTP headers to be sent with the request
	Q_PROPERTY(QtDataSync::RemoteConfig::HeaderHash headers READ headers WRITE setHeaders STORED false)
	//! The keep alive timeout to be used to send pings to the server
	Q_PROPERTY(int keepaliveTimeout READ keepaliveTimeout WRITE setKeepaliveTimeout)

	//! Internal property that mirrors RemoteConfig::headers, but in a serializable format
	Q_PROPERTY(QVariantMap storedHeaders READ storedHeaders WRITE setStoredHeaders REVISION 2)

public:
	//! Typedef for a hash of additional HTTP headers
	using HeaderHash = QHash<QByteArray, QByteArray>;

	//! Default constructor, with optional parameters
	RemoteConfig(const QUrl &url = {},
				 const QString &accessKey = {},
				 const HeaderHash &headers = {},
				 int keepaliveTimeout = 1); //1 minute between ping messages (nginx timeout is 75 seconds be default)
	//! Copy constructor
	RemoteConfig(const RemoteConfig &other);
	//! Move constructor
	RemoteConfig(RemoteConfig &&other) noexcept;
	~RemoteConfig();

	//! Copy-Assignment operator
	RemoteConfig &operator=(const RemoteConfig &other);
	//! Move-Assignment operator
	RemoteConfig &operator=(RemoteConfig &&other) noexcept;

	//! Equality operator
	bool operator==(const RemoteConfig &other) const;
	//! Inequality operator
	bool operator!=(const RemoteConfig &other) const;

	//! @readAcFn{RemoteConfig::url}
	QUrl url() const;
	//! @readAcFn{RemoteConfig::accessKey}
	QString accessKey() const;
	//! @readAcFn{RemoteConfig::headers}
	HeaderHash headers() const;
	//! @readAcFn{RemoteConfig::keepaliveTimeout}
	int keepaliveTimeout() const;

	//! @writeAcFn{RemoteConfig::url}
	void setUrl(QUrl url);
	//! @writeAcFn{RemoteConfig::accessKey}
	void setAccessKey(QString accessKey);
	//! @writeAcFn{RemoteConfig::headers}
	void setHeaders(HeaderHash headers);
	//! @writeAcFn{RemoteConfig::keepaliveTimeout}
	void setKeepaliveTimeout(int keepaliveTimeout);

private:
	QSharedDataPointer<RemoteConfigPrivate> d;

	QVariantMap storedHeaders() const;
	void setStoredHeaders(const QVariantMap &map);

	friend Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RemoteConfig &deviceInfo);
	friend Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RemoteConfig &deviceInfo);
};

//! Stream operator to stream into a QDataStream
Q_DATASYNC_EXPORT QDataStream &operator<<(QDataStream &stream, const RemoteConfig &deviceInfo);
//! Stream operator to stream out of a QDataStream
Q_DATASYNC_EXPORT QDataStream &operator>>(QDataStream &stream, RemoteConfig &deviceInfo);

}

Q_DECLARE_METATYPE(QtDataSync::RemoteConfig)
Q_DECLARE_TYPEINFO(QtDataSync::RemoteConfig, Q_MOVABLE_TYPE);


#endif // QTDATASYNC_REMOTECONFIG_H
