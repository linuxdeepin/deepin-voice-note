#ifndef IMAGEPROVIDER_H
#define IMAGEPROVIDER_H

#include <QObject>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QImage>

class ImageProvider : public QQuickImageProvider
{
public:
    static ImageProvider* instance();
    virtual ~ImageProvider() = default;

    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize);
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize);

    QImage img;

private:
    explicit ImageProvider(ImageType type);
};

#endif // IMAGEPROVIDER_H
