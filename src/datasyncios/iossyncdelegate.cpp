#include "iossyncdelegate.h"
#include "iossyncdelegate_p.h"
#include "qtdatasyncappdelegate_capi_p.h"
#include <QtRemoteObjects/QRemoteObjectReplica>
using namespace QtDataSync;
using namespace std::chrono;

void IosSyncDelegate::init(IosSyncDelegate *delegate)
{
	QtDatasyncAppDelegateInitialize();

	if(IosSyncDelegatePrivate::delegateInstance) {
		if(!delegate)
			return;

		qWarning() << "Replacing existing IosSyncDelegate"
				   << IosSyncDelegatePrivate::delegateInstance
				   << "with"
				   << delegate;
	}

	if(!delegate)
		delegate = new IosSyncDelegate{};

	if(delegate->persistState()) {
		// if persisted, load the state
		auto settings = delegate->d->settings;
		settings->beginGroup(IosSyncDelegatePrivate::SyncStateGroup);
		delegate->setDelay(settings->value(IosSyncDelegatePrivate::SyncDelayKey,
										   delegate->delay())
						   .toLongLong());
		delegate->setEnabled(settings->value(IosSyncDelegatePrivate::SyncEnabledKey,
											 delegate->isEnabled())
							 .toBool());
		delegate->setWaitFullSync(settings->value(IosSyncDelegatePrivate::SyncWaitKey,
												  delegate->waitFullSync())
								  .toBool());
		settings->endGroup();
	}

	// once everything was loaded, trigger the update
	IosSyncDelegatePrivate::delegateInstance = delegate;
	delegate->d->allowUpdateSync = true;
	delegate->d->updateSyncInterval();
}

IosSyncDelegate *IosSyncDelegate::currentDelegate()
{
	return IosSyncDelegatePrivate::delegateInstance;
}

IosSyncDelegate::IosSyncDelegate(QObject *parent) :
	QObject{parent},
	d{new IosSyncDelegatePrivate{new QSettings{this}, this}}
{}

IosSyncDelegate::IosSyncDelegate(QSettings *settings, QObject *parent) :
	QObject{parent},
	d{new IosSyncDelegatePrivate{settings, this}}
{
	settings->setParent(this);
}

IosSyncDelegate::~IosSyncDelegate() = default;

qint64 IosSyncDelegate::delay() const
{
	return d->delay.count();
}

std::chrono::minutes IosSyncDelegate::delayMinutes() const
{
	return d->delay;
}

bool IosSyncDelegate::isEnabled() const
{
	return d->enabled;
}

bool IosSyncDelegate::waitFullSync() const
{
	return d->waitFullSync;
}

bool IosSyncDelegate::persistState() const
{
	return true;
}

void IosSyncDelegate::setDelay(std::chrono::minutes delay)
{
	if(d->delay == delay)
		return;

	d->delay = delay;
	emit delayChanged(d->delay.count(), {});
	d->updateSyncInterval();
}

void IosSyncDelegate::setDelay(qint64 delay)
{
	setDelay(minutes{delay});
}

void IosSyncDelegate::setEnabled(bool enabled)
{
	if(d->enabled == enabled)
		return;

	d->enabled = enabled;
	emit enabledChanged(d->enabled, {});
	d->updateSyncInterval();
}

void IosSyncDelegate::setWaitFullSync(bool waitFullSync)
{
	if (d->waitFullSync == waitFullSync)
		return;

	d->waitFullSync = waitFullSync;
	emit waitFullSyncChanged(d->waitFullSync, {});
	d->storeState();
}

QString IosSyncDelegate::setupName() const
{
	return DefaultSetup;
}

void IosSyncDelegate::prepareSetup(Setup &setup)
{
	Q_UNUSED(setup);
}

IosSyncDelegate::SyncResult IosSyncDelegate::onSyncCompleted(SyncManager::SyncState state)
{
	switch (state) {
	case QtDataSync::SyncManager::Downloading:
	case QtDataSync::SyncManager::Uploading:
	case QtDataSync::SyncManager::Synchronized:
		return SyncResult::NewData;
	case QtDataSync::SyncManager::Disconnected:
		return SyncResult::NoData;
	case QtDataSync::SyncManager::Error:
	default:
		return SyncResult::Error;
	}
}

void IosSyncDelegate::performSync(std::function<void(SyncResult)> callback)
{
	// if handler is set, a sync is currently in progress
	if(d->currentCallback) {
		qWarning() << "completionHandler already set - replacing with newer handler";
		d->currentCallback = std::move(callback);
		return;
	}

	try {
		// create the setup
		if(!Setup::exists(setupName())) {
			Setup setup;
			prepareSetup(setup);
			setup.create(setupName());
		}

		// create the manager
		if(!d->manager)
			d->manager = new SyncManager{setupName(), this};

		if(d->manager->replica()->isInitialized())
			backendReady();
		else {
			connect(d->manager->replica(), &QRemoteObjectReplica::initialized,
					this, &IosSyncDelegate::backendReady,
					Qt::UniqueConnection);
		}
	} catch (Exception &e) {
		qCritical() << e.what();
		d->currentCallback(SyncResult::Error);
		d->currentCallback = {};
	}
}

void IosSyncDelegate::backendReady()
{
	const auto syncFn = std::bind(&IosSyncDelegatePrivate::onSyncDone, d.data(), std::placeholders::_1);
	if(d->waitFullSync)
		d->manager->runOnSynchronized(syncFn, true);
	else
		d->manager->runOnDownloaded(syncFn, true);
}

// ------------- PRIVATE IMPLEMENTATION -------------

const QString IosSyncDelegatePrivate::SyncStateGroup = QStringLiteral("qtdatasync::iossyncdelegate");
const QString IosSyncDelegatePrivate::SyncDelayKey = QStringLiteral("delay");
const QString IosSyncDelegatePrivate::SyncEnabledKey = QStringLiteral("enabled");
const QString IosSyncDelegatePrivate::SyncWaitKey = QStringLiteral("wait");
QPointer<IosSyncDelegate> IosSyncDelegatePrivate::delegateInstance;

void IosSyncDelegatePrivate::performFetch(std::function<void (IosSyncDelegate::SyncResult)> callback)
{
	if(delegateInstance)
		delegateInstance->performSync(std::move(callback));
	else {
		qCritical() << "performFetchWithCompletionHandler was triggered without initializing an IosSyncDelegate first";
		callback(IosSyncDelegate::SyncResult::Error);
	}
}

IosSyncDelegatePrivate::IosSyncDelegatePrivate(QSettings *settings, IosSyncDelegate *q_ptr) :
	q{q_ptr},
	settings{settings}
{}

void IosSyncDelegatePrivate::updateSyncInterval()
{
	if(!allowUpdateSync)
		return;

	if(enabled)
		::setSyncInterval(duration_cast<milliseconds>(delay).count() / 1000.0);
	else
		::setSyncInterval(-1.0);

	storeState();
}

void IosSyncDelegatePrivate::storeState()
{
	if(q->persistState()) {
		settings->beginGroup(SyncStateGroup);
		settings->setValue(SyncDelayKey, QVariant::fromValue<qint64>(delay.count()));
		settings->setValue(SyncEnabledKey, enabled);
		settings->setValue(SyncWaitKey, waitFullSync);
		settings->endGroup();
	}
}

void IosSyncDelegatePrivate::onSyncDone(SyncManager::SyncState state)
{
	auto result = q->onSyncCompleted(state);
	if(currentCallback) {
		currentCallback(result);
		currentCallback = {};
	}
}
