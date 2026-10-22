#include "imageprovider.h"
#include "ddlog.h"
#include "vnotefolderoper.h"
#include <QDebug>
#include <qlogging.h>

ImageProvider *ImageProvider::instance()
{
    // qInfo() << "ImageProvider instance requested";
    static ImageProvider *imageProvider = nullptr;
    if (!imageProvider) {
        // qInfo() << "Creating new ImageProvider instance";
        imageProvider = new ImageProvider(ImageType::Pixmap);
    }
    return imageProvider;
}

QImage ImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    // qInfo() << "Requesting image for ID:" << id;
    return this->img;
}

QPixmap ImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    qCInfo(voiceNoteLog) << "Requesting pixmap for ID:" << id;
    VNoteFolderOper folderOper;
    int iconIndex = id.toInt();
    QPixmap pix = folderOper.getDefaultIcon(iconIndex, IconsType::DefaultIcon);
    qCInfo(voiceNoteLog) << "Pixmap request finished";
    return pix;
}

ImageProvider::ImageProvider(ImageType type)
    :QQuickImageProvider(type)
{
    qCInfo(voiceNoteLog) << "ImageProvider constructor called with type:" << static_cast<int>(type);
}
