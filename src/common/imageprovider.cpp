#include "imageprovider.h"
#include "vnotefolderoper.h"

ImageProvider *ImageProvider::instance()
{
    static ImageProvider *imageProvider = nullptr;
    if (!imageProvider) {
        imageProvider = new ImageProvider(ImageType::Pixmap);
    }
    return imageProvider;
}

QImage ImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    return this->img;
}

QPixmap ImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    VNoteFolderOper folderOper;
    int iconIndex = id.toInt();
    QPixmap pix = folderOper.getDefaultIcon(iconIndex, IconsType::DefaultIcon);
    return pix;
}

ImageProvider::ImageProvider(ImageType type)
    :QQuickImageProvider(type)
{

}
