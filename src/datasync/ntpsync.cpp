#include "ntpsync_p.h"
#include <qntp/NtpReply.h>
using namespace QtDataSync;
using namespace std::chrono;
using namespace std::chrono_literals;

Q_LOGGING_CATEGORY(QtDataSync::logNtp, "qt.datasync.NtpSync")

NtpSync::NtpSync(QObject *parent) :
	QObject{parent},
	_ntpClient{new NtpClient{this}},
	_ntpTimer{new QTimer{this}}
{
	connect(_ntpClient, &NtpClient::replyReceived,
			this, &NtpSync::ntpReply);

	_ntpTimer->setInterval(_ntpMinPoll);
	_ntpTimer->setTimerType(Qt::VeryCoarseTimer);
	connect(_ntpTimer, &QTimer::timeout,
			this, &NtpSync::ntpSync);
}

void NtpSync::syncWith(const QString &hostName, quint16 port)
{
	_ntpPort = port;
	QHostAddress addr{hostName};
	if (addr.isNull()) {
		qCDebug(logNtp) << "Resolving host" << hostName << "...";
		QHostInfo::lookupHost(hostName, this, &NtpSync::ntpLookup);
	} else {
		_ntpHost = std::move(addr);
		ntpSync();
		_ntpTimer->start();
	}
}

void NtpSync::ntpLookup(QHostInfo info)
{
	if (info.error() == QHostInfo::NoError) {
		if (const auto addr = info.addresses(); !addr.isEmpty()) {
			qCDebug(logNtp) << "Found ip address for" << info.hostName()
							<< "as" << addr.first();
			_ntpHost = addr.first();
			ntpSync();
			_ntpTimer->start();
		} else {
			qCWarning(logNtp) << "Host lookup for" << info.hostName()
							  << "successful, but no ip addresses were found!";
		}
	} else {
		qCWarning(logNtp) << "Failed to lookup host" << info.hostName()
						  << "with error" << qUtf8Printable(info.errorString());
	}
}

void NtpSync::ntpSync()
{
	qCDebug(logNtp) << "Sending NTP request...";
	_ntpClient->sendRequest(_ntpHost, _ntpPort);
}

void NtpSync::ntpReply(const QHostAddress &address, quint16 port, const NtpReply &reply)
{
	Q_UNUSED(address);
	Q_UNUSED(port);

	auto currentPoll = round<seconds>(_ntpTimer->intervalAsDuration());

	// invalid reply -> slow down requests to relieve server
	if (reply.isNull()) {
		qCWarning(logNtp) << "Received invalid NTP reply, decreasing request rate";
		currentPoll = min(seconds{currentPoll.count() << 1}, NtpMaxPoll);
		_ntpTimer->setInterval(currentPoll);
		qCDebug(logNtp) << "Updated polling interval to" << currentPoll.count() << "s";
		return;
	}

	// read/calc values
	milliseconds offset{reply.localClockOffset()};
	_ntpMinPoll = seconds{qRound64(reply.pollInterval())};
	const auto delta = abs(_ntpCorrection - offset);

	if (delta > 150ms) {  // high delta -> poll more frequent, but honor min poll
		qCWarning(logNtp) << "High NTP offset delta of" << delta.count()
						  << "ms - increasing request rate";
		currentPoll = max(seconds{currentPoll.count() >> 1}, _ntpMinPoll);
	} else if (delta <= 50ms) {  // low delta -> poll less frequent, but honor max poll
		qCDebug(logNtp) << "Low NTP offset delta of" << delta.count()
						<< "ms - decreasing request rate";
		currentPoll = min(seconds{currentPoll.count() << 1}, NtpMaxPoll);
	} else {  // average delta -> poll the same, but honor min poll, which could have been increased
		qCDebug(logNtp) << "Acceptable NTP offset delta of"
						<< delta.count() << "ms";
		currentPoll = max(currentPoll, _ntpMinPoll);
	}

	// update sync timer and correction value
	if (currentPoll != _ntpTimer->intervalAsDuration()) {
		_ntpTimer->setInterval(currentPoll);
		qCDebug(logNtp) << "Updated polling interval to" << currentPoll.count() << "s";
	}

	if (delta > 0ms) {
		_ntpCorrection = std::move(offset);
		qCDebug(logNtp) << "Updated NTP offset to" << _ntpCorrection.count() << "ms";
		Q_EMIT updateNtpOffset(_ntpCorrection, {});
	}
}
