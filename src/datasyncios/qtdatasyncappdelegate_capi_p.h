#ifndef QTDATASYNCAPPDELEGATE_CAPI_P_H
#define QTDATASYNCAPPDELEGATE_CAPI_P_H

#include <functional>
#include <QtCore/QtGlobal>

#include "iossyncdelegate.h"

// c -> mm
void QtDatasyncAppDelegateInitialize();
void setSyncInterval(double delaySeconds);

#endif // DATASYNCAPPDELEGATE_CAPI_P_H
