#ifndef FAILABLECLOUDTRANSFORMER_H
#define FAILABLECLOUDTRANSFORMER_H

#include <QtDataSync/PlainCloudTransformer>
#include <QtDataSync/exception.h>

class FailableCloudTransformer : public QtDataSync::PlainCloudTransformer
{
public:
	class Exception : public QtDataSync::Exception
	{
	public:
		QString qWhat() const override;
		void raise() const override;
		QtDataSync::ExceptionBase * clone() const override;
	};

	FailableCloudTransformer(QObject *parent = nullptr);

	bool shouldFail = false;

protected:
	QJsonObject transformUploadSync(const QVariantHash &data) const override;
	QVariantHash transformDownloadSync(const QJsonObject &data) const override;
};

#endif // FAILABLECLOUDTRANSFORMER_H
