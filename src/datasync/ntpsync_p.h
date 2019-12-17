#ifndef QTDATASYNC_NTPSYNC_H
#define QTDATASYNC_NTPSYNC_H

#include "qtdatasync_global.h"

#include <chrono>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QLoggingCategory>

#include <QtNetwork/QHostAddress>
#include <QtNetwork/QHostInfo>

#include <qntp/NtpClient.h>

namespace QtDataSync {

class Q_DATASYNC_EXPORT NtpSync : public QObject
{
	Q_OBJECT

public:
	static constexpr quint16 NtpDefaultPort = 123;
	static constexpr std::chrono::seconds NtpMinPoll {64};
	static constexpr std::chrono::seconds NtpMaxPoll {1024};

	explicit NtpSync(QObject *parent = nullptr);

public Q_SLOTS:
	void syncWith(const QString &hostName, quint16 port = NtpDefaultPort);

Q_SIGNALS:
	void updateNtpOffset(std::chrono::milliseconds, QPrivateSignal);

private Q_SLOTS:
	void ntpLookup(QHostInfo info);
	void ntpSync();
	void ntpReply(const QHostAddress &address, quint16 port, const NtpReply &reply);

private:
	NtpClient *_ntpClient;
	QTimer *_ntpTimer;
	QHostAddress _ntpHost;
	quint16 _ntpPort = NtpDefaultPort;
	std::chrono::seconds _ntpMinPoll {NtpMinPoll};
	std::chrono::milliseconds _ntpCorrection {0};
};

Q_DECLARE_LOGGING_CATEGORY(logNtp)

}

#endif // QTDATASYNC_NTPSYNC_H
