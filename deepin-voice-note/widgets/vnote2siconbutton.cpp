#include "vnote2siconbutton.h"
#include "common/utils.h"

VNote2SIconButton::VNote2SIconButton(
        const QString normal, const QString press, QWidget *parent)
    : DFloatingButton(parent)
{
    m_icons[Normal] = normal;
    m_icons[Press] = press;

    updateIcon();
}

bool VNote2SIconButton::isPressed() const
{
    return (Press == m_state);
}

void VNote2SIconButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (rect().contains(event->pos()) && isEnabled()) {
        if (Normal == m_state) {
            m_state = Press;
        } else {
            m_state = Normal;
        }

        updateIcon();
    }

    DIconButton::mouseReleaseEvent(event);
}

void VNote2SIconButton::updateIcon()
{
    if (!m_icons[m_state].isEmpty()) {
        setIcon(Utils::loadSVG(m_icons[m_state], m_useCommonIcons));
    }
}

void VNote2SIconButton::setCommonIcon(bool isCommon)
{
    m_useCommonIcons = isCommon;
}
