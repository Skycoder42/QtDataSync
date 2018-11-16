#ifndef QQMLIOSSYNCSINGLETON_H
#define QQMLIOSSYNCSINGLETON_H

#include <QtCore/QObject>

#include <QtDataSyncIos/IosSyncDelegate>

#ifdef DOXYGEN_RUN
namespace de::skycoder42::QtDataSync {

/*! @brief The QML bindings for the static methods of of QtDataSync::EventCursor
 *
 * @extends QtQml.QtObject
 * @since 4.2
 *
 * @sa QtDataSync::IosSyncDelegate
 */
class IosSyncSingleton
#else
namespace QtDataSync {

class QQmlIosSyncSingleton : public QObject
#endif
{
	Q_OBJECT

public:
	//! @private
	explicit QQmlIosSyncSingleton(QObject *parent = nullptr);

	//! Returns the currently registered sync delegate
	Q_INVOKABLE QtDataSync::IosSyncDelegate *delegate() const;
};

}

#endif // QQMLIOSSYNCSINGLETON_H
